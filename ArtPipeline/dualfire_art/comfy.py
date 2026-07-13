from __future__ import annotations

import copy
import json
import time
import uuid
from pathlib import Path
from typing import Any
from urllib.parse import urlparse

import httpx

from .config import PipelineConfig
from .errors import PipelineError


REQUIRED_NODES = {
    "CFGNorm",
    "CLIPLoader",
    "FluxKontextImageScale",
    "KSampler",
    "LoadImage",
    "ModelSamplingAuraFlow",
    "SaveImage",
    "TextEncodeQwenImageEditPlus",
    "UNETLoader",
    "VAEDecode",
    "VAEEncode",
    "VAELoader",
}
LIGHTNING_LORA_NODE = "LoraLoaderModelOnly"


class ComfyClient:
    def __init__(self, config: PipelineConfig, transport: httpx.BaseTransport | None = None):
        self.config = config
        self.client_id = str(uuid.uuid4())
        self.client = httpx.Client(
            base_url=config.comfy_url,
            timeout=config.generation.timeout_seconds,
            transport=transport,
        )

    def __enter__(self) -> "ComfyClient":
        return self

    def __exit__(self, exc_type, exc_value, traceback) -> None:
        self.client.close()

    def system_stats(self) -> dict[str, Any]:
        return self._get_json("/system_stats")

    def object_info(self) -> dict[str, Any]:
        return self._get_json("/object_info")

    def upload_image(self, image_path: Path, remote_name: str) -> str:
        with image_path.open("rb") as image_file:
            response = self.client.post(
                "/upload/image",
                files={"image": (remote_name, image_file, "image/png")},
                data={"type": "input", "subfolder": "dualfire", "overwrite": "false"},
            )
        self._raise(response)
        data = response.json()
        filename = data.get("name") or remote_name
        subfolder = data.get("subfolder") or "dualfire"
        return f"{subfolder}/{filename}" if subfolder else filename

    def submit(self, workflow: dict[str, Any]) -> str:
        response = self.client.post(
            "/prompt",
            json={"prompt": workflow, "client_id": self.client_id},
        )
        self._raise(response)
        prompt_id = response.json().get("prompt_id")
        if not prompt_id:
            raise PipelineError(f"ComfyUI did not return prompt_id: {response.text}")
        return str(prompt_id)

    def wait_for_images(self, prompt_id: str) -> list[dict[str, str]]:
        deadline = time.monotonic() + self.config.generation.timeout_seconds
        while time.monotonic() < deadline:
            history = self._get_json(f"/history/{prompt_id}")
            prompt_history = history.get(prompt_id)
            if prompt_history:
                status = prompt_history.get("status", {})
                if status.get("status_str") == "error":
                    raise PipelineError(f"ComfyUI prompt failed: {prompt_id}")
                images = _collect_output_images(prompt_history.get("outputs", {}))
                if images:
                    return images
            time.sleep(self.config.generation.poll_interval_seconds)
        raise PipelineError(f"Timed out waiting for ComfyUI prompt: {prompt_id}")

    def prompt_history(self, prompt_id: str) -> dict[str, Any]:
        return self._get_json(f"/history/{prompt_id}")

    def download_image(self, image_info: dict[str, str]) -> bytes:
        response = self.client.get("/view", params=image_info)
        self._raise(response)
        return response.content

    def _get_json(self, path: str) -> dict[str, Any]:
        try:
            response = self.client.get(path)
            self._raise(response)
            data = response.json()
        except (httpx.HTTPError, ValueError) as error:
            raise PipelineError(f"ComfyUI request failed for {path}: {error}") from error
        if not isinstance(data, dict):
            raise PipelineError(f"ComfyUI returned non-object JSON for {path}")
        return data

    @staticmethod
    def _raise(response: httpx.Response) -> None:
        try:
            response.raise_for_status()
        except httpx.HTTPStatusError as error:
            request = response.request
            raise PipelineError(
                f"ComfyUI {request.method} {request.url} HTTP {response.status_code}: {response.text}"
            ) from error


