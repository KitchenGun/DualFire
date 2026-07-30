"""Render modular Unreal-ready textures for the mission result screen."""

from __future__ import annotations

import argparse
import json
from pathlib import Path

from PIL import Image, ImageChops, ImageDraw, ImageFilter, ImageFont, ImageOps


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT / "assets" / "ui_result" / "source" / "T_UI_Result_Background_Source.png"
DEFAULT_OUTPUT = ROOT / "assets" / "ui_result" / "output" / "textures"
DEFAULT_PREVIEW = ROOT.parent / "Saved" / "DesignPreviews" / "ResultUI" / "ResultLayers_Composite.png"
FONT_CANDIDATES = (
    Path("C:/Windows/Fonts/impact.ttf"),
    Path("C:/Windows/Fonts/bahnschrift.ttf"),
    Path("C:/Windows/Fonts/arialbd.ttf"),
)

CYAN = (72, 224, 244, 255)
CYAN_DIM = (45, 132, 154, 150)
LINE_DIM = (74, 128, 145, 94)
PANEL_FILL = (2, 12, 19, 156)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--background-source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT)
    parser.add_argument("--preview", type=Path, default=DEFAULT_PREVIEW)
    return parser.parse_args()


def draw_corner_brackets(draw: ImageDraw.ImageDraw, bounds: tuple[int, int, int, int], length: int = 28) -> None:
    left, top, right, bottom = bounds
    segments = (
        (left, top + length, left, top, left + length, top),
        (right - length, top, right, top, right, top + length),
        (left, bottom - length, left, bottom, left + length, bottom),
        (right - length, bottom, right, bottom, right, bottom - length),
    )
    for segment in segments:
        draw.line(segment, fill=CYAN, width=2, joint="curve")


def render_background(source: Path, output_dir: Path) -> Path:
    with Image.open(source) as image:
        background = ImageOps.fit(image.convert("RGB"), (1920, 1080), method=Image.Resampling.LANCZOS)
    destination = output_dir / "T_UI_Result_Background.png"
    background.save(destination, format="PNG", optimize=True)
    return destination


def render_evaluation_row(output_dir: Path) -> Path:
    width, height = 1024, 112
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    pixels = image.load()
    for x in range(width):
        fade = max(0.0, 1.0 - (x / width) ** 1.8)
        alpha = round(PANEL_FILL[3] * fade)
        for y in range(height):
            pixels[x, y] = (*PANEL_FILL[:3], alpha)

    draw = ImageDraw.Draw(image)
    draw.line((8, 14, 8, 27), fill=CYAN, width=2)
    draw.line((0, height - 1, width, height - 1), fill=LINE_DIM, width=1)
    draw.line((438, 91, 938, 91), fill=CYAN_DIM, width=2)

    destination = output_dir / "T_UI_Result_EvaluationRow.png"
    image.save(destination, format="PNG", optimize=True)
    return destination


def render_rank_panel(output_dir: Path) -> Path:
    width, height = 512, 640
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rectangle((10, 10, width - 10, height - 10), fill=PANEL_FILL)
    draw_corner_brackets(draw, (8, 8, width - 8, height - 8), 30)
    divider_y = 344
    center = width // 2
    draw.line((8, divider_y, center - 14, divider_y), fill=CYAN, width=2)
    draw.line((center - 14, divider_y, center, divider_y + 12), fill=CYAN, width=2)
    draw.line((center, divider_y + 12, center + 14, divider_y), fill=CYAN, width=2)
    draw.line((center + 14, divider_y, width - 8, divider_y), fill=CYAN, width=2)
    draw.line((44, 500, width - 44, 500), fill=LINE_DIM, width=2)

    destination = output_dir / "T_UI_Result_RankPanel.png"
    image.save(destination, format="PNG", optimize=True)
    return destination


def render_unlock_panel(output_dir: Path) -> Path:
    width, height = 768, 112
    image = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rectangle((8, 8, width - 8, height - 8), fill=(2, 14, 21, 170))
    draw_corner_brackets(draw, (8, 8, width - 8, height - 8), 24)
    draw.rectangle((42, 30, 90, 78), outline=CYAN_DIM, width=2)
    draw.polygon(((66, 38), (72, 57), (86, 57), (75, 66), (80, 82), (66, 72), (52, 82), (57, 66), (46, 57), (60, 57)), fill=CYAN)

    destination = output_dir / "T_UI_Result_UnlockPanel.png"
    image.save(destination, format="PNG", optimize=True)
    return destination


def find_font(size: int) -> ImageFont.FreeTypeFont:
    for path in FONT_CANDIDATES:
        if path.exists():
            return ImageFont.truetype(str(path), size=size)
    raise FileNotFoundError("No supported rank font was found")


