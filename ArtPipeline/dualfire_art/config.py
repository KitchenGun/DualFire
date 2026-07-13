from __future__ import annotations

import ipaddress
from dataclasses import dataclass
from pathlib import Path
from urllib.parse import urlparse

from .errors import PipelineError
from .util import expand_environment, load_yaml


REQUIRED_MODEL_KEYS = ("diffusion", "text_encoder", "vae")


@dataclass(frozen=True)
class ModelConfig:
    diffusion: str
    text_encoder: str
    vae: str
    lightning_lora: str | None = None

    def as_dict(self) -> dict[str, str]:
        result = {
            "diffusion": self.diffusion,
            "text_encoder": self.text_encoder,
            "vae": self.vae,
        }
        if self.lightning_lora:
            result["lightning_lora"] = self.lightning_lora
        return result


@dataclass(frozen=True)
class GenerationConfig:
    steps: int = 40
    cfg: float = 4.0
    sampler: str = "euler"
    scheduler: str = "simple"
    denoise: float = 1.0
    candidates: int = 4
    seed_base: int = 250900
    timeout_seconds: float = 600.0
    poll_interval_seconds: float = 1.0


@dataclass(frozen=True)
class PipelineConfig:
    comfy_url: str
    comfy_data_dir: Path
    workflow_file: Path
    models: ModelConfig
    generation: GenerationConfig

    @property
    def model_paths(self) -> dict[str, Path]:
        paths = {
            "diffusion": self.comfy_data_dir / "models" / "diffusion_models" / self.models.diffusion,
            "text_encoder": self.comfy_data_dir / "models" / "text_encoders" / self.models.text_encoder,
            "vae": self.comfy_data_dir / "models" / "vae" / self.models.vae,
        }
        if self.models.lightning_lora:
            paths["lightning_lora"] = self.comfy_data_dir / "models" / "loras" / self.models.lightning_lora
        return paths


def load_pipeline_config(path: Path) -> PipelineConfig:
    raw = expand_environment(load_yaml(path))
    comfy = raw.get("comfy")
    models = raw.get("models")
    generation = raw.get("generation", {})
    if not isinstance(comfy, dict) or not isinstance(models, dict):
        raise PipelineError("Pipeline config requires comfy and models mappings")
    missing = [key for key in REQUIRED_MODEL_KEYS if not models.get(key)]
    if missing:
        raise PipelineError(f"Missing model config keys: {', '.join(missing)}")

    root = path.parent.parent.resolve()
    workflow_value = raw.get("workflow_file", "workflows/qwen_image_edit_2511_api.json")
    workflow_file = Path(workflow_value)
    if not workflow_file.is_absolute():
        workflow_file = root / workflow_file

    result = PipelineConfig(
        comfy_url=str(comfy.get("url", "http://127.0.0.1:8001")).rstrip("/"),
        comfy_data_dir=Path(str(comfy.get("data_dir", ""))).expanduser(),
        workflow_file=workflow_file.resolve(),
        models=ModelConfig(
            **{key: str(models[key]) for key in REQUIRED_MODEL_KEYS},
            lightning_lora=str(models["lightning_lora"]) if models.get("lightning_lora") else None,
        ),
        generation=GenerationConfig(
            steps=int(generation.get("steps", 40)),
            cfg=float(generation.get("cfg", 4.0)),
            sampler=str(generation.get("sampler", "euler")),
            scheduler=str(generation.get("scheduler", "simple")),
            denoise=float(generation.get("denoise", 1.0)),
            candidates=int(generation.get("candidates", 4)),
            seed_base=int(generation.get("seed_base", 250900)),
            timeout_seconds=float(generation.get("timeout_seconds", 600.0)),
            poll_interval_seconds=float(generation.get("poll_interval_seconds", 1.0)),
        ),
    )
    validate_loopback_url(result.comfy_url)
    return result


def validate_loopback_url(url: str) -> None:
    parsed = urlparse(url)
    if parsed.scheme not in {"http", "https"} or not parsed.hostname:
        raise PipelineError(f"Invalid ComfyUI URL: {url}")
    hostname = parsed.hostname.lower()
    try:
        is_loopback = ipaddress.ip_address(hostname).is_loopback
    except ValueError:
        is_loopback = hostname == "localhost"
    if not is_loopback:
        raise PipelineError(f"ComfyUI URL must use loopback only: {url}")