def load_workflow(path: Path) -> dict[str, Any]:
    if not path.is_file():
        raise PipelineError(f"Missing API workflow: {path}")
    try:
        workflow = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise PipelineError(f"Invalid API workflow: {path}: {error}") from error
    if not isinstance(workflow, dict) or not workflow:
        raise PipelineError(f"API workflow must be a non-empty object: {path}")
    return workflow


def render_workflow(
    template: dict[str, Any],
    config: PipelineConfig,
    input_name: str,
    prompt: str,
    seed: int,
    output_prefix: str,
) -> dict[str, Any]:
    workflow = copy.deepcopy(template)
    _set_input(workflow, "1", "image", input_name)
    _set_input(workflow, "2", "unet_name", config.models.diffusion)
    _set_input(workflow, "3", "clip_name", config.models.text_encoder)
    _set_input(workflow, "4", "vae_name", config.models.vae)
    _set_input(workflow, "7", "prompt", prompt)
    _set_input(workflow, "11", "seed", seed)
    _set_input(workflow, "11", "steps", config.generation.steps)
    _set_input(workflow, "11", "cfg", config.generation.cfg)
    _set_input(workflow, "11", "sampler_name", config.generation.sampler)
    _set_input(workflow, "11", "scheduler", config.generation.scheduler)
    _set_input(workflow, "11", "denoise", config.generation.denoise)
    _set_input(workflow, "13", "filename_prefix", output_prefix)
    _set_lightning_lora(workflow, config.models.lightning_lora)
    return workflow


def validate_workflow_nodes(workflow: dict[str, Any]) -> list[str]:
    return sorted(required_nodes_for_workflow(workflow) - _workflow_node_classes(workflow))


def required_nodes_for_workflow(workflow: dict[str, Any]) -> set[str]:
    classes = _workflow_node_classes(workflow)
    return REQUIRED_NODES | ({LIGHTNING_LORA_NODE} if LIGHTNING_LORA_NODE in classes else set())


def _workflow_node_classes(workflow: dict[str, Any]) -> set[str]:
    classes = {str(node.get("class_type")) for node in workflow.values() if isinstance(node, dict)}
    return classes


def is_loopback_url(url: str) -> bool:
    hostname = (urlparse(url).hostname or "").lower()
    return hostname in {"127.0.0.1", "localhost", "::1"}


def _set_input(workflow: dict[str, Any], node_id: str, name: str, value: Any) -> None:
    try:
        workflow[node_id]["inputs"][name] = value
    except KeyError as error:
        raise PipelineError(f"Workflow node {node_id} is missing input {name}") from error


def _set_lightning_lora(workflow: dict[str, Any], lora_name: str | None) -> None:
    lora_nodes = [
        node
        for node in workflow.values()
        if isinstance(node, dict) and node.get("class_type") == "LoraLoaderModelOnly"
    ]
    if not lora_nodes:
        return
    if not lora_name:
        raise PipelineError("Lightning workflow requires models.lightning_lora")
    for node in lora_nodes:
        inputs = node.get("inputs")
        if not isinstance(inputs, dict) or "lora_name" not in inputs:
            raise PipelineError("Lightning workflow is missing the lora_name input")
        inputs["lora_name"] = lora_name


def _collect_output_images(outputs: dict[str, Any]) -> list[dict[str, str]]:
    result: list[dict[str, str]] = []
    for output in outputs.values():
        if not isinstance(output, dict):
            continue
        for image in output.get("images", []):
            if isinstance(image, dict) and image.get("filename"):
                result.append(
                    {
                        "filename": str(image["filename"]),
                        "subfolder": str(image.get("subfolder", "")),
                        "type": str(image.get("type", "output")),
                    }
                )
    return result
