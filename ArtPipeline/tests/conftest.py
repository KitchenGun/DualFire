from __future__ import annotations

from pathlib import Path

import pytest
from PIL import Image

from dualfire_art.asset import create_asset


@pytest.fixture
def prepared_asset_dir(tmp_path: Path) -> Path:
    asset_dir = tmp_path / "DF_EN_AIR_TEST_01"
    create_asset(asset_dir, "DF_EN_AIR_TEST_01", "enemy", "normal", "air")
    source = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    for y in range(3, 13):
        for x in range(4, 12):
            source.putpixel((x, y), (40 + x * 8, 90 + y * 4, 180, 255))
    source.save(asset_dir / "input" / "source.png")
    return asset_dir
