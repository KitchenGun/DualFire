"""Build Unreal-ready main-menu background and button-state textures."""

from __future__ import annotations

import argparse
from pathlib import Path

from PIL import Image, ImageDraw, ImageFilter, ImageOps


BACKGROUND_SIZE = (1920, 1080)
BUTTON_SIZE = (1024, 128)

BUTTON_STATES = {
    "Normal": {
        "fill": (4, 17, 24, 92),
        "accent": (70, 102, 119, 110),
        "line": (72, 115, 135, 55),
    },
    "Hovered": {
        "fill": (4, 22, 30, 102),
        "accent": (65, 132, 150, 128),
        "line": (60, 139, 157, 72),
    },
    "Focused": {
        "fill": (2, 35, 45, 148),
        "accent": (65, 224, 244, 255),
        "line": (61, 214, 240, 155),
    },
    "Pressed": {
        "fill": (1, 47, 58, 178),
        "accent": (180, 248, 255, 255),
        "line": (95, 234, 249, 190),
    },
    "Disabled": {
        "fill": (8, 14, 18, 64),
        "accent": (81, 93, 100, 78),
        "line": (82, 93, 99, 38),
    },
}


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--background-source", type=Path, required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    return parser.parse_args()


def render_background(source: Path, output_dir: Path) -> Path:
    with Image.open(source) as image:
        background = ImageOps.fit(
            image.convert("RGB"),
            BACKGROUND_SIZE,
            method=Image.Resampling.LANCZOS,
        )
        destination = output_dir / "T_UI_MainMenu_Background.png"
        background.save(destination, format="PNG", optimize=True)
        return destination


def horizontal_fade(color: tuple[int, int, int, int]) -> Image.Image:
    width, height = BUTTON_SIZE
    image = Image.new("RGBA", BUTTON_SIZE)
    pixels = image.load()
    for x in range(width):
        fade = max(0.0, 1.0 - (x / width) ** 1.7)
        alpha = round(color[3] * fade)
        for y in range(height):
            pixels[x, y] = (*color[:3], alpha)
    return image


def glow_layer(accent: tuple[int, int, int, int], intensity: float) -> Image.Image:
    mask = Image.new("L", BUTTON_SIZE)
    draw = ImageDraw.Draw(mask)
    draw.rounded_rectangle((8, 14, 20, 114), radius=2, fill=round(220 * intensity))
    blurred = mask.filter(ImageFilter.GaussianBlur(radius=10))
    glow = Image.new("RGBA", BUTTON_SIZE, (*accent[:3], 0))
    glow.putalpha(blurred)
    return glow


def render_button(state: str, values: dict[str, tuple[int, int, int, int]], output_dir: Path) -> Path:
    image = horizontal_fade(values["fill"])
    glow_intensity = {
        "Normal": 0.20,
        "Hovered": 0.28,
        "Focused": 1.00,
        "Pressed": 1.00,
        "Disabled": 0.10,
    }
    intensity = glow_intensity[state]
    image = Image.alpha_composite(image, glow_layer(values["accent"], intensity))

    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((10, 17, 17, 111), radius=2, fill=values["accent"])
    draw.line((18, 64, 1014, 64), fill=values["line"], width=2)
    draw.line((18, 112, 610, 112), fill=(*values["line"][:3], values["line"][3] // 2), width=1)

    destination = output_dir / f"T_UI_MenuButton_{state}.png"
    image.save(destination, format="PNG", optimize=True)
    return destination


def main() -> None:
    args = parse_args()
    args.output_dir.mkdir(parents=True, exist_ok=True)

    outputs = [render_background(args.background_source, args.output_dir)]
    outputs.extend(
        render_button(state, values, args.output_dir)
        for state, values in BUTTON_STATES.items()
    )

    for output in outputs:
        with Image.open(output) as image:
            print(f"{output}: {image.size[0]}x{image.size[1]} {image.mode}")


if __name__ == "__main__":
    main()
