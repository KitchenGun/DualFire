from __future__ import annotations

import json
from pathlib import Path

import pytest
from PIL import Image, ImageDraw

from dualfire_art.asset import create_asset, load_asset, save_asset, validate_asset_data
from dualfire_art.config import GenerationConfig, ModelConfig, PipelineConfig
from dualfire_art.errors import PipelineError
from dualfire_art.image_ops import count_visible_colors
from dualfire_art.operations import approve_candidate, build_frames, import_keyframe, pack_frames, prepare_asset, submit_comfy
from dualfire_art.util import sha256_file
from dualfire_art.validation import validate_outputs


STATES = (
    "north_afterburner_off",
    "move_left_afterburner_off",
    "move_right_afterburner_off",
    "north_afterburner_on",
    "move_left_afterburner_on",
    "move_right_afterburner_on",
)


def test_qwen_fighter_flight_builds_canonical_frames_and_review_sheet(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path)
    prepare_asset(asset_dir)
    for state in STATES:
        candidate = _write_candidate(asset_dir, state)
        _record_run(asset_dir, state, candidate)
        approved = approve_candidate(asset_dir, state, 0)
        assert approved.name == f"{state}.png"
        assert approved.parent.name == "canonical"
        first = build_frames(asset_dir, state)
        first_hashes = [sha256_file(path) for path in first]
        second = build_frames(asset_dir, state, force=True)
        assert first_hashes == [sha256_file(path) for path in second]
        pack_frames(asset_dir, state)

    result = validate_outputs(asset_dir)

    assert result.ok, result.errors
    review = Image.open(asset_dir / "output" / "sheets" / "DF_PL_FIGHTER_TEST_FLIGHT_flight_sheet.png")
    assert review.size == (64 * 6, 64 * 2)
    approval = json.loads((asset_dir / "work" / "approval.json").read_text(encoding="utf-8"))
    assert set(approval["states"]) == set(STATES)
    assert all("run_id" in item and "canonical_sha256" in item for item in approval["states"].values())


def test_qwen_fighter_flight_preserves_high_detail_frames(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path, high_detail=True)
    prepare_asset(asset_dir)
    for state in STATES:
        candidate = _write_candidate(asset_dir, state)
        _record_run(asset_dir, state, candidate)
        approve_candidate(asset_dir, state, 0)
        build_frames(asset_dir, state)
        pack_frames(asset_dir, state)

    result = validate_outputs(asset_dir)

    assert result.ok, result.errors
    canonical = Image.open(asset_dir / "work" / "canonical" / "north_afterburner_off.png").convert("RGBA")
    assert canonical.size == (128, 128)
    pixels = canonical.get_flattened_data() if hasattr(canonical, "get_flattened_data") else canonical.getdata()
    assert len({pixel[:3] for pixel in pixels if pixel[3] > 0}) > 16


def test_qwen_v2_rejects_parent_state_before_approval(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path)
    prepare_asset(asset_dir)
    config = _config(tmp_path)

    with pytest.raises(PipelineError, match="Approve and canonicalize input state"):
        submit_comfy(asset_dir, "move_left_afterburner_off", config)


def test_imported_keyframe_uses_the_same_approval_and_build_contract(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path, high_detail=True)
    external = tmp_path / "camo_fighter.png"
    Image.new("RGB", (128, 128), (0, 255, 0)).save(external)
    image = Image.open(external).convert("RGB")
    ImageDraw.Draw(image).polygon(((64, 12), (34, 108), (64, 116), (94, 108)), fill=(82, 94, 112))
    image.save(external)

    canonical = import_keyframe(asset_dir, "north_afterburner_off", external)
    build_frames(asset_dir, "north_afterburner_off")
    pack_frames(asset_dir, "north_afterburner_off")
    result = validate_outputs(asset_dir, "north_afterburner_off")

    run = json.loads((asset_dir / "work" / "run_manifest.json").read_text(encoding="utf-8"))["runs"][0]
    assert canonical.is_file()
    assert run["run_type"] == "imported_keyframe"
    assert run["input"]["file_name"] == "camo_fighter.png"
    assert result.ok, result.errors


