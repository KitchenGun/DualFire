from __future__ import annotations

import json
import shutil
import uuid
from datetime import datetime, timezone
from pathlib import Path
from typing import Any

from PIL import Image

from .asset import load_asset, save_asset, validate_state
from .comfy import ComfyClient, load_workflow, render_workflow
from .config import PipelineConfig
from .errors import PipelineError
from .image_ops import (
    canonicalize_qwen_candidate,
    composite_rgba_on_rgb_background,
    composite_with_mask,
    load_mask,
    load_rgba,
    offset_frame,
    pixelize,
    prepare_source,
    quantize_limited_palette,
    resize_nearest,
    stylize_source_preserving_pixel_art,
    threshold_to_binary_mask,
    validate_input_file,
)
from .fighter_sprite import build_flight_keyframe_frame
from .util import (
    require_new_path,
    resolve_within,
    sha256_bytes,
    sha256_file,
    write_json,
)


def prepare_asset(asset_dir: Path, force: bool = False) -> dict[str, str]:
    asset = load_asset(asset_dir)
    hashes = prepare_source(asset_dir, asset, force=force)
    asset["source"]["sha256"] = hashes["source_sha256"]
    save_asset(asset_dir, asset)
    return hashes


def extract_birefnet_mask(
    asset_dir: Path,
    config: PipelineConfig,
    threshold: float = 0.5,
    apply_source_mask: bool = False,
    force: bool = False,
) -> Path:
    """Extract a reusable source mask through ComfyUI's local BiRefNet model."""
    if not 0.0 <= threshold <= 1.0:
        raise PipelineError("BiRefNet threshold must be between 0.0 and 1.0")
    asset_dir = asset_dir.resolve()
    asset = load_asset(asset_dir)
    source_path = resolve_within(asset_dir, asset["source"]["file"])
    validate_input_file(source_path)
    output_path = asset_dir / "work" / "masks" / "birefnet.png"
    require_new_path(output_path, force)

    with ComfyClient(config) as client:
        nodes = client.object_info()
        required_nodes = {"LoadImage", "LoadBackgroundRemovalModel", "RemoveBackground", "ThresholdMask", "MaskToImage", "SaveImage"}
        missing_nodes = sorted(node for node in required_nodes if node not in nodes)
        if missing_nodes:
            raise PipelineError(f"ComfyUI is missing BiRefNet nodes: {', '.join(missing_nodes)}")
        options = nodes["LoadBackgroundRemovalModel"].get("input", {}).get("required", {}).get("bg_removal_name", [None, {}])[1]
        model_names = options.get("options", []) if isinstance(options, dict) else []
        # Some ComfyUI builds expose an empty model-option list before the first load.
        # Let the runtime validate that case instead of rejecting an installed weight.
        if model_names and "birefnet.safetensors" not in model_names:
            raise PipelineError("Missing local BiRefNet weight: models/background_removal/birefnet.safetensors")

        remote_input = client.upload_image(source_path, f"{asset['asset_id']}_{sha256_file(source_path)[:12]}.png")
        workflow = {
            "1": {"class_type": "LoadImage", "inputs": {"image": remote_input}},
            "2": {"class_type": "LoadBackgroundRemovalModel", "inputs": {"bg_removal_name": "birefnet.safetensors"}},
            "3": {"class_type": "RemoveBackground", "inputs": {"bg_removal_model": ["2", 0], "image": ["1", 0]}},
            "4": {"class_type": "ThresholdMask", "inputs": {"mask": ["3", 0], "value": threshold}},
            "5": {"class_type": "MaskToImage", "inputs": {"mask": ["4", 0]}},
            "6": {"class_type": "SaveImage", "inputs": {"images": ["5", 0], "filename_prefix": f"dualfire/{asset['asset_id']}/masks/birefnet"}},
        }
        prompt_id = client.submit(workflow)
        images = client.wait_for_images(prompt_id)
        if not images:
            raise PipelineError("BiRefNet did not return a mask image")
        output_path.parent.mkdir(parents=True, exist_ok=True)
        output_path.write_bytes(client.download_image(images[0]))

    mask = load_mask(output_path, load_rgba(source_path).size)
    mask.save(output_path, format="PNG", optimize=False)
    manifest_path = asset_dir / "work" / "masks" / "birefnet.json"
    write_json(
        manifest_path,
        {
            "source": source_path.relative_to(asset_dir).as_posix(),
            "source_sha256": sha256_file(source_path),
            "mask": output_path.relative_to(asset_dir).as_posix(),
            "mask_sha256": sha256_file(output_path),
            "threshold": threshold,
        },
    )
    if apply_source_mask:
        asset["source"]["mask_file"] = output_path.relative_to(asset_dir).as_posix()
        asset["source"]["chroma_key"] = None
        save_asset(asset_dir, asset)
    return output_path


