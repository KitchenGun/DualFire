from __future__ import annotations

import json
from pathlib import Path
from typing import Any


SCHEMA_VERSION = "1.0"
CONFIDENCE_VALUES = {"observed", "inferred", "unknown"}
RISK_VALUES = {"low", "medium", "high"}
VERIFICATION_VALUES = {"passed", "failed", "not_run"}


def load_json(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as handle:
        data = json.load(handle)
    if not isinstance(data, dict):
        raise ValueError("report root must be a JSON object")
    return data


def _is_text(value: Any) -> bool:
    return isinstance(value, str) and bool(value.strip())


def validate_report(data: dict[str, Any]) -> list[str]:
    errors: list[str] = []

    if data.get("schema_version") != SCHEMA_VERSION:
        errors.append(f"schema_version must be {SCHEMA_VERSION!r}")

    mode = data.get("mode")
    if mode not in {"compact", "full"}:
        errors.append("mode must be 'compact' or 'full'")

    for key in ("title", "project", "generated_at"):
        if not _is_text(data.get(key)):
            errors.append(f"{key} must be a non-empty string")

    scope = data.get("scope")
    if not isinstance(scope, dict):
        errors.append("scope must be an object")
    else:
        for key in ("kind", "label"):
            if not _is_text(scope.get(key)):
                errors.append(f"scope.{key} must be a non-empty string")

    _validate_string_list(data, "executive_summary", errors, required=True)
    _validate_sections(data, "background", errors)
    _validate_sections(data, "intuition", errors)
    _validate_flows(data, errors)
    _validate_changes(data, errors)
    _validate_risks(data, errors)
    _validate_verification(data, errors)
    _validate_string_list(data, "limitations", errors, required=False)
    _validate_quiz(data, mode, errors)

    return errors


def _validate_string_list(
    data: dict[str, Any], key: str, errors: list[str], *, required: bool
) -> None:
    value = data.get(key)
    if not isinstance(value, list):
        errors.append(f"{key} must be an array")
        return
    if required and not value:
        errors.append(f"{key} must not be empty")
    for index, item in enumerate(value):
        if not _is_text(item):
            errors.append(f"{key}[{index}] must be a non-empty string")


def _validate_sections(data: dict[str, Any], key: str, errors: list[str]) -> None:
    value = data.get(key)
    if not isinstance(value, list):
        errors.append(f"{key} must be an array")
        return
    for index, item in enumerate(value):
        if not isinstance(item, dict):
            errors.append(f"{key}[{index}] must be an object")
            continue
        for field in ("title", "body"):
            if not _is_text(item.get(field)):
                errors.append(f"{key}[{index}].{field} must be a non-empty string")


def _validate_flows(data: dict[str, Any], errors: list[str]) -> None:
    value = data.get("flows")
    if not isinstance(value, list):
        errors.append("flows must be an array")
        return
    for index, item in enumerate(value):
        if not isinstance(item, dict):
            errors.append(f"flows[{index}] must be an object")
            continue
        if not _is_text(item.get("title")):
            errors.append(f"flows[{index}].title must be a non-empty string")
        steps = item.get("steps")
        if not isinstance(steps, list) or len(steps) < 2:
            errors.append(f"flows[{index}].steps must contain at least two steps")
        elif any(not _is_text(step) for step in steps):
            errors.append(f"flows[{index}].steps must contain only non-empty strings")


def _validate_changes(data: dict[str, Any], errors: list[str]) -> None:
    value = data.get("changes")
    if not isinstance(value, list) or not value:
        errors.append("changes must be a non-empty array")
        return
    for index, item in enumerate(value):
        if not isinstance(item, dict):
            errors.append(f"changes[{index}] must be an object")
            continue
        for field in ("title", "domain", "adapter", "summary"):
            if not _is_text(item.get(field)):
                errors.append(f"changes[{index}].{field} must be a non-empty string")
        confidence = item.get("confidence")
        if confidence not in CONFIDENCE_VALUES:
            errors.append(
                f"changes[{index}].confidence must be one of {sorted(CONFIDENCE_VALUES)}"
            )
        details = item.get("details")
        if not isinstance(details, list) or any(not _is_text(detail) for detail in details):
            errors.append(f"changes[{index}].details must be an array of non-empty strings")
        sources = item.get("sources")
        if not isinstance(sources, list):
            errors.append(f"changes[{index}].sources must be an array")
            continue
        if confidence == "observed" and not sources:
            errors.append(f"changes[{index}] is observed but has no source")
        for source_index, source in enumerate(sources):
            if not isinstance(source, dict) or not _is_text(source.get("path")):
                errors.append(
                    f"changes[{index}].sources[{source_index}].path must be a non-empty string"
                )
            elif "line" in source and (
                not isinstance(source["line"], int) or source["line"] < 1
            ):
                errors.append(
                    f"changes[{index}].sources[{source_index}].line must be a positive integer"
                )


def _validate_risks(data: dict[str, Any], errors: list[str]) -> None:
    value = data.get("risks")
    if not isinstance(value, list):
        errors.append("risks must be an array")
        return
    for index, item in enumerate(value):
        if not isinstance(item, dict):
            errors.append(f"risks[{index}] must be an object")
            continue
        if item.get("severity") not in RISK_VALUES:
            errors.append(f"risks[{index}].severity must be one of {sorted(RISK_VALUES)}")
        for field in ("title", "detail", "mitigation"):
            if not _is_text(item.get(field)):
                errors.append(f"risks[{index}].{field} must be a non-empty string")


def _validate_verification(data: dict[str, Any], errors: list[str]) -> None:
    value = data.get("verification")
    if not isinstance(value, list):
        errors.append("verification must be an array")
        return
    for index, item in enumerate(value):
        if not isinstance(item, dict):
            errors.append(f"verification[{index}] must be an object")
            continue
        if item.get("status") not in VERIFICATION_VALUES:
            errors.append(
                f"verification[{index}].status must be one of {sorted(VERIFICATION_VALUES)}"
            )
        for field in ("name", "evidence"):
            if not _is_text(item.get(field)):
                errors.append(f"verification[{index}].{field} must be a non-empty string")


def _validate_quiz(data: dict[str, Any], mode: Any, errors: list[str]) -> None:
    value = data.get("quiz")
    if not isinstance(value, list):
        errors.append("quiz must be an array")
        return
    if mode == "full" and not 3 <= len(value) <= 5:
        errors.append("a full report must contain three to five quiz questions")
    if mode == "compact" and len(value) > 3:
        errors.append("a compact report may contain at most three quiz questions")

    for index, item in enumerate(value):
        if not isinstance(item, dict):
            errors.append(f"quiz[{index}] must be an object")
            continue
        for field in ("question", "explanation"):
            if not _is_text(item.get(field)):
                errors.append(f"quiz[{index}].{field} must be a non-empty string")
        options = item.get("options")
        if not isinstance(options, list) or not 3 <= len(options) <= 4:
            errors.append(f"quiz[{index}].options must contain three or four choices")
            continue
        if any(not _is_text(option) for option in options):
            errors.append(f"quiz[{index}].options must contain only non-empty strings")
        answer = item.get("answer_index")
        if not isinstance(answer, int) or not 0 <= answer < len(options):
            errors.append(f"quiz[{index}].answer_index is outside the options array")


def assert_outside_repo(path: Path, repo_root: Path) -> None:
    resolved_path = path.resolve()
    resolved_root = repo_root.resolve()
    try:
        resolved_path.relative_to(resolved_root)
    except ValueError:
        return
    raise ValueError(f"output must be outside repository: {resolved_path}")