def render_rank(letter: str, output_dir: Path) -> Path:
    size = 512
    font = find_font(390)
    mask = Image.new("L", (size, size), 0)
    mask_draw = ImageDraw.Draw(mask)
    bounds = mask_draw.textbbox((0, 0), letter, font=font, stroke_width=2)
    x = (size - (bounds[2] - bounds[0])) // 2 - bounds[0]
    y = (size - (bounds[3] - bounds[1])) // 2 - bounds[1] - 6
    mask_draw.text((x, y), letter, font=font, fill=255)

    glow_mask = mask.filter(ImageFilter.GaussianBlur(radius=14))
    glow_mask = glow_mask.point(lambda alpha: round(alpha * 0.18))
    image = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    glow = Image.new("RGBA", (size, size), (55, 222, 246, 0))
    glow.putalpha(glow_mask)
    image = Image.alpha_composite(image, glow)

    stroke_mask = mask.filter(ImageFilter.MaxFilter(9))
    stroke_mask = ImageChops.subtract(stroke_mask, mask).point(lambda alpha: round(alpha * 0.74))
    stroke = Image.new("RGBA", (size, size), (63, 221, 241, 0))
    stroke.putalpha(stroke_mask)
    image = Image.alpha_composite(image, stroke)

    fill = Image.new("RGBA", (size, size), (0, 0, 0, 0))
    fill_pixels = fill.load()
    for y_pos in range(size):
        ratio = y_pos / (size - 1)
        color = (
            round(230 - 58 * ratio),
            round(252 - 18 * ratio),
            round(255 - 8 * ratio),
            255,
        )
        for x_pos in range(size):
            fill_pixels[x_pos, y_pos] = color
    fill.putalpha(mask)
    image = Image.alpha_composite(image, fill)

    stripe_mask = Image.new("L", (size, size), 0)
    scan_draw = ImageDraw.Draw(stripe_mask)
    for y_pos in range(0, size, 8):
        scan_draw.line((0, y_pos, size, y_pos), fill=54, width=2)
    scanlines = Image.new("RGBA", (size, size), (2, 45, 56, 0))
    scanlines.putalpha(ImageChops.multiply(stripe_mask, mask))
    image = Image.alpha_composite(image, scanlines)

    destination = output_dir / f"T_UI_Result_Rank_{letter}.png"
    image.save(destination, format="PNG", optimize=True)
    return destination


def render_preview(background_path: Path, output_dir: Path, destination: Path) -> None:
    with Image.open(background_path) as background:
        preview = background.convert("RGBA")
    with Image.open(output_dir / "T_UI_Result_EvaluationRow.png") as row:
        for y in (250, 370, 490, 610, 730):
            preview.alpha_composite(row, (72, y))
    with Image.open(output_dir / "T_UI_Result_RankPanel.png") as panel:
        preview.alpha_composite(panel, (1300, 214))
    with Image.open(output_dir / "T_UI_Result_Rank_A.png") as rank:
        preview.alpha_composite(rank.resize((300, 300), Image.Resampling.LANCZOS), (1406, 244))
    with Image.open(output_dir / "T_UI_Result_UnlockPanel.png") as unlock:
        preview.alpha_composite(unlock, (72, 878))
    destination.parent.mkdir(parents=True, exist_ok=True)
    preview.convert("RGB").save(destination, format="PNG", optimize=True)


def validate(outputs: list[Path]) -> None:
    expected = {
        "T_UI_Result_Background.png": ((1920, 1080), "RGB"),
        "T_UI_Result_EvaluationRow.png": ((1024, 112), "RGBA"),
        "T_UI_Result_RankPanel.png": ((512, 640), "RGBA"),
        "T_UI_Result_UnlockPanel.png": ((768, 112), "RGBA"),
        **{f"T_UI_Result_Rank_{letter}.png": ((512, 512), "RGBA") for letter in "SABCD"},
    }
    for output in outputs:
        with Image.open(output) as image:
            assert (image.size, image.mode) == expected[output.name], output
            if image.mode == "RGBA":
                alpha = image.getchannel("A")
                assert alpha.getextrema()[0] == 0, f"{output.name} has no transparent pixels"


def write_manifest(output_dir: Path, outputs: list[Path]) -> None:
    entries = []
    for output in outputs:
        with Image.open(output) as image:
            entries.append(
                {
                    "asset_name": output.stem,
                    "source": output.relative_to(ROOT).as_posix(),
                    "size": list(image.size),
                    "mode": image.mode,
                    "unreal_destination": "/Game/UI/Textures/Result",
                    "texture_group": "UI",
                    "mip_gen_settings": "NoMipmaps",
                    "never_stream": True,
                    "srgb": True,
                }
            )
    manifest = {"version": 1, "textures": entries}
    destination = ROOT / "assets" / "ui_result" / "result_ui_manifest.json"
    destination.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    args = parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)
    outputs = [
        render_background(args.background_source, args.output_dir),
        render_evaluation_row(args.output_dir),
        render_rank_panel(args.output_dir),
        render_unlock_panel(args.output_dir),
    ]
    outputs.extend(render_rank(letter, args.output_dir) for letter in "SABCD")
    validate(outputs)
    write_manifest(args.output_dir, outputs)
    render_preview(outputs[0], args.output_dir, args.preview)
    for output in outputs:
        print(output)
    print(args.preview)


if __name__ == "__main__":
    main()
