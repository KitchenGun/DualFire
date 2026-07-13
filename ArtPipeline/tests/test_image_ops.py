from pathlib import Path

import pytest
from PIL import Image

from dualfire_art.asset import load_asset, save_asset
from dualfire_art.errors import PipelineError
from dualfire_art.image_ops import (
    canonicalize_qwen_candidate,
    composite_rgba_on_rgb_background,
    count_visible_colors,
    pixelize,
    prepare_source,
    quantize_limited_palette,
    resize_nearest,
    stylize_source_preserving_pixel_art,
    threshold_to_binary_mask,
)


def test_prepare_preserves_binary_alpha(prepared_asset_dir: Path) -> None:
    hashes = prepare_source(prepared_asset_dir, load_asset(prepared_asset_dir))
    mask = Image.open(prepared_asset_dir / "work" / "normalized" / "mask.png")
    assert set(mask.tobytes()) == {0, 255}
    assert len(hashes["source_sha256"]) == 64


def test_threshold_to_binary_mask_includes_the_threshold_value() -> None:
    source = Image.new("L", (3, 1))
    source.putdata([127, 128, 129])

    result = threshold_to_binary_mask(source, 128)

    assert result.mode == "L"
    assert list(result.get_flattened_data()) == [0, 255, 255]


def test_resize_nearest_repeats_source_pixels_without_interpolation() -> None:
    source = Image.new("RGB", (2, 2))
    source.putdata([(10, 20, 30), (40, 50, 60), (70, 80, 90), (100, 110, 120)])

    result = resize_nearest(source, (4, 4))

    assert result.size == (4, 4)
    assert [result.getpixel((x, y)) for x, y in ((0, 0), (1, 1), (2, 0), (3, 1), (0, 2), (1, 3), (2, 2), (3, 3))] == [
        (10, 20, 30),
        (10, 20, 30),
        (40, 50, 60),
        (40, 50, 60),
        (70, 80, 90),
        (70, 80, 90),
        (100, 110, 120),
        (100, 110, 120),
    ]


def test_quantize_limited_palette_is_deterministic_and_does_not_dither(monkeypatch: pytest.MonkeyPatch) -> None:
    source = Image.new("RGB", (4, 2))
    source.putdata(
        [
            (0, 0, 0),
            (64, 64, 64),
            (128, 128, 128),
            (192, 192, 192),
        ]
        * 2
    )

    original_quantize = Image.Image.quantize
    dithers: list[Image.Dither] = []

    def record_quantize(image: Image.Image, *args: object, **kwargs: object) -> Image.Image:
        dithers.append(kwargs["dither"])
        return original_quantize(image, *args, **kwargs)

    monkeypatch.setattr(Image.Image, "quantize", record_quantize)
    first = quantize_limited_palette(source, 2).convert("RGB")
    second = quantize_limited_palette(source, 2).convert("RGB")

    assert first.tobytes() == second.tobytes()
    assert len(set(first.get_flattened_data())) == 2
    assert dithers == [Image.Dither.NONE, Image.Dither.NONE]


def test_composite_rgba_on_rgb_background_preserves_exact_background_pixels() -> None:
    subject = Image.new("RGBA", (2, 1))
    subject.putdata([(255, 0, 0, 0), (255, 0, 0, 128)])
    background = Image.new("RGB", (2, 1), (10, 20, 30))

    result = composite_rgba_on_rgb_background(subject, background)

    assert result.mode == "RGB"
    assert result.getpixel((0, 0)) == (10, 20, 30)
    assert result.getpixel((1, 0)) == (133, 10, 15)


def test_prepare_rejects_opaque_input_without_mask(tmp_path: Path, prepared_asset_dir: Path) -> None:
    opaque = Image.new("RGB", (8, 8), (255, 0, 0))
    opaque.save(prepared_asset_dir / "input" / "source.png")
    with pytest.raises(PipelineError, match="requires source.mask_file or source.chroma_key"):
        prepare_source(prepared_asset_dir, load_asset(prepared_asset_dir))


def test_prepare_chroma_key_writes_fitted_sprite(prepared_asset_dir: Path) -> None:
    source = Image.new("RGB", (20, 30), (0, 255, 0))
    for y in range(4, 26):
        for x in range(6, 14):
            source.putpixel((x, y), (80, 90, 100))
    source.save(prepared_asset_dir / "input" / "source.png")
    asset = load_asset(prepared_asset_dir)
    asset["source"]["chroma_key"] = {"color": [0, 255, 0], "tolerance": 8}
    asset["source"]["frame_fit"] = {"padding": 4}
    asset["target"]["cell_width"] = 32
    asset["target"]["cell_height"] = 32
    save_asset(prepared_asset_dir, asset)

    hashes = prepare_source(prepared_asset_dir, load_asset(prepared_asset_dir))

    mask = Image.open(prepared_asset_dir / "work" / "normalized" / "mask.png")
    fitted = Image.open(prepared_asset_dir / "work" / "normalized" / "fitted.png").convert("RGBA")
    fitted_mask = Image.open(prepared_asset_dir / "work" / "normalized" / "fitted_mask.png")
    assert mask.getpixel((0, 0)) == 0
    assert mask.getpixel((10, 10)) == 255
    assert fitted.size == (32, 32)
    assert fitted.getchannel("A").getbbox() == (11, 4, 20, 28)
    assert "fitted_sha256" in hashes
    assert "fitted_mask_sha256" in hashes
    assert set(fitted_mask.tobytes()) == {0, 255}


