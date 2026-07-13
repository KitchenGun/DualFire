from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

from PIL import Image

from .asset import load_asset, validate_state
from .image_ops import count_visible_colors
from .util import resolve_within, sha256_file


@dataclass(frozen=True)
class ValidationResult:
    errors: list[str]
    warnings: list[str]

    @property
    def ok(self) -> bool:
        return not self.errors


def validate_outputs(asset_dir: Path, state: str | None = None) -> ValidationResult:
    errors: list[str] = []
    warnings: list[str] = []
    try:
        asset = load_asset(asset_dir)
    except Exception as error:
        return ValidationResult([str(error)], [])
    states = [state] if state else list(asset["animations"])
    width = int(asset["target"]["cell_width"])
    height = int(asset["target"]["cell_height"])
    palette_limit = int(asset["target"]["palette_limit"])
    color_mode = str(asset["target"].get("color_mode", "indexed"))

    for current_state in states:
        try:
            validate_state(current_state)
        except Exception as error:
            errors.append(str(error))
            continue
        animation = asset["animations"].get(current_state)
        if not animation:
            errors.append(f"Unknown animation state: {current_state}")
            continue
        count = int(animation["frames"])
        bboxes: list[tuple[int, int]] = []
        loaded_frames: list[Image.Image] = []
        pixel_art = animation.get("pixel_art") if isinstance(animation.get("pixel_art"), dict) else {}
        background = pixel_art.get("background") if isinstance(pixel_art.get("background"), dict) else {}
        chroma_color = tuple(background.get("color", ())) if background.get("mode") == "chroma_key" else None
        for index in range(count):
            expected_name = f"{asset['asset_id']}_{current_state}_{index:03d}.png"
            frame_path = asset_dir / "output" / "frames" / expected_name
            if not frame_path.is_file():
                errors.append(f"Missing frame: {frame_path}")
                continue
            try:
                with Image.open(frame_path) as image:
                    rgba = image.convert("RGBA")
            except OSError as error:
                errors.append(f"Invalid frame {frame_path}: {error}")
                continue
            if rgba.mode != "RGBA" or rgba.size != (width, height):
                errors.append(f"Frame must be RGBA {width}x{height}: {frame_path}")
            loaded_frames.append(rgba)
            alpha = rgba.getchannel("A")
            if chroma_color:
                if alpha.getextrema() != (255, 255):
                    errors.append(f"Chroma-key frame alpha must be fully opaque: {frame_path}")
                subject_alpha = Image.new("L", rgba.size, 0)
                subject_alpha.putdata([255 if pixel[:3] != chroma_color else 0 for pixel in _pixels(rgba)])
            else:
                subject_alpha = alpha
            bbox = subject_alpha.getbbox()
            if not bbox:
                errors.append(f"Frame is empty: {frame_path}")
            else:
                bboxes.append((bbox[2] - bbox[0], bbox[3] - bbox[1]))
            color_count = count_visible_colors(rgba)
            if color_mode == "indexed" and color_count > palette_limit:
                errors.append(f"Frame exceeds {palette_limit} visible colors ({color_count}): {frame_path}")
            if any(value not in {0, 255} for value in alpha.tobytes()):
                errors.append(f"Frame alpha must be binary: {frame_path}")
            if asset.get("schema_version") == 2 and bbox and (bbox[0] < 2 or bbox[1] < 2 or bbox[2] > width - 2 or bbox[3] > height - 2):
                errors.append(f"Frame violates the two-pixel safety margin: {frame_path}")
            if asset.get("schema_version") == 2 and current_state.endswith("afterburner_on"):
                visible = {color[:3] for color in _pixels(rgba) if color[3] > 0}
                required = {tuple(color) for color in asset["target"].get("reserved_palette_colors", [])}
                if not required.issubset(visible):
                    errors.append(f"Afterburner frame is missing reserved flame colors: {frame_path}")
        if bboxes:
            widths = [value[0] for value in bboxes]
            heights = [value[1] for value in bboxes]
            if max(widths) - min(widths) > 2 or max(heights) - min(heights) > 2:
                warnings.append(f"Visible scale varies by more than 2 pixels: {current_state}")
        if (
            asset.get("schema_version") == 2
            and animation.get("recipe_version") in {"fighter_flight_v1", "fighter_flight_hd_v1", "source_preserving_pixel_v1"}
            and len(loaded_frames) == 2
        ):
            _validate_fighter_frame_delta(current_state, loaded_frames[0], loaded_frames[1], errors)

        sheet_path = asset_dir / "output" / "sheets" / f"{asset['asset_id']}_{current_state}_sheet.png"
        if not sheet_path.is_file():
            errors.append(f"Missing sheet: {sheet_path}")
        else:
            with Image.open(sheet_path) as sheet:
                if sheet.size != (width * count, height):
                    errors.append(f"Sheet has unexpected size: {sheet_path}: {sheet.size}")
        metadata_path = asset_dir / "output" / "metadata" / f"{asset['asset_id']}_{current_state}.json"
        if not metadata_path.is_file():
            errors.append(f"Missing metadata: {metadata_path}")
        else:
            try:
                metadata = json.loads(metadata_path.read_text(encoding="utf-8"))
                if metadata.get("asset_id") != asset["asset_id"] or metadata.get("state") != current_state:
                    errors.append(f"Metadata identity mismatch: {metadata_path}")
                if len(metadata.get("frames", [])) != count:
                    errors.append(f"Metadata frame count mismatch: {metadata_path}")
                if asset.get("schema_version") == 2:
                    _validate_v2_provenance(asset_dir, current_state, metadata, errors)
            except (OSError, json.JSONDecodeError) as error:
                errors.append(f"Invalid metadata {metadata_path}: {error}")
        _validate_anchors(asset, width, height, errors)
    if asset.get("schema_version") == 2 and state is None:
        _validate_fighter_flight_review(asset_dir, asset, errors)
    return ValidationResult(errors, warnings)