def test_source_preserving_pixel_recipe_keeps_the_imported_silhouette(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path, high_detail=True)
    asset = load_asset(asset_dir)
    asset["target"]["palette_limit"] = 64
    asset["animations"]["north_afterburner_off"]["recipe_version"] = "source_preserving_pixel_v1"
    asset["animations"]["north_afterburner_off"]["pixel_art"] = {"pixel_scale": 4, "palette_limit": 32}
    save_asset(asset_dir, asset)
    source = Image.new("RGB", (128, 128), (0, 255, 0))
    ImageDraw.Draw(source).polygon(((64, 8), (26, 112), (64, 120), (102, 112)), fill=(88, 103, 122))
    source.save(asset_dir / "input" / "preserve.png")

    canonical = import_keyframe(asset_dir, "north_afterburner_off", asset_dir / "input" / "preserve.png")
    frames = build_frames(asset_dir, "north_afterburner_off")

    base = Image.open(canonical).convert("RGBA")
    frame = Image.open(frames[0]).convert("RGBA")
    assert frame.size == base.size == (128, 128)
    assert frame.getchannel("A").getbbox() == base.getchannel("A").getbbox()
    assert count_visible_colors(frame) <= 33


def test_static_high_detail_recipe_writes_one_unchanged_keyframe(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path, high_detail=True)
    asset = load_asset(asset_dir)
    asset["animations"] = {
        "idle": {
            "frames": 1,
            "fps": 1,
            "loop": True,
            "recipe_version": "static_keyframe_hd_v1",
            "qwen": {"input_state": "source", "prompt_file": "prompts/north_afterburner_off.txt", "candidates": 1},
        }
    }
    save_asset(asset_dir, asset)
    prepare_asset(asset_dir)
    candidate = _write_candidate(asset_dir, "idle")
    _record_run(asset_dir, "idle", candidate)
    canonical = approve_candidate(asset_dir, "idle", 0)

    frames = build_frames(asset_dir, "idle")
    pack_frames(asset_dir, "idle")
    result = validate_outputs(asset_dir)

    assert len(frames) == 1
    assert Image.open(frames[0]).convert("RGBA").tobytes() == Image.open(canonical).convert("RGBA").tobytes()
    assert result.ok, result.errors


def test_static_pixel_sprite_recipe_quantizes_and_composites_chroma_background(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path, high_detail=True)
    asset = load_asset(asset_dir)
    asset["animations"] = {
        "idle": {
            "frames": 1,
            "fps": 1,
            "loop": True,
            "recipe_version": "pixel_sprite_static_v1",
            "pixel_art": {
                "working_width": 32,
                "working_height": 32,
                "palette_limit": 8,
                "background": {"mode": "chroma_key", "color": [0, 255, 0]},
            },
            "qwen": {"input_state": "source", "prompt_file": "prompts/north_afterburner_off.txt", "candidates": 1},
        }
    }
    save_asset(asset_dir, asset)
    prepare_asset(asset_dir)
    candidate = _write_candidate(asset_dir, "idle")
    _record_run(asset_dir, "idle", candidate)
    approve_candidate(asset_dir, "idle", 0)

    frame = build_frames(asset_dir, "idle")[0]
    pack_frames(asset_dir, "idle")
    result = validate_outputs(asset_dir)
    image = Image.open(frame).convert("RGBA")

    assert image.getchannel("A").getextrema() == (255, 255)
    assert image.getpixel((0, 0))[:3] == (0, 255, 0)
    assert result.ok, result.errors


def test_qwen_v2_rejects_candidate_not_in_recorded_run(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path)
    candidate = _write_candidate(asset_dir, "north_afterburner_off")

    with pytest.raises(PipelineError, match="Missing Qwen run manifest"):
        approve_candidate(asset_dir, "north_afterburner_off", 0)

    _record_run(asset_dir, "north_afterburner_off", candidate, candidate_hash="0" * 64)
    with pytest.raises(PipelineError, match="not present in the recorded Qwen run"):
        approve_candidate(asset_dir, "north_afterburner_off", 0)


def test_qwen_v2_rejects_dependency_cycle(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path)
    data = load_asset(asset_dir)
    data["animations"]["north_afterburner_off"]["qwen"]["input_state"] = "move_left_afterburner_off"
    data["animations"]["move_left_afterburner_off"]["qwen"]["input_state"] = "north_afterburner_off"

    with pytest.raises(PipelineError, match="dependency cycle"):
        validate_asset_data(data)


def test_qwen_v2_validation_detects_stale_canonical_hash(tmp_path: Path) -> None:
    asset_dir = _create_v2_fighter_asset(tmp_path)
    prepare_asset(asset_dir)
    for state in STATES:
        candidate = _write_candidate(asset_dir, state)
        _record_run(asset_dir, state, candidate)
        approve_candidate(asset_dir, state, 0)
        build_frames(asset_dir, state)
        pack_frames(asset_dir, state)

    canonical = asset_dir / "work" / "canonical" / "north_afterburner_off.png"
    image = Image.open(canonical).convert("RGBA")
    image.putpixel((32, 32), (255, 255, 255, 255))
    image.save(canonical)

    result = validate_outputs(asset_dir)

    assert not result.ok
    assert any("Canonical keyframe hash mismatch" in error for error in result.errors)


