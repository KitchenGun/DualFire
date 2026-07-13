from __future__ import annotations

import re
from pathlib import Path
from typing import Any

from .errors import PipelineError
from .util import load_yaml, write_yaml


ASSET_ID_PATTERN = re.compile(r"^[A-Z][A-Z0-9_]{2,63}$")
STATE_PATTERN = re.compile(r"^[a-z][a-z0-9_]{0,31}$")
ALLOWED_CATEGORIES = {"player", "enemy", "projectile"}
ALLOWED_ATTRIBUTES = {"none", "air", "ground", "universal", "anti_air", "anti_ground"}
ALLOWED_COLOR_MODES = {"indexed", "preserve"}
ALLOWED_CANVAS_MODES = {"fit", "preserve"}
ALLOWED_PIXEL_BACKGROUND_MODES = {"transparent", "chroma_key"}
ALLOWED_ASSET_PROFILES = {"legacy", "pixel_sprite"}
SUPPORTED_SCHEMA_VERSIONS = {1, 2}


def create_asset(
    asset_dir: Path,
    asset_id: str,
    category: str,
    grade: str,
    attribute: str,
    profile: str = "legacy",
    force: bool = False,
) -> Path:
    validate_asset_id(asset_id)
    if category not in ALLOWED_CATEGORIES:
        raise PipelineError(f"Unsupported category: {category}")
    if attribute not in ALLOWED_ATTRIBUTES:
        raise PipelineError(f"Unsupported attribute: {attribute}")
    if profile not in ALLOWED_ASSET_PROFILES:
        raise PipelineError(f"Unsupported asset profile: {profile}")
    config_path = asset_dir / "asset.yaml"
    if config_path.exists() and not force:
        raise PipelineError(f"Asset already exists: {config_path}")
    for relative in (
        "input",
        "work/normalized",
        "work/qwen",
        "work/imported",
        "work/approved",
        "work/canonical",
        "output/frames",
        "output/sheets",
        "output/metadata",
        "output/preview",
    ):
        (asset_dir / relative).mkdir(parents=True, exist_ok=True)
    data = default_asset_data(asset_id, category, grade, attribute, profile)
    write_yaml(config_path, data)
    if profile == "pixel_sprite":
        (asset_dir / "prompts").mkdir(parents=True, exist_ok=True)
        (asset_dir / "prompts" / "idle.txt").write_text(PIXEL_SPRITE_PROMPT, encoding="utf-8")
    return config_path


PIXEL_SPRITE_PROMPT = (
    "Preserve the supplied subject silhouette and proportions exactly. Render crisp military pixel art only: "
    "clean pixel clusters, controlled palette, no text, no logo, no environment, no shadow, transparent background."
)


def default_asset_data(asset_id: str, category: str, grade: str, attribute: str, profile: str = "legacy") -> dict[str, Any]:
    if profile == "pixel_sprite":
        return _pixel_sprite_asset_data(asset_id, category, grade, attribute)
    return {
        "schema_version": 1,
        "asset_id": asset_id,
        "category": category,
        "grade": grade,
        "attribute": attribute,
        "source": {
            "file": "input/source.png",
            "mask_file": None,
            "chroma_key": None,
            "frame_fit": None,
            "sha256": None,
            "origin": "user_provided",
            "license": "unverified",
        },
        "target": {
            "cell_width": 48,
            "cell_height": 48,
            "view": "top_down_orthographic",
            "color_mode": "indexed",
            "palette_limit": 16,
            "reserved_palette_colors": [],
            "alpha_threshold": 128,
        },
        "animations": {
            "idle": {
                "frames": 2,
                "fps": 6,
                "loop": True,
                "source_frames": [],
                "recipe": {"type": "offsets", "offsets": [[0, 0], [0, 0]]},
            }
        },
        "anchors": {
            "coordinate_origin": "top_left",
            "pivot": [24, 24],
            "hit_center": [24, 24],
            "muzzle": [24, 6],
        },
    }