def _validate_anchors(asset: dict, width: int, height: int, errors: list[str]) -> None:
    for name in ("pivot", "hit_center", "muzzle"):
        value = asset["anchors"].get(name)
        if not isinstance(value, list) or len(value) != 2:
            errors.append(f"Anchor {name} must be [x, y]")
            continue
        x, y = int(value[0]), int(value[1])
        if not 0 <= x <= width or not 0 <= y <= height:
            errors.append(f"Anchor {name} is outside the frame: {value}")


def _validate_v2_provenance(asset_dir: Path, state: str, metadata: dict, errors: list[str]) -> None:
    approval_path = asset_dir / "work" / "approval.json"
    run_path = asset_dir / "work" / "run_manifest.json"
    build_path = asset_dir / "work" / "build_manifest.json"
    if not approval_path.is_file() or not run_path.is_file() or not build_path.is_file():
        errors.append(f"Missing schema v2 provenance manifests: {state}")
        return
    try:
        approval = json.loads(approval_path.read_text(encoding="utf-8")).get("states", {}).get(state)
        runs = json.loads(run_path.read_text(encoding="utf-8")).get("runs", [])
        build = json.loads(build_path.read_text(encoding="utf-8")).get("states", {}).get(state)
    except (OSError, json.JSONDecodeError) as error:
        errors.append(f"Invalid schema v2 provenance manifest: {error}")
        return
    if not isinstance(approval, dict) or not isinstance(build, dict):
        errors.append(f"Missing approval or build provenance: {state}")
        return
    try:
        candidate = resolve_within(asset_dir, approval["candidate"])
        canonical = resolve_within(asset_dir, approval["canonical_file"])
    except Exception as error:
        errors.append(str(error))
        return
    if not candidate.is_file() or sha256_file(candidate) != approval.get("candidate_sha256"):
        errors.append(f"Approved candidate hash mismatch: {state}")
    if not canonical.is_file() or sha256_file(canonical) != approval.get("canonical_sha256"):
        errors.append(f"Canonical keyframe hash mismatch: {state}")
    if not any(
        run.get("run_id") == approval.get("run_id")
        and run.get("state") == state
        and any(item.get("file") == approval.get("candidate") and item.get("sha256") == approval.get("candidate_sha256") for item in run.get("candidates", []))
        for run in runs
        if isinstance(run, dict)
    ):
        errors.append(f"Qwen run does not match approved candidate: {state}")
    provenance = metadata.get("provenance")
    if not isinstance(provenance, dict) or provenance.get("canonical_sha256") != approval.get("canonical_sha256"):
        errors.append(f"Output metadata canonical provenance mismatch: {state}")
    if build.get("canonical_sha256") != approval.get("canonical_sha256"):
        errors.append(f"Build manifest canonical provenance mismatch: {state}")


