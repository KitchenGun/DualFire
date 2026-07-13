from __future__ import annotations

import os
import shutil
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Mapping, Sequence

from .config import PipelineConfig, load_pipeline_config
from .errors import PipelineError


MCP_PACKAGE = "comfyui-mcp@0.30.0"
MCP_HARDENING_ENVIRONMENT = {
    # The pipeline must not mutate ComfyUI by installing UI nodes or self-updating.
    "COMFYUI_MCP_PANEL_AUTOINSTALL": "0",
    "COMFYUI_MCP_AUTOUPDATE": "0",
}
BLOCKED_ENVIRONMENT_KEYS = (
    "CIVITAI_API_TOKEN",
    "COMFY_API_KEY",
    "COMFYUI_ALWAYS_RESTART",
    "COMFYUI_API_KEY",
    "COMFYUI_AUTH_HEADER",
    "COMFYUI_AUTH_TOKEN",
    "COMFYUI_CLOUD_URL",
    "COMFYUI_MCP_FORCE_REMOTE",
    "GITHUB_TOKEN",
    "HF_TOKEN",
    "HUGGINGFACE_TOKEN",
    "REGISTRY_ACCESS_TOKEN",
)


@dataclass(frozen=True)
class McpLaunchContext:
    command: tuple[str, ...]
    environment: dict[str, str]


def prepare_mcp_launch(
    config_path: Path,
    *,
    npx_executable: str | None = None,
    base_environment: Mapping[str, str] | None = None,
) -> McpLaunchContext:
    if not config_path.is_file():
        raise PipelineError(f"Local pipeline config is missing: {config_path}")

    config = load_pipeline_config(config_path)
    _validate_data_directory(config)
    npx = npx_executable or _find_npx()

    environment = dict(base_environment if base_environment is not None else os.environ)
    for key in BLOCKED_ENVIRONMENT_KEYS:
        environment.pop(key, None)
    environment["COMFYUI_URL"] = config.comfy_url
    environment["COMFYUI_PATH"] = str(config.comfy_data_dir.resolve())
    environment.update(MCP_HARDENING_ENVIRONMENT)

    command = (npx, "-y", MCP_PACKAGE, "--comfyui-url", config.comfy_url)
    return McpLaunchContext(command=command, environment=environment)


def run_mcp_server(config_path: Path) -> int:
    context = prepare_mcp_launch(config_path)
    completed = subprocess.run(context.command, env=context.environment, check=False)
    return completed.returncode


def main(argv: Sequence[str] | None = None) -> int:
    args = list(argv if argv is not None else sys.argv[1:])
    default_path = Path(__file__).resolve().parents[1] / "config" / "pipeline.local.yaml"
    config_path = Path(args[0]).expanduser().resolve() if args else default_path
    try:
        return run_mcp_server(config_path)
    except PipelineError as error:
        print(f"ComfyUI MCP launcher error: {error}", file=sys.stderr)
        return 1


def _validate_data_directory(config: PipelineConfig) -> None:
    if not config.comfy_data_dir.is_dir():
        raise PipelineError(f"ComfyUI data directory is missing: {config.comfy_data_dir}")


def _find_npx() -> str:
    executable = shutil.which("npx.cmd") or shutil.which("npx")
    if executable is None:
        raise PipelineError("npx was not found on PATH; Node.js 22 or newer is required")
    return executable
