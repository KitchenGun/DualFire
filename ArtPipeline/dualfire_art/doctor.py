from __future__ import annotations

from dataclasses import dataclass

import httpx

from .comfy import ComfyClient, REQUIRED_NODES, load_workflow, required_nodes_for_workflow, validate_workflow_nodes
from .config import PipelineConfig
from .util import sha256_file


@dataclass(frozen=True)
class CheckResult:
    name: str
    ok: bool
    detail: str


def run_doctor(config: PipelineConfig, transport: httpx.BaseTransport | None = None) -> list[CheckResult]:
    results: list[CheckResult] = []
    required_nodes = REQUIRED_NODES
    results.append(CheckResult("data_dir", config.comfy_data_dir.is_dir(), str(config.comfy_data_dir)))

    for name, path in config.model_paths.items():
        if path.is_file():
            results.append(CheckResult(f"model:{name}", True, f"{path} sha256={sha256_file(path)}"))
        else:
            results.append(CheckResult(f"model:{name}", False, f"missing: {path}"))

    try:
        workflow = load_workflow(config.workflow_file)
        required_nodes = required_nodes_for_workflow(workflow)
        missing = validate_workflow_nodes(workflow)
        results.append(
            CheckResult(
                "workflow",
                not missing,
                f"{config.workflow_file}" if not missing else f"missing node classes: {', '.join(missing)}",
            )
        )
    except Exception as error:
        results.append(CheckResult("workflow", False, str(error)))

    custom_nodes = config.comfy_data_dir / "custom_nodes"
    custom_entries = []
    if custom_nodes.is_dir():
        custom_entries = [path.name for path in custom_nodes.iterdir() if path.name not in {".gitkeep", "__pycache__"}]
    results.append(
        CheckResult(
            "custom_nodes",
            not custom_entries,
            "none" if not custom_entries else f"not allowed: {', '.join(sorted(custom_entries))}",
        )
    )

    try:
        with ComfyClient(config, transport=transport) as client:
            stats = client.system_stats()
            info = client.object_info()
        devices = stats.get("devices", [])
        gpu_detail = ", ".join(str(device.get("name", "unknown")) for device in devices) or "API reachable"
        results.append(CheckResult("comfy_api", True, gpu_detail))
        missing_nodes = sorted(required_nodes - set(info))
        results.append(
            CheckResult(
                "comfy_nodes",
                not missing_nodes,
                "all required nodes available" if not missing_nodes else f"missing: {', '.join(missing_nodes)}",
            )
        )
    except Exception as error:
        results.append(CheckResult("comfy_api", False, str(error)))
        results.append(CheckResult("comfy_nodes", False, "API unavailable"))
    return results


def doctor_succeeded(results: list[CheckResult]) -> bool:
    return all(result.ok for result in results)