def test_pixelize_is_binary_and_palette_limited() -> None:
    image = Image.new("RGB", (8, 8))
    for y in range(8):
        for x in range(8):
            image.putpixel((x, y), (x * 31, y * 31, (x + y) * 15))
    mask = Image.new("L", (8, 8), 255)
    result = pixelize(image, mask, (4, 4), palette_limit=4, alpha_threshold=128)
    assert result.mode == "RGBA"
    assert result.size == (4, 4)
    assert count_visible_colors(result) <= 4
    assert set(result.getchannel("A").tobytes()) == {255}


def test_pixelize_ignores_chroma_key_pixels_when_building_palette() -> None:
    image = Image.new("RGB", (8, 8), (0, 255, 0))
    image.putpixel((3, 3), (255, 122, 36))
    mask = Image.new("L", (8, 8), 0)
    mask.putpixel((3, 3), 255)

    result = pixelize(image, mask, (8, 8), palette_limit=2, alpha_threshold=128)

    assert result.getpixel((3, 3))[:3] == (255, 122, 36)
    assert result.getpixel((0, 0))[3] == 0


def test_pixelize_preserves_reserved_palette_colors() -> None:
    image = Image.new("RGB", (8, 8), (80, 80, 88))
    image.putpixel((3, 3), (255, 112, 36))
    mask = Image.new("L", (8, 8), 255)

    result = pixelize(
        image,
        mask,
        (8, 8),
        palette_limit=2,
        alpha_threshold=128,
        reserved_palette_colors=[(255, 112, 36)],
    )

    assert result.getpixel((3, 3))[:3] == (255, 112, 36)


def test_canonicalize_qwen_candidate_removes_chroma_and_preserves_sprite_contract() -> None:
    candidate = Image.new("RGB", (32, 40), (0, 255, 0))
    for y in range(4, 35):
        for x in range(10, 22):
            candidate.putpixel((x, y), (80, 90, 100))

    result = canonicalize_qwen_candidate(
        candidate,
        (32, 32),
        {"color": [0, 255, 0], "tolerance": 8},
        None,
        padding=4,
        palette_limit=4,
        alpha_threshold=128,
        reserved_palette_colors=[(255, 112, 36)],
    )

    assert result.size == (32, 32)
    assert set(result.getchannel("A").tobytes()) == {0, 255}
    assert count_visible_colors(result) <= 4
    assert result.getpixel((0, 0))[3] == 0


def test_canonicalize_qwen_candidate_rejects_opaque_image_without_alpha_source() -> None:
    with pytest.raises(PipelineError, match="require canonical.chroma_key or --mask"):
        canonicalize_qwen_candidate(
            Image.new("RGB", (8, 8), (80, 80, 80)),
            (8, 8),
            None,
            None,
            padding=1,
            palette_limit=4,
            alpha_threshold=128,
            reserved_palette_colors=[],
        )


def test_canonicalize_qwen_candidate_preserves_high_detail_canvas() -> None:
    candidate = Image.new("RGB", (12, 16), (0, 255, 0))
    for y in range(2, 14):
        for x in range(3, 9):
            candidate.putpixel((x, y), (20 + x * 10, 40 + y * 10, 80 + (x + y) * 3))

    result = canonicalize_qwen_candidate(
        candidate,
        (12, 16),
        {"color": [0, 255, 0], "tolerance": 8},
        None,
        padding=0,
        palette_limit=2,
        alpha_threshold=128,
        reserved_palette_colors=[],
        color_mode="preserve",
        preserve_canvas=True,
    )

    assert result.size == (12, 16)
    assert result.getpixel((0, 0))[3] == 0
    assert count_visible_colors(result) > 2


def test_source_preserving_pixel_art_keeps_silhouette_and_reduces_palette() -> None:
    source = Image.new("RGBA", (16, 16), (0, 0, 0, 0))
    for y in range(3, 13):
        for x in range(4, 12):
            source.putpixel((x, y), (x * 16, y * 14, 110, 255))

    result = stylize_source_preserving_pixel_art(source, pixel_scale=2, palette_limit=8)

    assert result.size == source.size
    assert result.getpixel((0, 0))[3] == 0
    assert result.getpixel((8, 8))[3] == 255
    assert count_visible_colors(result) <= 9