def _pixel_sprite_asset_data(asset_id: str, category: str, grade: str, attribute: str) -> dict[str, Any]:
    return {
        "schema_version": 2,
        "asset_id": asset_id,
        "category": category,
        "grade": grade,
        "attribute": attribute,
        "source": {
            "file": "input/source.png",
            "mask_file": None,
            "chroma_key": None,
            "frame_fit": {"padding": 32},
            "sha256": None,
            "origin": "user_provided",
            "license": "unverified",
        },
        "target": {
            "cell_width": 1024,
            "cell_height": 1024,
            "view": "top_down_orthographic",
            "color_mode": "preserve",
            "palette_limit": 48,
            "reserved_palette_colors": [],
            "alpha_threshold": 128,
        },
        "canonical": {
            "canvas_mode": "fit",
            "chroma_key": {"color": [0, 255, 0], "tolerance": 8},
            "frame_fit": {"padding": 32},
        },
        "animations": {
            "idle": {
                "frames": 1,
                "fps": 1,
                "loop": True,
                "recipe_version": "pixel_sprite_static_v1",
                "pixel_art": {
                    "working_width": 128,
                    "working_height": 128,
                    "palette_limit": 48,
                    "alpha_threshold": 128,
                    "background": {"mode": "transparent"},
                },
                "qwen": {"input_state": "source", "prompt_file": "prompts/idle.txt", "candidates": 1},
            }
        },
        "anchors": {
            "coordinate_origin": "top_left",
            "pivot": [512, 512],
            "hit_center": [512, 512],
            "muzzle": [512, 32],
        },
    }


def load_asset(asset_dir: Path) -> dict[str, Any]:
    data = load_yaml(asset_dir / "asset.yaml")
    validate_asset_data(data)
    return data


def save_asset(asset_dir: Path, data: dict[str, Any]) -> None:
    validate_asset_data(data)
    write_yaml(asset_dir / "asset.yaml", data)


def validate_asset_data(data: dict[str, Any]) -> None:
    schema_version = data.get("schema_version")
    if schema_version not in SUPPORTED_SCHEMA_VERSIONS:
        raise PipelineError("asset.yaml schema_version must be 1 or 2")
    validate_asset_id(str(data.get("asset_id", "")))
    if data.get("category") not in ALLOWED_CATEGORIES:
        raise PipelineError(f"Unsupported category: {data.get('category')}")
    if data.get("attribute") not in ALLOWED_ATTRIBUTES:
        raise PipelineError(f"Unsupported attribute: {data.get('attribute')}")
    source = data.get("source")
    target = data.get("target")
    animations = data.get("animations")
    anchors = data.get("anchors")
    if not all(isinstance(value, dict) for value in (source, target, animations, anchors)):
        raise PipelineError("asset.yaml requires source, target, animations, and anchors mappings")
    if target.get("view") != "top_down_orthographic":
        raise PipelineError("target.view must be top_down_orthographic")
    for key in ("cell_width", "cell_height"):
        value = int(target.get(key, 0))
        if not 1 <= value <= 2048:
            raise PipelineError(f"target.{key} must be between 1 and 2048")
    _validate_source_processing(source, target)
    color_mode = target.get("color_mode", "indexed")
    if color_mode not in ALLOWED_COLOR_MODES:
        raise PipelineError("target.color_mode must be indexed or preserve")
    palette = int(target.get("palette_limit", 0))
    if not 2 <= palette <= 256:
        raise PipelineError("target.palette_limit must be between 2 and 256")
    _validate_reserved_palette_colors(target.get("reserved_palette_colors", []), palette)
    threshold = int(target.get("alpha_threshold", -1))
    if not 0 <= threshold <= 255:
        raise PipelineError("target.alpha_threshold must be between 0 and 255")
    if anchors.get("coordinate_origin") != "top_left":
        raise PipelineError("anchors.coordinate_origin must be top_left")
    for state, animation in animations.items():
        validate_state(state)
        if not isinstance(animation, dict):
            raise PipelineError(f"Animation must be a mapping: {state}")
        if int(animation.get("frames", 0)) < 1:
            raise PipelineError(f"Animation frames must be positive: {state}")
        if float(animation.get("fps", 0)) <= 0:
            raise PipelineError(f"Animation fps must be positive: {state}")
        _validate_pixel_art_config(animation, target)
    if schema_version == 2:
        _validate_qwen_asset_v2(data)