def _validate_fighter_frame_delta(state: str, first: Image.Image, second: Image.Image, errors: list[str]) -> None:
    if first.size != (64, 64):
        _validate_high_detail_fighter_frame_delta(state, first, second, errors)
        return
    allowed: set[tuple[int, int]] = set()
    for y in range(25, 49):
        for x in range(8, 56):
            allowed.add((x, y))
    for y in range(42, 57):
        for x in range(26, 39):
            allowed.add((x, y))
    if state.endswith("afterburner_on"):
        for y in range(52, 64):
            for x in range(23, 42):
                allowed.add((x, y))
    changed = [
        (index % first.width, index // first.width)
        for index, (before, after) in enumerate(zip(_pixels(first), _pixels(second)))
        if before != after
    ]
    if not changed:
        errors.append(f"Fighter animation has no control or exhaust motion: {state}")
    elif any(point not in allowed for point in changed):
        errors.append(f"Fighter animation changes pixels outside allowed motion regions: {state}")


def _validate_high_detail_fighter_frame_delta(state: str, first: Image.Image, second: Image.Image, errors: list[str]) -> None:
    width, height = first.size
    allowed: set[tuple[int, int]] = set()
    for y in range(round(height * 0.35), round(height * 0.93)):
        for x in range(round(width * 0.08), round(width * 0.92)):
            allowed.add((x, y))
    if state.endswith("afterburner_on"):
        for y in range(round(height * 0.84), height):
            for x in range(round(width * 0.28), round(width * 0.72)):
                allowed.add((x, y))
    changed = [
        (index % width, index // width)
        for index, (before, after) in enumerate(zip(_pixels(first), _pixels(second)))
        if before != after
    ]
    if not changed:
        errors.append(f"Fighter animation has no control or exhaust motion: {state}")
    elif any(point not in allowed for point in changed):
        errors.append(f"Fighter animation changes pixels outside allowed motion regions: {state}")


def _validate_fighter_flight_review(asset_dir: Path, asset: dict, errors: list[str]) -> None:
    states = list(asset["animations"])
    flight_recipes = {"fighter_flight_v1", "fighter_flight_hd_v1"}
    if any(asset["animations"][state].get("recipe_version") not in flight_recipes for state in states):
        return
    width = int(asset["target"]["cell_width"])
    height = int(asset["target"]["cell_height"])
    sheet = asset_dir / "output" / "sheets" / f"{asset['asset_id']}_flight_sheet.png"
    metadata = asset_dir / "output" / "metadata" / f"{asset['asset_id']}_flight.json"
    if not sheet.is_file() or not metadata.is_file():
        errors.append("Missing combined fighter flight review sheet or metadata")
        return
    with Image.open(sheet) as image:
        if image.convert("RGBA").size != (width * len(states), height * 2):
            errors.append(f"Combined fighter flight sheet has unexpected size: {sheet}")
    try:
        data = json.loads(metadata.read_text(encoding="utf-8"))
        if data.get("columns") != states or data.get("rows") != ["frame_000", "frame_001"]:
            errors.append(f"Combined fighter flight metadata ordering mismatch: {metadata}")
    except (OSError, json.JSONDecodeError) as error:
        errors.append(f"Invalid combined fighter flight metadata: {error}")


def _pixels(image: Image.Image):
    return image.get_flattened_data() if hasattr(image, "get_flattened_data") else image.getdata()
