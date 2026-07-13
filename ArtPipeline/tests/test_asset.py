from pathlib import Path

import pytest

from dualfire_art.asset import create_asset, load_asset
from dualfire_art.errors import PipelineError
from dualfire_art.util import resolve_within


def test_create_asset_uses_image_only_defaults(tmp_path: Path) -> None:
    asset_dir = tmp_path / "asset"
    create_asset(asset_dir, "DF_PL_SHIP_TEST_01", "player", "normal", "none")
    asset = load_asset(asset_dir)
    assert asset["source"]["file"] == "input/source.png"
    assert asset["source"]["chroma_key"] is None
    assert asset["source"]["frame_fit"] is None
    assert asset["target"]["view"] == "top_down_orthographic"
    assert asset["target"]["color_mode"] == "indexed"
    assert (asset_dir / "work" / "approved").is_dir()


def test_rejects_invalid_asset_id(tmp_path: Path) -> None:
    with pytest.raises(PipelineError, match="asset_id"):
        create_asset(tmp_path / "asset", "bad-id", "player", "normal", "none")


def test_create_asset_pixel_sprite_profile_has_a_runnable_static_recipe(tmp_path: Path) -> None:
    asset_dir = tmp_path / "pixel_sprite"
    create_asset(asset_dir, "DF_PL_PIXEL_TEST_01", "player", "normal", "none", profile="pixel_sprite")

    asset = load_asset(asset_dir)
    assert asset["schema_version"] == 2
    assert asset["target"]["cell_width"] == 1024
    assert asset["animations"]["idle"]["recipe_version"] == "pixel_sprite_static_v1"
    assert asset["animations"]["idle"]["pixel_art"]["background"]["mode"] == "transparent"
    assert (asset_dir / "prompts" / "idle.txt").is_file()


def test_resolve_within_rejects_path_escape(tmp_path: Path) -> None:
    with pytest.raises(PipelineError, match="escapes"):
        resolve_within(tmp_path / "asset", "../outside.png")