def validate_asset_id(asset_id: str) -> None:
    if not ASSET_ID_PATTERN.fullmatch(asset_id):
        raise PipelineError("asset_id must match ^[A-Z][A-Z0-9_]{2,63}$")


def validate_state(state: str) -> None:
    if not STATE_PATTERN.fullmatch(state):
        raise PipelineError(f"Animation state must be lowercase snake_case: {state}")


def _validate_source_processing(source: dict[str, Any], target: dict[str, Any]) -> None:
    chroma_key = source.get("chroma_key")
    if chroma_key is not None:
        if source.get("mask_file"):
            raise PipelineError("source.chroma_key cannot be combined with source.mask_file")
        if not isinstance(chroma_key, dict):
            raise PipelineError("source.chroma_key must be a mapping")
        color = chroma_key.get("color")
        if not isinstance(color, list) or len(color) != 3 or any(not isinstance(value, int) or not 0 <= value <= 255 for value in color):
            raise PipelineError("source.chroma_key.color must be an RGB [0..255, 0..255, 0..255] list")
        tolerance = chroma_key.get("tolerance")
        if not isinstance(tolerance, int) or not 0 <= tolerance <= 441:
            raise PipelineError("source.chroma_key.tolerance must be between 0 and 441")

    frame_fit = source.get("frame_fit")
    if frame_fit is not None:
        if not isinstance(frame_fit, dict):
            raise PipelineError("source.frame_fit must be a mapping")
        padding = frame_fit.get("padding")
        max_padding = (min(int(target["cell_width"]), int(target["cell_height"])) - 1) // 2
        if not isinstance(padding, int) or not 0 <= padding <= max_padding:
            raise PipelineError(f"source.frame_fit.padding must be between 0 and {max_padding}")


def _validate_reserved_palette_colors(value: Any, palette_limit: int) -> None:
    if not isinstance(value, list) or len(value) >= palette_limit:
        raise PipelineError("target.reserved_palette_colors must contain fewer entries than target.palette_limit")
    for color in value:
        if not isinstance(color, list) or len(color) != 3 or any(not isinstance(channel, int) or not 0 <= channel <= 255 for channel in color):
            raise PipelineError("target.reserved_palette_colors entries must be RGB [0..255, 0..255, 0..255] lists")


def _validate_qwen_asset_v2(data: dict[str, Any]) -> None:
    canonical = data.get("canonical")
    if not isinstance(canonical, dict):
        raise PipelineError("schema_version 2 requires a canonical mapping")
    _validate_chroma_key(canonical.get("chroma_key"), "canonical.chroma_key")
    _validate_frame_fit(canonical.get("frame_fit"), data["target"], "canonical.frame_fit")
    if canonical.get("canvas_mode", "fit") not in ALLOWED_CANVAS_MODES:
        raise PipelineError("canonical.canvas_mode must be fit or preserve")

    animations = data["animations"]
    for state, animation in animations.items():
        qwen = animation.get("qwen")
        if not isinstance(qwen, dict):
            raise PipelineError(f"schema_version 2 animation requires qwen: {state}")
        input_state = qwen.get("input_state")
        if input_state != "source" and input_state not in animations:
            raise PipelineError(f"qwen.input_state must be source or an animation state: {state}")
        if input_state == state:
            raise PipelineError(f"qwen.input_state cannot reference itself: {state}")
        prompt_file = qwen.get("prompt_file")
        if not isinstance(prompt_file, str) or not prompt_file:
            raise PipelineError(f"qwen.prompt_file is required: {state}")
        candidates = qwen.get("candidates")
        if not isinstance(candidates, int) or not 1 <= candidates <= 16:
            raise PipelineError(f"qwen.candidates must be between 1 and 16: {state}")
        recipe_version = animation.get("recipe_version")
        if not isinstance(recipe_version, str) or not recipe_version:
            raise PipelineError(f"recipe_version is required: {state}")
        if recipe_version == "pixel_sprite_static_v1":
            if int(animation["frames"]) != 1:
                raise PipelineError("pixel_sprite_static_v1 requires one frame")
            pixel_art = animation.get("pixel_art")
            if not isinstance(pixel_art, dict) or "working_width" not in pixel_art or "working_height" not in pixel_art:
                raise PipelineError("pixel_sprite_static_v1 requires pixel_art.working_width and working_height")
    _validate_qwen_dependency_cycles(animations)


