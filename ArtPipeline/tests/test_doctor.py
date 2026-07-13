from __future__ import annotations

from dataclasses import replace
from pathlib import Path

import httpx

from dualfire_art.comfy import REQUIRED_NODES
from dualfire_art.config import GenerationConfig, ModelConfig, PipelineConfig
from dualfire_art.doctor import doctor_succeeded, run_doctor


def make_config(tmp_path: Path) -> PipelineConfig:
    workflow = Path(__file__).resolve().parents[1] / "workflows" / "qwen_image_edit_2511_api.json"
    return PipelineConfig(
        comfy_url="http://127.0.0.1:8001",
        comfy_data_dir=tmp_path,
        workflow_file=workflow,
        models=ModelConfig("diffusion.safetensors", "encoder.safetensors", "vae.safetensors"),
        generation=GenerationConfig(timeout_seconds=1),
    )


def test_doctor_reports_missing_models_and_api(tmp_path: Path) -> None:
    def handler(request: httpx.Request) -> httpx.Response:
        return httpx.Response(503, text="offline")

    results = run_doctor(make_config(tmp_path), transport=httpx.MockTransport(handler))
    assert not doctor_succeeded(results)
    assert any(result.name == "model:diffusion" and not result.ok for result in results)
    assert any(result.name == "comfy_api" and not result.ok for result in results)


def test_doctor_passes_with_models_and_required_nodes(tmp_path: Path) -> None:
    config = make_config(tmp_path)
    for path in config.model_paths.values():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"model")

    def handler(request: httpx.Request) -> httpx.Response:
        if request.url.path == "/system_stats":
            return httpx.Response(200, json={"devices": [{"name": "RTX 4090"}]})
        if request.url.path == "/object_info":
            return httpx.Response(200, json={name: {} for name in REQUIRED_NODES})
        raise AssertionError(request.url)

    results = run_doctor(config, transport=httpx.MockTransport(handler))
    assert doctor_succeeded(results), results


def test_doctor_requires_lightning_lora_node_for_lightning_workflow(tmp_path: Path) -> None:
    config = replace(
        make_config(tmp_path),
        workflow_file=Path(__file__).resolve().parents[1] / "workflows" / "qwen_image_edit_2511_lightning_4step_api.json",
        models=ModelConfig(
            diffusion="diffusion.safetensors",
            text_encoder="encoder.safetensors",
            vae="vae.safetensors",
            lightning_lora="lightning.safetensors",
        ),
    )
    for path in config.model_paths.values():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(b"model")

    def handler(request: httpx.Request) -> httpx.Response:
        if request.url.path == "/system_stats":
            return httpx.Response(200, json={"devices": [{"name": "RTX 4090"}]})
        if request.url.path == "/object_info":
            return httpx.Response(200, json={name: {} for name in REQUIRED_NODES})
        raise AssertionError(request.url)

    results = run_doctor(config, transport=httpx.MockTransport(handler))

    lora_check = next(result for result in results if result.name == "comfy_nodes")
    assert not lora_check.ok
    assert "LoraLoaderModelOnly" in lora_check.detail