def submit_comfy(
    asset_dir: Path,
    state: str,
    config: PipelineConfig,
    candidates: int | None = None,
    force: bool = False,
    prompt_file: Path | None = None,
) -> dict[str, Any]:
    validate_state(state)
    asset = load_asset(asset_dir)
    if state not in asset["animations"]:
        raise PipelineError(f"Unknown animation state: {state}")
    input_path, input_state = _resolve_submission_input(asset_dir, asset, state)
    for name, path in config.model_paths.items():
        if not path.is_file():
            raise PipelineError(f"Missing {name} model: {path}")

    qwen = animation_qwen_config(asset, state)
    count = candidates if candidates is not None else int(qwen.get("candidates", config.generation.candidates))
    if not 1 <= count <= 16:
        raise PipelineError("Candidate count must be between 1 and 16")
    candidate_dir = asset_dir / "work" / "qwen" / state
    existing = list(candidate_dir.glob("candidate_*.png")) if candidate_dir.exists() else []
    if existing and not force:
        raise PipelineError(f"Candidates already exist for {state}; use --force")
    if force and candidate_dir.exists():
        shutil.rmtree(candidate_dir)
    candidate_dir.mkdir(parents=True, exist_ok=True)
    _ensure_run_slot(asset_dir, state, force)

    prompt_path = _resolve_prompt_path(asset_dir, asset, state, prompt_file)
    prompt = prompt_path.read_text(encoding="utf-8").strip()
    workflow_template = load_workflow(config.workflow_file)
    workflow_hash = sha256_file(config.workflow_file)
    uploaded_name = f"{asset['asset_id']}_{sha256_file(input_path)[:12]}.png"
    run: dict[str, Any] = {
        "run_id": str(uuid.uuid4()),
        "state": state,
        "created_at": _utc_now(),
        "input": {
            "file": input_path.relative_to(asset_dir).as_posix(),
            "sha256": sha256_file(input_path),
            "input_state": input_state,
        },
        "workflow_sha256": workflow_hash,
        "prompt_sha256": sha256_bytes(prompt.encode("utf-8")),
        "models": {name: {"file": path.name, "sha256": sha256_file(path)} for name, path in config.model_paths.items()},
        "generation": {
            "steps": config.generation.steps,
            "cfg": config.generation.cfg,
            "sampler": config.generation.sampler,
            "scheduler": config.generation.scheduler,
            "denoise": config.generation.denoise,
        },
        "candidates": [],
    }

    with ComfyClient(config) as client:
        remote_input = client.upload_image(input_path, uploaded_name)
        for index in range(count):
            seed = config.generation.seed_base + index
            output_prefix = f"dualfire/{asset['asset_id']}/{state}_{index:03d}"
            workflow = render_workflow(
                workflow_template,
                config,
                remote_input,
                prompt,
                seed,
                output_prefix,
            )
            prompt_id = client.submit(workflow)
            outputs = client.wait_for_images(prompt_id)
            if not outputs:
                raise PipelineError(f"ComfyUI returned no image for candidate {index}")
            payload = client.download_image(outputs[0])
            candidate_path = candidate_dir / f"candidate_{index:03d}.png"
            candidate_path.write_bytes(payload)
            try:
                with Image.open(candidate_path) as image:
                    image.verify()
            except OSError as error:
                candidate_path.unlink(missing_ok=True)
                raise PipelineError(f"ComfyUI returned an invalid image for candidate {index}") from error
            run["candidates"].append(
                {
                    "index": index,
                    "seed": seed,
                    "prompt_id": prompt_id,
                    "file": candidate_path.relative_to(asset_dir).as_posix(),
                    "sha256": sha256_file(candidate_path),
                }
            )
    _record_run(asset_dir, run, force=force)
    return run