def _validate_pixel_art_config(animation: dict[str, Any], target: dict[str, Any]) -> None:
    pixel_art = animation.get("pixel_art")
    if pixel_art is None:
        return
    if not isinstance(pixel_art, dict):
        raise PipelineError("pixel_art must be a mapping")

    palette_limit = pixel_art.get("palette_limit")
    if palette_limit is not None:
        if not isinstance(palette_limit, int) or not 2 <= palette_limit <= int(target["palette_limit"]):
            raise PipelineError("pixel_art.palette_limit must be between 2 and target.palette_limit")

    pixel_scale = pixel_art.get("pixel_scale")
    if pixel_scale is not None and (not isinstance(pixel_scale, int) or not 1 <= pixel_scale <= 16):
        raise PipelineError("pixel_art.pixel_scale must be between 1 and 16")

    width = pixel_art.get("working_width")
    height = pixel_art.get("working_height")
    if (width is None) != (height is None):
        raise PipelineError("pixel_art.working_width and working_height must be provided together")
    if width is not None:
        target_width = int(target["cell_width"])
        target_height = int(target["cell_height"])
        if not isinstance(width, int) or not isinstance(height, int) or not 1 <= width <= target_width or not 1 <= height <= target_height:
            raise PipelineError("pixel_art working size must fit inside the target frame")
        if target_width % width != 0 or target_height % height != 0:
            raise PipelineError("pixel_art working size must evenly divide the target frame for nearest scaling")

    background = pixel_art.get("background", {"mode": "transparent"})
    if not isinstance(background, dict):
        raise PipelineError("pixel_art.background must be a mapping")
    mode = background.get("mode", "transparent")
    if mode not in ALLOWED_PIXEL_BACKGROUND_MODES:
        raise PipelineError("pixel_art.background.mode must be transparent or chroma_key")
    if mode == "chroma_key":
        _validate_rgb_color(background.get("color"), "pixel_art.background.color")


def _validate_rgb_color(value: Any, name: str) -> None:
    if not isinstance(value, list) or len(value) != 3 or any(not isinstance(channel, int) or not 0 <= channel <= 255 for channel in value):
        raise PipelineError(f"{name} must be an RGB [0..255, 0..255, 0..255] list")


def _validate_chroma_key(value: Any, name: str) -> None:
    if not isinstance(value, dict):
        raise PipelineError(f"{name} must be a mapping")
    color = value.get("color")
    if not isinstance(color, list) or len(color) != 3 or any(not isinstance(channel, int) or not 0 <= channel <= 255 for channel in color):
        raise PipelineError(f"{name}.color must be an RGB [0..255, 0..255, 0..255] list")
    tolerance = value.get("tolerance")
    if not isinstance(tolerance, int) or not 0 <= tolerance <= 441:
        raise PipelineError(f"{name}.tolerance must be between 0 and 441")


def _validate_frame_fit(value: Any, target: dict[str, Any], name: str) -> None:
    if not isinstance(value, dict):
        raise PipelineError(f"{name} must be a mapping")
    padding = value.get("padding")
    max_padding = (min(int(target["cell_width"]), int(target["cell_height"])) - 1) // 2
    if not isinstance(padding, int) or not 0 <= padding <= max_padding:
        raise PipelineError(f"{name}.padding must be between 0 and {max_padding}")


def _validate_qwen_dependency_cycles(animations: dict[str, Any]) -> None:
    visiting: set[str] = set()
    visited: set[str] = set()

    def visit(state: str) -> None:
        if state in visited:
            return
        if state in visiting:
            raise PipelineError(f"qwen.input_state dependency cycle detected: {state}")
        visiting.add(state)
        parent = animations[state]["qwen"]["input_state"]
        if parent != "source":
            visit(str(parent))
        visiting.remove(state)
        visited.add(state)

    for state in animations:
        visit(state)
