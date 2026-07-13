from __future__ import annotations

import json
from pathlib import Path

import httpx
import pytest

from dualfire_art.comfy import ComfyClient, load_workflow, render_workflow
from dualfire_art.config import GenerationConfig, ModelConfig, PipelineConfig
from dualfire_art.errors import PipelineError


def make_config(tmp_path: Path) -> PipelineConfig:
    workflow = Path(__file__).resolve().parents[1] / "workflows" / "qwen_image_edit_2511_api.json"
    return PipelineConfig(
        comfy_url="http://127.0.0.1:8001",
        comfy_data_dir=tmp_path,
        workflow_file=workflow,
        models=ModelConfig(
            diffusion="diffusion.safetensors",
            text_encoder="encoder.safetensors",
            vae="vae.safetensors",
        ),
        generation=GenerationConfig(timeout_seconds=1, poll_interval_seconds=0),
    )


def test_render_workflow_injects_runtime_values(tmp_path: Path) -> None:
    config = make_config(tmp_path)
    workflow = render_workflow(
        load_workflow(config.workflow_file),
        config,
        "dualfire/input.png",
        "test prompt",
        42,
        "dualfire/output",
    )
    assert workflow["1"]["inputs"]["image"] == "dualfire/input.png"
    assert workflow["2"]["inputs"]["unet_name"] == "diffusion.safetensors"
    assert workflow["7"]["inputs"]["prompt"] == "test prompt"
    assert workflow["11"]["inputs"]["seed"] == 42
    assert workflow["11"]["inputs"]["steps"] == 40


def test_render_lightning_workflow_injects_lora(tmp_path: Path) -> None:
    workflow_file = Path(__file__).resolve().parents[1] / "workflows" / "qwen_image_edit_2511_lightning_4step_api.json"
    config = PipelineConfig(
        comfy_url="http://127.0.0.1:8001",
        comfy_data_dir=tmp_path,
        workflow_file=workflow_file,
        models=ModelConfig(
            diffusion="diffusion.safetensors",
            text_encoder="encoder.safetensors",
            vae="vae.safetensors",
            lightning_lora="lightning.safetensors",
        ),
        generation=GenerationConfig(steps=4, cfg=1.0, timeout_seconds=1, poll_interval_seconds=0),
    )

    workflow = render_workflow(load_workflow(config.workflow_file), config, "input.png", "test prompt", 42, "output")

    assert workflow["14"]["inputs"]["lora_name"] == "lightning.safetensors"
    assert workflow["11"]["inputs"]["steps"] == 4
    assert workflow["11"]["inputs"]["cfg"] == 1.0
    assert config.model_paths["lightning_lora"] == tmp_path / "models" / "loras" / "lightning.safetensors"


def test_lightning_workflow_requires_lora(tmp_path: Path) -> None:
    config = make_config(tmp_path)
    workflow_file = Path(__file__).resolve().parents[1] / "workflows" / "qwen_image_edit_2511_lightning_4step_api.json"

    with pytest.raises(PipelineError, match="requires models.lightning_lora"):
        render_workflow(load_workflow(workflow_file), config, "input.png", "test prompt", 42, "output")


def test_comfy_client_prompt_history_and_download(tmp_path: Path) -> None:
    def handler(request: httpx.Request) -> httpx.Response:
        if request.url.path == "/prompt":
            return httpx.Response(200, json={"prompt_id": "p1"})
        if request.url.path == "/history/p1":
            return httpx.Response(200, json={"p1": {"outputs": {"13": {"images": [{"filename": "out.png", "subfolder": "dualfire", "type": "output"}]}}}})
        if request.url.path == "/view":
            return httpx.Response(200, content=b"png-bytes")
        raise AssertionError(request.url)

    config = make_config(tmp_path)
    with ComfyClient(config, transport=httpx.MockTransport(handler)) as client:
        prompt_id = client.submit({"1": {"class_type": "Test", "inputs": {}}})
        images = client.wait_for_images(prompt_id)
        payload = client.download_image(images[0])
    assert prompt_id == "p1"
    assert images[0]["filename"] == "out.png"
    assert payload == b"png-bytes"


def test_http_error_reports_the_request_target(tmp_path: Path) -> None:
    def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(400, request=request, json={"detail": "Bad Request"})

    with ComfyClient(make_config(tmp_path), transport=httpx.MockTransport(handler)) as client:
        with pytest.raises(PipelineError, match=r"POST http://127\.0\.0\.1:8001/prompt HTTP 400"):
            client.submit({"1": {}})