def collect_comfy_result(
    asset_dir: Path,
    state: str,
    prompt_id: str,
    config: PipelineConfig,
    candidate_index: int = 0,
    force: bool = False,
) -> dict[str, Any]:
    """Recover a completed ComfyUI result after the calling process stopped polling."""
    validate_state(state)
    if not 0 <= candidate_index <= 15:
        raise PipelineError("Candidate index must be between 0 and 15")
    asset_dir = asset_dir.resolve()
    asset = load_asset(asset_dir)
    if state not in asset["animations"]:
        raise PipelineError(f"Unknown animation state: {state}")
    input_path, input_state = _resolve_submission_input(asset_dir, asset, state)
    prompt_path = _resolve_prompt_path(asset_dir, asset, state, None)
    prompt = prompt_path.read_text(encoding="utf-8").strip()
    candidate_path = asset_dir / "work" / "qwen" / state / f"candidate_{candidate_index:03d}.png"
    require_new_path(candidate_path, force)
    _ensure_run_slot(asset_dir, state, force)

    with ComfyClient(config) as client:
        history = client.prompt_history(prompt_id)
        prompt_history = history.get(prompt_id)
        if not isinstance(prompt_history, dict):
            raise PipelineError(f"ComfyUI has no history for prompt: {prompt_id}")
        if prompt_history.get("status", {}).get("status_str") != "success":
            raise PipelineError(f"ComfyUI prompt is not complete: {prompt_id}")
        outputs = client.wait_for_images(prompt_id)
        if not outputs:
            raise PipelineError(f"ComfyUI returned no image for prompt: {prompt_id}")
        candidate_path.parent.mkdir(parents=True, exist_ok=True)
        candidate_path.write_bytes(client.download_image(outputs[0]))

    try:
        with Image.open(candidate_path) as image:
            image.verify()
    except OSError as error:
        candidate_path.unlink(missing_ok=True)
        raise PipelineError(f"ComfyUI returned an invalid image for prompt: {prompt_id}") from error

    sampler_inputs = _history_sampler_inputs(prompt_history)
    run = {
        "run_id": str(uuid.uuid4()),
        "state": state,
        "created_at": _utc_now(),
        "recovered_prompt_id": prompt_id,
        "input": {
            "file": input_path.relative_to(asset_dir).as_posix(),
            "sha256": sha256_file(input_path),
            "input_state": input_state,
        },
        "workflow_sha256": sha256_file(config.workflow_file),
        "prompt_sha256": sha256_bytes(prompt.encode("utf-8")),
        "models": {name: {"file": path.name, "sha256": sha256_file(path)} for name, path in config.model_paths.items()},
        "generation": {
            "steps": sampler_inputs["steps"],
            "cfg": sampler_inputs["cfg"],
            "sampler": sampler_inputs["sampler_name"],
            "scheduler": sampler_inputs["scheduler"],
            "denoise": sampler_inputs["denoise"],
        },
        "candidates": [
            {
                "index": candidate_index,
                "seed": sampler_inputs["seed"],
                "prompt_id": prompt_id,
                "file": candidate_path.relative_to(asset_dir).as_posix(),
                "sha256": sha256_file(candidate_path),
            }
        ],
    }
    _record_run(asset_dir, run, force=force)
    return run


def approve_candidate(
    asset_dir: Path,
    state: str,
    candidate_index: int,
    mask_file: str | None = None,
    force: bool = False,
) -> Path:
    validate_state(state)
    asset = load_asset(asset_dir)
    if state not in asset["animations"]:
        raise PipelineError(f"Unknown animation state: {state}")
    candidate = asset_dir / "work" / "qwen" / state / f"candidate_{candidate_index:03d}.png"
    if not candidate.is_file():
        raise PipelineError(f"Missing candidate: {candidate}")
    approved = asset_dir / "work" / "approved" / f"{state}.png"
    approved.parent.mkdir(parents=True, exist_ok=True)
    if approved.exists() and not force:
        raise PipelineError(f"Approved image already exists: {approved}")

    if asset.get("schema_version") == 2:
        return _approve_qwen_v2_candidate(asset_dir, asset, state, candidate, approved, mask_file, force)

    mask_hash = None
    if mask_file:
        if not approved.is_file():
            raise PipelineError("Masked approval requires an existing approved image")
        base = load_rgba(approved)
        mask_path = resolve_within(asset_dir, mask_file)
        mask = load_mask(mask_path, base.size)
        result = composite_with_mask(base, load_rgba(candidate), mask)
        result.save(approved, format="PNG", optimize=False)
        mask_hash = sha256_file(mask_path)
    else:
        require_new_path(approved, force)
        shutil.copyfile(candidate, approved)

    approval_path = asset_dir / "work" / "approval.json"
    approvals = {"schema_version": 1, "states": {}}
    if approval_path.is_file():
        approvals = json.loads(approval_path.read_text(encoding="utf-8"))
    approvals.setdefault("states", {})[state] = {
        "candidate": candidate.relative_to(asset_dir).as_posix(),
        "candidate_sha256": sha256_file(candidate),
        "approved_sha256": sha256_file(approved),
        "mask_sha256": mask_hash,
        "approved_at": _utc_now(),
    }
    write_json(approval_path, approvals)
    return approved


