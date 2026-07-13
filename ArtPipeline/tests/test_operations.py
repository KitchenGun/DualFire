from __future__ import annotations

import json
from pathlib import Path

import pytest
from PIL import Image

from dualfire_art.asset import load_asset
from dualfire_art.errors import PipelineError
from dualfire_art.operations import (
    approve_candidate,
    build_frames,
    extract_birefnet_mask,
    pack_frames,
    prepare_asset,
)
from dualfire_art.validation import validate_outputs


def test_complete_local_flow(prepared_asset_dir: Path) -> None:
    prepare_asset(prepared_asset_dir)
    candidate_dir = prepared_asset_dir / "work" / "qwen" / "idle"
    candidate_dir.mkdir(parents=True)
    candidate = Image.new("RGB", (32, 32), (20, 80, 160))
    candidate.save(candidate_dir / "candidate_000.png")

    approve_candidate(prepared_asset_dir, "idle", 0)
    frames = build_frames(prepared_asset_dir, "idle")
    packed = pack_frames(prepared_asset_dir, "idle")
    result = validate_outputs(prepared_asset_dir)

    assert len(frames) == 2
    assert packed["sheet"].is_file()
    assert packed["metadata"].is_file()
    assert packed["preview"].is_file()
    assert result.ok, result.errors
    metadata = json.loads(packed["metadata"].read_text(encoding="utf-8"))
    assert metadata["sheet"] == {"file": "DF_EN_AIR_TEST_01_idle_sheet.png", "rows": 1, "columns": 2, "padding": 0}


def test_build_frames_requires_approval(prepared_asset_dir: Path) -> None:
    prepare_asset(prepared_asset_dir)
    with pytest.raises(PipelineError, match="Approve a candidate"):
        build_frames(prepared_asset_dir, "idle")


def test_pack_refuses_overwrite(prepared_asset_dir: Path) -> None:
    prepare_asset(prepared_asset_dir)
    candidate_dir = prepared_asset_dir / "work" / "qwen" / "idle"
    candidate_dir.mkdir(parents=True)
    Image.new("RGB", (32, 32), (20, 80, 160)).save(candidate_dir / "candidate_000.png")
    approve_candidate(prepared_asset_dir, "idle", 0)
    build_frames(prepared_asset_dir, "idle")
    pack_frames(prepared_asset_dir, "idle")
    with pytest.raises(PipelineError, match="Refusing to overwrite"):
        pack_frames(prepared_asset_dir, "idle")


def test_extract_birefnet_mask_can_apply_the_generated_source_mask(monkeypatch: pytest.MonkeyPatch, prepared_asset_dir: Path) -> None:
    class FakeComfyClient:
        def __init__(self, config: object):
            pass

        def __enter__(self) -> "FakeComfyClient":
            return self

        def __exit__(self, *args: object) -> None:
            pass

        def object_info(self) -> dict:
            return {
                "LoadImage": {},
                "LoadBackgroundRemovalModel": {"input": {"required": {"bg_removal_name": ["COMBO", {"options": []}]}}},
                "RemoveBackground": {},
                "ThresholdMask": {},
                "MaskToImage": {},
                "SaveImage": {},
            }

        def upload_image(self, image_path: Path, remote_name: str) -> str:
            return remote_name

        def submit(self, workflow: dict) -> str:
            assert workflow["4"]["inputs"]["value"] == 0.5
            return "birefnet-test"

        def wait_for_images(self, prompt_id: str) -> list[dict[str, str]]:
            return [{"filename": "mask.png", "subfolder": "", "type": "output"}]

        def download_image(self, image_info: dict[str, str]) -> bytes:
            from io import BytesIO

            result = Image.new("L", (16, 16), 255)
            output = BytesIO()
            result.save(output, format="PNG")
            return output.getvalue()

    import dualfire_art.operations as operations

    monkeypatch.setattr(operations, "ComfyClient", FakeComfyClient)
    mask = extract_birefnet_mask(prepared_asset_dir, object(), apply_source_mask=True)

    assert mask.is_file()
    assert load_asset(prepared_asset_dir)["source"]["mask_file"] == "work/masks/birefnet.png"


def test_force_run_record_keeps_the_prior_approval_provenance(tmp_path: Path) -> None:
    import dualfire_art.operations as operations

    first_run = {"run_id": "approved-run", "state": "idle", "candidates": []}
    next_run = {"run_id": "candidate-run", "state": "idle", "candidates": []}

    operations._record_run(tmp_path, first_run, force=False)
    operations._record_run(tmp_path, next_run, force=True)

    manifest = json.loads((tmp_path / "work" / "run_manifest.json").read_text(encoding="utf-8"))
    assert [run["run_id"] for run in manifest["runs"]] == ["approved-run", "candidate-run"]