def test_player_fighter_asset_uses_high_detail_preserve_profile() -> None:
    asset_dir = Path(__file__).resolve().parents[1] / "assets" / "player_fighter_flight"
    data = load_asset(asset_dir)

    assert data["target"]["color_mode"] == "preserve"
    assert (data["target"]["cell_width"], data["target"]["cell_height"]) == (1075, 1464)
    assert data["canonical"]["canvas_mode"] == "preserve"
    assert {state["recipe_version"] for state in data["animations"].values()} == {"fighter_flight_hd_v1"}


def _create_v2_fighter_asset(tmp_path: Path, high_detail: bool = False) -> Path:
    asset_dir = tmp_path / "DF_PL_FIGHTER_TEST_FLIGHT"
    create_asset(asset_dir, "DF_PL_FIGHTER_TEST_FLIGHT", "player", "normal", "none")
    source = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
    ImageDraw.Draw(source).polygon(((64, 10), (46, 110), (64, 118), (82, 110)), fill=(82, 94, 112, 255))
    source.save(asset_dir / "input" / "source.png")
    prompts = asset_dir / "prompts"
    prompts.mkdir()
    data = load_asset(asset_dir)
    data["schema_version"] = 2
    data["target"].update({
        "cell_width": 128 if high_detail else 64,
        "cell_height": 128 if high_detail else 64,
        "color_mode": "preserve" if high_detail else "indexed",
        "reserved_palette_colors": [[255, 242, 200], [255, 188, 74], [255, 112, 36]],
    })
    data["canonical"] = {
        "chroma_key": {"color": [0, 255, 0], "tolerance": 8},
        "canvas_mode": "preserve" if high_detail else "fit",
        "frame_fit": {"padding": 0 if high_detail else 8},
    }
    data["animations"] = {}
    for state in STATES:
        parent = "source" if state == "north_afterburner_off" else "north_afterburner_off"
        prompt = prompts / f"{state}.txt"
        prompt.write_text("fighter sprite", encoding="utf-8")
        data["animations"][state] = {
            "frames": 2,
            "fps": 12,
            "loop": True,
            "recipe_version": "fighter_flight_hd_v1" if high_detail else "fighter_flight_v1",
            "qwen": {"input_state": parent, "prompt_file": f"prompts/{prompt.name}", "candidates": 1},
        }
    data["anchors"] = {
        "coordinate_origin": "top_left",
        "pivot": [32, 32],
        "hit_center": [32, 32],
        "muzzle": [32, 4],
    }
    save_asset(asset_dir, data)
    return asset_dir


def _write_candidate(asset_dir: Path, state: str) -> Path:
    path = asset_dir / "work" / "qwen" / state / "candidate_000.png"
    path.parent.mkdir(parents=True, exist_ok=True)
    image = Image.new("RGB", (128, 128), (0, 255, 0))
    draw = ImageDraw.Draw(image)
    offset = -4 if "move_left" in state else 4 if "move_right" in state else 0
    draw.polygon(((64 + offset, 12), (34 + offset, 108), (64 + offset, 116), (94 + offset, 108)), fill=(82, 94, 112))
    draw.rectangle((57 + offset, 38, 71 + offset, 66), fill=(11, 78, 87))
    for index in range(24):
        draw.point((52 + offset + (index % 12) * 2, 72 + (index // 12) * 3), fill=(60 + index * 3, 70 + index * 2, 90 + index))
    path.write_bytes(_png_bytes(image))
    return path


def _record_run(asset_dir: Path, state: str, candidate: Path, candidate_hash: str | None = None) -> None:
    manifest_path = asset_dir / "work" / "run_manifest.json"
    manifest = {"schema_version": 1, "runs": []}
    if manifest_path.is_file():
        manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    manifest["runs"].append(
        {
            "run_id": f"run-{state}",
            "state": state,
            "candidates": [
                {
                    "file": candidate.relative_to(asset_dir).as_posix(),
                    "sha256": candidate_hash or sha256_file(candidate),
                }
            ],
        }
    )
    manifest_path.write_text(json.dumps(manifest), encoding="utf-8")


def _config(tmp_path: Path) -> PipelineConfig:
    return PipelineConfig(
        comfy_url="http://127.0.0.1:8001",
        comfy_data_dir=tmp_path,
        workflow_file=tmp_path / "workflow.json",
        models=ModelConfig("diffusion.safetensors", "encoder.safetensors", "vae.safetensors"),
        generation=GenerationConfig(),
    )


def _png_bytes(image: Image.Image) -> bytes:
    path = image.copy()
    from io import BytesIO

    output = BytesIO()
    path.save(output, format="PNG")
    return output.getvalue()