def import_keyframe(
    asset_dir: Path,
    state: str,
    source_file: Path,
    mask_file: str | None = None,
    force: bool = False,
) -> Path:
    """Register a user-approved keyframe without claiming that Qwen generated it."""
    validate_state(state)
    asset = load_asset(asset_dir)
    if asset.get("schema_version") != 2:
        raise PipelineError("import-keyframe requires schema_version 2")
    if state not in asset["animations"]:
        raise PipelineError(f"Unknown animation state: {state}")

    source_path = source_file.resolve()
    validate_input_file(source_path)
    candidate = asset_dir / "work" / "imported" / state / "keyframe_000.png"
    candidate.parent.mkdir(parents=True, exist_ok=True)
    require_new_path(candidate, force)
    shutil.copyfile(source_path, candidate)

    run = {
        "run_id": str(uuid.uuid4()),
        "state": state,
        "run_type": "imported_keyframe",
        "created_at": _utc_now(),
        "input": {
            "file_name": source_path.name,
            "sha256": sha256_file(source_path),
            "input_state": "external",
        },
        "candidates": [
            {
                "index": 0,
                "file": candidate.relative_to(asset_dir).as_posix(),
                "sha256": sha256_file(candidate),
            }
        ],
    }
    _record_run(asset_dir, run, force=force)

    approved = asset_dir / "work" / "approved" / f"{state}.png"
    approved.parent.mkdir(parents=True, exist_ok=True)
    if approved.exists() and not force:
        raise PipelineError(f"Approved image already exists: {approved}")
    return _approve_qwen_v2_candidate(asset_dir, asset, state, candidate, approved, mask_file, force)


def build_frames(asset_dir: Path, state: str, force: bool = False) -> list[Path]:
    validate_state(state)
    asset = load_asset(asset_dir)
    animation = asset["animations"].get(state)
    if not animation:
        raise PipelineError(f"Unknown animation state: {state}")
    target = asset["target"]
    size = (int(target["cell_width"]), int(target["cell_height"]))
    if asset.get("schema_version") == 2:
        base = _load_canonical_keyframe(asset_dir, asset, state)
        recipe_version = str(animation.get("recipe_version"))
        if recipe_version not in {
            "fighter_flight_v1",
            "fighter_flight_hd_v1",
            "source_preserving_pixel_v1",
            "static_keyframe_hd_v1",
            "pixel_sprite_static_v1",
        }:
            raise PipelineError(f"Unsupported schema v2 recipe_version: {recipe_version}")
        if recipe_version == "fighter_flight_v1" and size != (64, 64):
            raise PipelineError("fighter_flight_v1 requires 64x64 frames")
        if recipe_version in {"fighter_flight_hd_v1", "source_preserving_pixel_v1", "static_keyframe_hd_v1", "pixel_sprite_static_v1"} and target.get("color_mode") != "preserve":
            raise PipelineError(f"{recipe_version} requires target.color_mode: preserve")
        if recipe_version not in {"static_keyframe_hd_v1", "pixel_sprite_static_v1"} and int(animation["frames"]) != 2:
            raise PipelineError(f"{recipe_version} requires two frames")
        if recipe_version in {"static_keyframe_hd_v1", "pixel_sprite_static_v1"} and int(animation["frames"]) != 1:
            raise PipelineError(f"{recipe_version} requires one frame")
        if recipe_version == "static_keyframe_hd_v1":
            if "pixel_art" in animation:
                pixel_scale, palette_limit = _source_preserving_pixel_settings(animation, target)
                base = stylize_source_preserving_pixel_art(base, pixel_scale=pixel_scale, palette_limit=palette_limit)
            return _write_frames(asset_dir, asset, state, [base], force, recipe_version)
        if recipe_version == "pixel_sprite_static_v1":
            return _write_frames(asset_dir, asset, state, [_build_static_pixel_sprite(base, animation)], force, recipe_version)
        if recipe_version == "source_preserving_pixel_v1":
            pixel_scale, palette_limit = _source_preserving_pixel_settings(animation, target)
            base = stylize_source_preserving_pixel_art(base, pixel_scale=pixel_scale, palette_limit=palette_limit)
        bases = [build_flight_keyframe_frame(base, state, index) for index in range(2)]
        return _write_frames(asset_dir, asset, state, bases, force, recipe_version)

    normalized_mask_path = asset_dir / "work" / "normalized" / "mask.png"
    if not normalized_mask_path.is_file():
        raise PipelineError("Run prepare before build-frames")
    with Image.open(normalized_mask_path) as mask_image:
        normalized_mask = mask_image.convert("L")

    source_frames = animation.get("source_frames") or []
    count = int(animation["frames"])
    if source_frames:
        if len(source_frames) != count:
            raise PipelineError(f"source_frames count must match frames for {state}")
        bases = []
        for value in source_frames:
            source = load_rgba(resolve_within(asset_dir, value))
            source_alpha = source.getchannel("A")
            if source_alpha.getextrema() == (255, 255):
                source_alpha = normalized_mask.resize(source.size, Image.Resampling.NEAREST)
            bases.append(_pixelize_source(source, source_alpha, asset))
    else:
        approved = asset_dir / "work" / "approved" / f"{state}.png"
        if not approved.is_file():
            raise PipelineError(f"Approve a candidate before build-frames: {state}")
        source = load_rgba(approved)
        base = _pixelize_source(source, normalized_mask, asset)
        offsets = _resolve_offsets(animation, count)
        bases = [offset_frame(base, offset) for offset in offsets]

    return _write_frames(asset_dir, asset, state, bases, force)


