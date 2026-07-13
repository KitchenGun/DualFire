from __future__ import annotations

import tomllib
from pathlib import Path

import pytest

from dualfire_art.errors import PipelineError
from dualfire_art.mcp_launcher import (
    BLOCKED_ENVIRONMENT_KEYS,
    MCP_HARDENING_ENVIRONMENT,
    MCP_PACKAGE,
    prepare_mcp_launch,
)


PROJECT_ROOT = Path(__file__).resolve().parents[2]
EXPECTED_ENABLED_TOOLS = {
    "get_system_stats",
    "list_skills",
    "read_skill",
    "list_packs",
    "read_pack_workflow",
    "list_workflow_templates",
    "check_workflow_runtime",
    "list_local_models",
    "get_node_info",
    "list_workflows",
    "get_workflow",
    "upload_image",
    "validate_workflow",
    "enqueue_workflow",
    "get_job_status",
    "get_queue",
    "cancel_job",
    "list_output_images",
    "view_image",
    "analyze_color",
    "list_assets",
    "get_asset_metadata",
    "get_history",
    "get_logs",
}


def write_config(tmp_path: Path, *, url: str, data_dir: Path) -> Path:
    config_path = tmp_path / "config" / "pipeline.local.yaml"
    config_path.parent.mkdir(parents=True)
    config_path.write_text(
        f"""comfy:
  url: {url}
  data_dir: {data_dir.as_posix()}
workflow_file: workflows/qwen_image_edit_2511_api.json
models:
  diffusion: diffusion.safetensors
  text_encoder: encoder.safetensors
  vae: vae.safetensors
""",
        encoding="utf-8",
    )
    return config_path


def test_launcher_rejects_missing_local_config(tmp_path: Path) -> None:
    with pytest.raises(PipelineError, match="config is missing"):
        prepare_mcp_launch(tmp_path / "missing.yaml", npx_executable="npx")


def test_launcher_rejects_non_loopback_url(tmp_path: Path) -> None:
    config_path = write_config(tmp_path, url="https://example.com:8188", data_dir=tmp_path)
    with pytest.raises(PipelineError, match="loopback only"):
        prepare_mcp_launch(config_path, npx_executable="npx")


def test_launcher_rejects_missing_data_directory(tmp_path: Path) -> None:
    config_path = write_config(tmp_path, url="http://127.0.0.1:8000", data_dir=tmp_path / "missing")
    with pytest.raises(PipelineError, match="data directory is missing"):
        prepare_mcp_launch(config_path, npx_executable="npx")


def test_launcher_builds_pinned_local_command_and_scrubs_tokens(tmp_path: Path) -> None:
    config_path = write_config(tmp_path, url="http://127.0.0.1:8000", data_dir=tmp_path)
    source_environment = {key: "secret" for key in BLOCKED_ENVIRONMENT_KEYS}

    context = prepare_mcp_launch(
        config_path,
        npx_executable="npx.cmd",
        base_environment=source_environment,
    )

    assert context.command == (
        "npx.cmd",
        "-y",
        MCP_PACKAGE,
        "--comfyui-url",
        "http://127.0.0.1:8000",
    )
    assert context.environment["COMFYUI_URL"] == "http://127.0.0.1:8000"
    assert context.environment["COMFYUI_PATH"] == str(tmp_path.resolve())
    assert {key: context.environment[key] for key in MCP_HARDENING_ENVIRONMENT} == MCP_HARDENING_ENVIRONMENT
    assert not set(BLOCKED_ENVIRONMENT_KEYS) & context.environment.keys()


def test_project_mcp_config_is_allowlisted_and_machine_independent() -> None:
    config_path = PROJECT_ROOT / ".codex" / "config.toml"
    raw = config_path.read_text(encoding="utf-8")
    config = tomllib.loads(raw)["mcp_servers"]["comfyui"]

    assert config["args"][-1] == "ArtPipeline/scripts/comfyui_mcp_launcher.py"
    assert config["default_tools_approval_mode"] == "writes"
    assert set(config["enabled_tools"]) == EXPECTED_ENABLED_TOOLS
    assert set(config["enabled_tools"]).isdisjoint(config["disabled_tools"])
    assert "E:\\" not in raw
    assert "token" not in raw.lower()