def pack_frames(asset_dir: Path, state: str, force: bool = False) -> dict[str, Path]:
    validate_state(state)
    asset = load_asset(asset_dir)
    animation = asset["animations"].get(state)
    if not animation:
        raise PipelineError(f"Unknown animation state: {state}")
    count = int(animation["frames"])
    width = int(asset["target"]["cell_width"])
    height = int(asset["target"]["cell_height"])
    frame_paths = [asset_dir / "output" / "frames" / f"{asset['asset_id']}_{state}_{index:03d}.png" for index in range(count)]
    missing = [str(path) for path in frame_paths if not path.is_file()]
    if missing:
        raise PipelineError(f"Missing frames: {', '.join(missing)}")

    sheet_path = asset_dir / "output" / "sheets" / f"{asset['asset_id']}_{state}_sheet.png"
    metadata_path = asset_dir / "output" / "metadata" / f"{asset['asset_id']}_{state}.json"
    preview_path = asset_dir / "output" / "preview" / f"{asset['asset_id']}_{state}_preview.png"
    for path in (sheet_path, metadata_path, preview_path):
        require_new_path(path, force)
        path.parent.mkdir(parents=True, exist_ok=True)

    sheet = Image.new("RGBA", (width * count, height), (0, 0, 0, 0))
    for index, path in enumerate(frame_paths):
        frame = load_rgba(path)
        if frame.size != (width, height):
            raise PipelineError(f"Frame has unexpected size: {path}: {frame.size}")
        sheet.alpha_composite(frame, (index * width, 0))
    sheet.save(sheet_path, format="PNG", optimize=False)
    preview = sheet.resize((sheet.width * 8, sheet.height * 8), Image.Resampling.NEAREST)
    preview.save(preview_path, format="PNG", optimize=False)

    metadata = {
        "schema_version": 2 if asset.get("schema_version") == 2 else 1,
        "asset_id": asset["asset_id"],
        "state": state,
        "frame_width": width,
        "frame_height": height,
        "fps": float(animation["fps"]),
        "loop": bool(animation.get("loop", True)),
        "frames": [path.name for path in frame_paths],
        "sheet": {
            "file": sheet_path.name,
            "rows": 1,
            "columns": count,
            "padding": 0,
        },
        "coordinate_origin": asset["anchors"]["coordinate_origin"],
        "pivot": asset["anchors"]["pivot"],
        "hit_center": asset["anchors"]["hit_center"],
        "anchors": {"muzzle": asset["anchors"]["muzzle"]},
        "hashes": {
            "frames": {path.name: sha256_file(path) for path in frame_paths},
            "sheet": sha256_file(sheet_path),
        },
    }
    if asset.get("schema_version") == 2:
        approval = _get_approval(asset_dir, state)
        canonical = asset_dir / str(approval["canonical_file"])
        metadata["provenance"] = {
            "canonical_file": canonical.relative_to(asset_dir).as_posix(),
            "canonical_sha256": sha256_file(canonical),
            "candidate_sha256": approval["candidate_sha256"],
            "recipe_version": animation["recipe_version"],
        }
    if isinstance(animation.get("pixel_art"), dict):
        metadata["pixel_art"] = animation["pixel_art"]
    write_json(metadata_path, metadata)
    if asset.get("schema_version") == 2:
        _maybe_pack_fighter_flight_review(asset_dir, asset, force)
    return {"sheet": sheet_path, "metadata": metadata_path, "preview": preview_path}


def _pixelize_source(source: Image.Image, mask: Image.Image, asset: dict[str, Any]) -> Image.Image:
    target = asset["target"]
    return pixelize(
        source,
        mask,
        (int(target["cell_width"]), int(target["cell_height"])),
        int(target["palette_limit"]),
        int(target["alpha_threshold"]),
        [tuple(color) for color in target.get("reserved_palette_colors", [])],
    )


def _resolve_offsets(animation: dict[str, Any], count: int) -> list[tuple[int, int]]:
    recipe = animation.get("recipe") or {}
    if recipe.get("type", "offsets") != "offsets":
        raise PipelineError(f"Unsupported animation recipe: {recipe.get('type')}")
    values = recipe.get("offsets") or [[0, 0] for _ in range(count)]
    if len(values) != count:
        raise PipelineError("Animation offsets count must match frames")
    offsets: list[tuple[int, int]] = []
    for value in values:
        if not isinstance(value, list) or len(value) != 2:
            raise PipelineError("Each animation offset must be [x, y]")
        offsets.append((int(value[0]), int(value[1])))
    return offsets


def _source_preserving_pixel_settings(animation: dict[str, Any], target: dict[str, Any]) -> tuple[int, int]:
    settings = animation.get("pixel_art", {})
    if not isinstance(settings, dict):
        raise PipelineError("source_preserving_pixel_v1 pixel_art must be a mapping")
    pixel_scale = settings.get("pixel_scale", 2)
    palette_limit = settings.get("palette_limit", min(int(target["palette_limit"]), 64))
    if not isinstance(pixel_scale, int) or not 1 <= pixel_scale <= 16:
        raise PipelineError("source_preserving_pixel_v1 pixel_art.pixel_scale must be between 1 and 16")
    if not isinstance(palette_limit, int) or not 2 <= palette_limit <= int(target["palette_limit"]):
        raise PipelineError("source_preserving_pixel_v1 pixel_art.palette_limit exceeds target.palette_limit")
    return pixel_scale, palette_limit


def _build_static_pixel_sprite(base: Image.Image, animation: dict[str, Any]) -> Image.Image:
    settings = animation.get("pixel_art")
    if not isinstance(settings, dict):
        raise PipelineError("pixel_sprite_static_v1 requires pixel_art settings")
    working_size = (int(settings["working_width"]), int(settings["working_height"]))
    palette_limit = int(settings["palette_limit"])
    alpha_threshold = int(settings.get("alpha_threshold", 128))
    source = base.convert("RGBA")
    alpha = threshold_to_binary_mask(source.getchannel("A"), alpha_threshold)
    reduced_rgb = source.convert("RGB").resize(working_size, Image.Resampling.BOX)
    reduced = quantize_limited_palette(reduced_rgb, palette_limit).convert("RGBA")
    reduced.putalpha(threshold_to_binary_mask(resize_nearest(alpha, working_size), alpha_threshold))
    sprite = resize_nearest(reduced, source.size)
    sprite.putalpha(threshold_to_binary_mask(sprite.getchannel("A"), alpha_threshold))

    background = settings.get("background", {"mode": "transparent"})
    if not isinstance(background, dict) or background.get("mode", "transparent") == "transparent":
        return sprite
    if background.get("mode") != "chroma_key":
        raise PipelineError("Unsupported pixel_art background mode")
    color = background.get("color")
    if not isinstance(color, list) or len(color) != 3:
        raise PipelineError("chroma_key background requires an RGB color")
    canvas = Image.new("RGB", source.size, tuple(int(channel) for channel in color))
    return composite_rgba_on_rgb_background(sprite, canvas).convert("RGBA")


def _record_run(asset_dir: Path, run: dict[str, Any], force: bool) -> None:
    path = asset_dir / "work" / "run_manifest.json"
    manifest: dict[str, Any] = {"schema_version": 1, "runs": []}
    if path.is_file():
        manifest = json.loads(path.read_text(encoding="utf-8"))
    existing = [item for item in manifest.get("runs", []) if item.get("run_id") == run["run_id"]]
    if existing and not force:
        raise PipelineError(f"Run manifest already contains run {run['run_id']}; use --force")
    # A re-run is a new candidate set, not a replacement for an already approved run.
    # Approval provenance must remain verifiable until a new candidate is approved.
    manifest["runs"] = [item for item in manifest.get("runs", []) if item.get("run_id") != run["run_id"]]
    manifest["runs"].append(run)
    write_json(path, manifest)


def _ensure_run_slot(asset_dir: Path, state: str, force: bool) -> None:
    path = asset_dir / "work" / "run_manifest.json"
    if not path.is_file():
        return
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if any(item.get("state") == state for item in manifest.get("runs", [])) and not force:
        raise PipelineError(f"Run manifest already contains state {state}; use --force")


def _history_sampler_inputs(prompt_history: dict[str, Any]) -> dict[str, Any]:
    try:
        workflow = prompt_history["prompt"][2]
        inputs = workflow["11"]["inputs"]
        return {
            "seed": int(inputs["seed"]),
            "steps": int(inputs["steps"]),
            "cfg": float(inputs["cfg"]),
            "sampler_name": str(inputs["sampler_name"]),
            "scheduler": str(inputs["scheduler"]),
            "denoise": float(inputs["denoise"]),
        }
    except (IndexError, KeyError, TypeError, ValueError) as error:
        raise PipelineError("ComfyUI history does not contain a valid KSampler record") from error


def animation_qwen_config(asset: dict[str, Any], state: str) -> dict[str, Any]:
    if asset.get("schema_version") != 2:
        return {}
    qwen = asset["animations"][state].get("qwen")
    if not isinstance(qwen, dict):
        raise PipelineError(f"schema_version 2 requires qwen config: {state}")
    return qwen


def _resolve_submission_input(asset_dir: Path, asset: dict[str, Any], state: str) -> tuple[Path, str]:
    if asset.get("schema_version") != 2:
        source = asset_dir / "work" / "normalized" / "source.png"
        if not source.is_file():
            raise PipelineError("Run prepare before submit-comfy")
        return source, "source"
    input_state = str(animation_qwen_config(asset, state)["input_state"])
    if input_state == "source":
        fitted_source = asset_dir / "work" / "normalized" / "fitted.png"
        source = fitted_source if fitted_source.is_file() else asset_dir / "work" / "normalized" / "source.png"
        if not source.is_file():
            raise PipelineError("Run prepare before submit-comfy")
        return source, input_state
    canonical = asset_dir / "work" / "canonical" / f"{input_state}.png"
    if not canonical.is_file():
        raise PipelineError(f"Approve and canonicalize input state before submit-comfy: {input_state}")
    _validate_canonical_hash(asset_dir, input_state, canonical)
    return canonical, input_state


def _find_matching_normalized_mask(asset_dir: Path, candidate: Path) -> str | None:
    candidate_size = load_rgba(candidate).size
    for relative in ("work/normalized/fitted_mask.png", "work/normalized/mask.png"):
        path = asset_dir / relative
        if not path.is_file():
            continue
        with Image.open(path) as image:
            if image.size == candidate_size:
                return relative
    return None


def _resolve_prompt_path(asset_dir: Path, asset: dict[str, Any], state: str, prompt_file: Path | None) -> Path:
    if prompt_file is not None:
        if not prompt_file.is_file():
            raise PipelineError(f"Missing prompt file: {prompt_file}")
        return prompt_file
    if asset.get("schema_version") == 2:
        return resolve_within(asset_dir, str(animation_qwen_config(asset, state)["prompt_file"]))
    return Path(__file__).resolve().parents[1] / "templates" / "prompts" / "image_to_pixel.txt"


def _approve_qwen_v2_candidate(
    asset_dir: Path,
    asset: dict[str, Any],
    state: str,
    candidate: Path,
    approved: Path,
    mask_file: str | None,
    force: bool,
) -> Path:
    require_new_path(approved, force)
    run = _find_candidate_run(asset_dir, state, candidate)
    shutil.copyfile(candidate, approved)

    effective_mask_file = mask_file or _find_matching_normalized_mask(asset_dir, candidate)
    mask = None
    if effective_mask_file:
        mask = load_mask(resolve_within(asset_dir, effective_mask_file), load_rgba(candidate).size)
    canonical_config = asset["canonical"]
    target = asset["target"]
    canonical = canonicalize_qwen_candidate(
        load_rgba(candidate),
        (int(target["cell_width"]), int(target["cell_height"])),
        canonical_config.get("chroma_key"),
        mask,
        int(canonical_config["frame_fit"]["padding"]),
        int(target["palette_limit"]),
        int(target["alpha_threshold"]),
        [tuple(color) for color in target.get("reserved_palette_colors", [])],
        str(target.get("color_mode", "indexed")),
        canonical_config.get("canvas_mode", "fit") == "preserve",
    )
    canonical_path = asset_dir / "work" / "canonical" / f"{state}.png"
    canonical_path.parent.mkdir(parents=True, exist_ok=True)
    require_new_path(canonical_path, force)
    canonical.save(canonical_path, format="PNG", optimize=False)

    approval_path = asset_dir / "work" / "approval.json"
    approvals = _load_approvals(approval_path)
    approvals["states"][state] = {
        "candidate": candidate.relative_to(asset_dir).as_posix(),
        "candidate_sha256": sha256_file(candidate),
        "approved_file": approved.relative_to(asset_dir).as_posix(),
        "approved_sha256": sha256_file(approved),
        "canonical_file": canonical_path.relative_to(asset_dir).as_posix(),
        "canonical_sha256": sha256_file(canonical_path),
        "mask_file": effective_mask_file,
        "mask_sha256": sha256_file(resolve_within(asset_dir, effective_mask_file)) if effective_mask_file else None,
        "approved_at": _utc_now(),
        "run_id": run["run_id"],
        "input_state": animation_qwen_config(asset, state)["input_state"],
        "recipe_version": asset["animations"][state]["recipe_version"],
    }
    write_json(approval_path, approvals)
    return canonical_path


def _load_canonical_keyframe(asset_dir: Path, asset: dict[str, Any], state: str) -> Image.Image:
    approval = _get_approval(asset_dir, state)
    canonical = asset_dir / str(approval["canonical_file"])
    if not canonical.is_file():
        raise PipelineError(f"Missing canonical keyframe: {canonical}")
    _validate_canonical_hash(asset_dir, state, canonical)
    image = load_rgba(canonical)
    target = asset["target"]
    expected_size = (int(target["cell_width"]), int(target["cell_height"]))
    if image.size != expected_size:
        raise PipelineError(f"Canonical keyframe has unexpected size: {canonical}: {image.size}")
    return image


def _write_frames(
    asset_dir: Path,
    asset: dict[str, Any],
    state: str,
    bases: list[Image.Image],
    force: bool,
    recipe_version: str | None = None,
) -> list[Path]:
    frame_dir = asset_dir / "output" / "frames"
    frame_dir.mkdir(parents=True, exist_ok=True)
    paths: list[Path] = []
    for index, frame in enumerate(bases):
        path = frame_dir / f"{asset['asset_id']}_{state}_{index:03d}.png"
        require_new_path(path, force)
        frame.save(path, format="PNG", optimize=False)
        paths.append(path)
    if recipe_version:
        _record_build(asset_dir, asset, state, recipe_version, paths)
    return paths


def _record_build(asset_dir: Path, asset: dict[str, Any], state: str, recipe_version: str, frames: list[Path]) -> None:
    path = asset_dir / "work" / "build_manifest.json"
    manifest: dict[str, Any] = {"schema_version": 1, "states": {}}
    if path.is_file():
        manifest = json.loads(path.read_text(encoding="utf-8"))
    approval = _get_approval(asset_dir, state)
    manifest.setdefault("states", {})[state] = {
        "recipe_version": recipe_version,
        "canonical_file": approval["canonical_file"],
        "canonical_sha256": approval["canonical_sha256"],
        "frames": {
            frame.relative_to(asset_dir).as_posix(): sha256_file(frame)
            for frame in frames
        },
    }
    write_json(path, manifest)


def _load_approvals(path: Path) -> dict[str, Any]:
    if not path.is_file():
        return {"schema_version": 2, "states": {}}
    data = json.loads(path.read_text(encoding="utf-8"))
    if not isinstance(data, dict) or not isinstance(data.get("states"), dict):
        raise PipelineError(f"Invalid approval manifest: {path}")
    return data


def _get_approval(asset_dir: Path, state: str) -> dict[str, Any]:
    approval = _load_approvals(asset_dir / "work" / "approval.json").get("states", {}).get(state)
    if not isinstance(approval, dict):
        raise PipelineError(f"Approve a candidate before build-frames: {state}")
    if "canonical_file" not in approval or "canonical_sha256" not in approval:
        raise PipelineError(f"Missing canonical approval data: {state}")
    return approval


def _validate_canonical_hash(asset_dir: Path, state: str, canonical: Path) -> None:
    approval = _get_approval(asset_dir, state)
    if sha256_file(canonical) != approval["canonical_sha256"]:
        raise PipelineError(f"Canonical keyframe hash mismatch: {state}")


def _find_candidate_run(asset_dir: Path, state: str, candidate: Path) -> dict[str, Any]:
    manifest_path = asset_dir / "work" / "run_manifest.json"
    if not manifest_path.is_file():
        raise PipelineError(f"Missing Qwen run manifest for candidate approval: {state}")
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    candidate_relative = candidate.relative_to(asset_dir).as_posix()
    candidate_hash = sha256_file(candidate)
    for run in manifest.get("runs", []):
        if run.get("state") != state or not run.get("run_id"):
            continue
        if any(item.get("file") == candidate_relative and item.get("sha256") == candidate_hash for item in run.get("candidates", [])):
            return run
    raise PipelineError(f"Candidate is not present in the recorded Qwen run: {candidate_relative}")


def _maybe_pack_fighter_flight_review(asset_dir: Path, asset: dict[str, Any], force: bool) -> None:
    states = list(asset["animations"])
    if any(asset["animations"][state].get("recipe_version") not in {"fighter_flight_v1", "fighter_flight_hd_v1"} for state in states):
        return
    frame_dir = asset_dir / "output" / "frames"
    if any(not (frame_dir / f"{asset['asset_id']}_{state}_{index:03d}.png").is_file() for state in states for index in range(2)):
        return
    width = int(asset["target"]["cell_width"])
    height = int(asset["target"]["cell_height"])
    sheet_path = asset_dir / "output" / "sheets" / f"{asset['asset_id']}_flight_sheet.png"
    metadata_path = asset_dir / "output" / "metadata" / f"{asset['asset_id']}_flight.json"
    if (sheet_path.exists() or metadata_path.exists()) and not force:
        return
    sheet = Image.new("RGBA", (width * len(states), height * 2), (0, 0, 0, 0))
    frame_hashes: dict[str, str] = {}
    for column, state in enumerate(states):
        for row in range(2):
            path = frame_dir / f"{asset['asset_id']}_{state}_{row:03d}.png"
            sheet.alpha_composite(load_rgba(path), (column * width, row * height))
            frame_hashes[path.name] = sha256_file(path)
    sheet.save(sheet_path, format="PNG", optimize=False)
    write_json(
        metadata_path,
        {
            "schema_version": 2,
            "asset_id": asset["asset_id"],
            "frame_size": [width, height],
            "columns": states,
            "rows": ["frame_000", "frame_001"],
            "pivot": asset["anchors"]["pivot"],
            "sheet": sheet_path.name,
            "sha256": sha256_file(sheet_path),
            "frame_sha256": frame_hashes,
        },
    )


def _utc_now() -> str:
    return datetime.now(timezone.utc).isoformat()
