from __future__ import annotations

import argparse
import json
import re
from pathlib import Path
from typing import Any

from PIL import Image, ImageDraw, ImageFont


PROJECT_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_MANIFEST = (
    PROJECT_ROOT
    / "ArtPipeline"
    / "assets"
    / "ui_input_glyphs"
    / "input_glyph_manifest.json"
)
DEFAULT_OUTPUT_DIR = DEFAULT_MANIFEST.parent / "output" / "textures"
ASSET_NAME_PATTERN = re.compile(r"^T_UI_Input_(Keyboard|Mouse|Xbox)_[A-Za-z0-9]+$")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="DualFire 공용 입력 글리프를 렌더링한다.")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST)
    parser.add_argument("--output-dir", type=Path, default=DEFAULT_OUTPUT_DIR)
    parser.add_argument("--validate-only", action="store_true")
    return parser.parse_args()


def load_manifest(path: Path) -> dict[str, Any]:
    with path.open("r", encoding="utf-8") as file:
        return json.load(file)


def validate_manifest(manifest: dict[str, Any]) -> list[dict[str, Any]]:
    entries = manifest.get("entries")
    if not isinstance(entries, list) or not entries:
        raise ValueError("manifest.entries가 비어 있습니다.")

    expected_counts = manifest.get("expected_counts", {})
    actual_counts: dict[str, int] = {}
    asset_names: set[str] = set()
    mapped_keys: set[tuple[str, str]] = set()

    for index, entry in enumerate(entries):
        prefix = f"entries[{index}]"
        required = {
            "device",
            "asset_name",
            "unreal_fkey",
            "display",
            "renderer",
            "source_size",
            "brush_size",
        }
        missing = sorted(required - entry.keys())
        if missing:
            raise ValueError(f"{prefix} 필수 필드 누락: {', '.join(missing)}")

        asset_name = entry["asset_name"]
        if not ASSET_NAME_PATTERN.fullmatch(asset_name):
            raise ValueError(f"{prefix} 잘못된 에셋 이름: {asset_name}")
        if asset_name in asset_names:
            raise ValueError(f"중복 에셋 이름: {asset_name}")
        asset_names.add(asset_name)

        source_size = entry["source_size"]
        brush_size = entry["brush_size"]
        if (
            not isinstance(source_size, list)
            or len(source_size) != 2
            or min(source_size) <= 0
            or not isinstance(brush_size, list)
            or len(brush_size) != 2
            or min(brush_size) <= 0
        ):
            raise ValueError(f"{prefix} 크기 정보가 올바르지 않습니다.")

        device = entry["device"]
        actual_counts[device] = actual_counts.get(device, 0) + 1
        unreal_fkey = entry["unreal_fkey"]
        if entry.get("map_to_controller_data", True):
            if not unreal_fkey:
                raise ValueError(f"{prefix} 매핑 대상에 Unreal FKey가 없습니다.")
            key_id = (device, unreal_fkey)
            if key_id in mapped_keys:
                raise ValueError(f"Controller Data 중복 키: {device}/{unreal_fkey}")
            mapped_keys.add(key_id)

    if expected_counts and actual_counts != expected_counts:
        raise ValueError(f"글리프 수 불일치: expected={expected_counts}, actual={actual_counts}")

    return entries


def rgba(hex_color: str) -> tuple[int, int, int, int]:
    value = hex_color.removeprefix("#")
    if len(value) == 6:
        value += "FF"
    if len(value) != 8:
        raise ValueError(f"잘못된 RGBA 색상: {hex_color}")
    return tuple(int(value[index : index + 2], 16) for index in range(0, 8, 2))  # type: ignore[return-value]


def load_font(size: int, bold: bool = False) -> ImageFont.FreeTypeFont | ImageFont.ImageFont:
    names = ["consolab.ttf", "DejaVuSansMono-Bold.ttf"] if bold else ["consola.ttf", "DejaVuSansMono.ttf"]
    roots = [Path("C:/Windows/Fonts"), Path("D:/UE_5.8/Engine/Content/Slate/Fonts")]
    for root in roots:
        for name in names:
            path = root / name
            if path.exists():
                return ImageFont.truetype(path, size=size)
    return ImageFont.load_default()


def centered_text(
    draw: ImageDraw.ImageDraw,
    box: tuple[float, float, float, float],
    text: str,
    font: ImageFont.FreeTypeFont | ImageFont.ImageFont,
    fill: tuple[int, int, int, int],
) -> None:
    bounds = draw.textbbox((0, 0), text, font=font)
    width = bounds[2] - bounds[0]
    height = bounds[3] - bounds[1]
    x = box[0] + ((box[2] - box[0]) - width) / 2 - bounds[0]
    y = box[1] + ((box[3] - box[1]) - height) / 2 - bounds[1]
    draw.text((x, y), text, font=font, fill=fill)


def chamfered_box(
    draw: ImageDraw.ImageDraw,
    box: tuple[int, int, int, int],
    cut: int,
    fill: tuple[int, int, int, int],
    outline: tuple[int, int, int, int],
    width: int,
) -> None:
    left, top, right, bottom = box
    points = [
        (left + cut, top),
        (right - cut, top),
        (right, top + cut),
        (right, bottom - cut),
        (right - cut, bottom),
        (left + cut, bottom),
        (left, bottom - cut),
        (left, top + cut),
    ]
    draw.polygon(points, fill=fill)
    draw.line(points + [points[0]], fill=outline, width=width, joint="curve")


def draw_arrow(
    draw: ImageDraw.ImageDraw,
    center: tuple[float, float],
    direction: str,
    length: float,
    color: tuple[int, int, int, int],
    width: int,
) -> None:
    x, y = center
    vectors = {
        "up": (0.0, -1.0),
        "down": (0.0, 1.0),
        "left": (-1.0, 0.0),
        "right": (1.0, 0.0),
    }
    dx, dy = vectors[direction]
    start = (x - dx * length * 0.45, y - dy * length * 0.45)
    end = (x + dx * length * 0.45, y + dy * length * 0.45)
    draw.line((start, end), fill=color, width=width)
    px, py = -dy, dx
    wing = length * 0.22
    back = length * 0.22
    draw.polygon(
        [
            end,
            (end[0] - dx * back + px * wing, end[1] - dy * back + py * wing),
            (end[0] - dx * back - px * wing, end[1] - dy * back - py * wing),
        ],
        fill=color,
    )


def render_keyboard(entry: dict[str, Any], colors: dict[str, tuple[int, int, int, int]]) -> Image.Image:
    size = tuple(entry["source_size"])
    image = Image.new("RGBA", size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    margin = max(8, round(min(size) * 0.10))
    box = (margin, margin, size[0] - margin - 1, size[1] - margin - 1)
    chamfered_box(draw, box, max(5, round(min(size) * 0.065)), colors["fill"], colors["outline"], 3)
    draw.line(
        (box[0] + 12, box[1] + 9, box[2] - 12, box[1] + 9),
        fill=colors["highlight"],
        width=2,
    )

    renderer = entry["renderer"]
    if renderer.startswith("arrow_"):
        draw_arrow(draw, ((box[0] + box[2]) / 2, (box[1] + box[3]) / 2 + 2), renderer[6:], min(size) * 0.34, colors["text"], 4)
    else:
        label = entry["display"]
        longest = max(len(line) for line in label.split("\n"))
        font_size = min(round(min(size) * 0.29), max(14, round(size[0] / max(longest + 1, 4))))
        font = load_font(font_size, bold=False)
        centered_text(draw, box, label, font, colors["text"])
    return image


def mouse_body(draw: ImageDraw.ImageDraw, colors: dict[str, tuple[int, int, int, int]]) -> tuple[int, int, int, int]:
    body = (39, 18, 91, 107)
    draw.rounded_rectangle(body, radius=24, fill=colors["fill"], outline=colors["outline"], width=3)
    draw.line((65, 19, 65, 54), fill=colors["outline"], width=2)
    draw.line((40, 55, 90, 55), fill=colors["outline"], width=2)
    return body


def render_mouse(entry: dict[str, Any], colors: dict[str, tuple[int, int, int, int]]) -> Image.Image:
    image = Image.new("RGBA", tuple(entry["source_size"]), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    renderer = entry["renderer"]

    if renderer in {"move", "move_x", "move_y"}:
        directions = {
            "move": ("up", "down", "left", "right"),
            "move_x": ("left", "right"),
            "move_y": ("up", "down"),
        }[renderer]
        for direction in directions:
            center = {
                "up": (64, 37),
                "down": (64, 91),
                "left": (37, 64),
                "right": (91, 64),
            }[direction]
            draw_arrow(draw, center, direction, 28, colors["outline"], 3)
        draw.ellipse((58, 58, 70, 70), outline=colors["outline"], width=2)
        return image

    body = mouse_body(draw, colors)
    if renderer == "button_left":
        draw.pieslice((body[0] + 2, body[1] + 2, 65, 65), 180, 270, fill=colors["active"])
        draw.rectangle((body[0] + 2, 39, 64, 53), fill=colors["active"])
    elif renderer == "button_right":
        draw.pieslice((65, body[1] + 2, body[2] - 2, 65), 270, 360, fill=colors["active"])
        draw.rectangle((66, 39, body[2] - 2, 53), fill=colors["active"])
    elif renderer in {"button_middle", "wheel_axis"}:
        draw.rounded_rectangle((60, 25, 70, 48), radius=3, fill=colors["active"])
    elif renderer in {"thumb_upper", "thumb_lower"}:
        upper = (29, 47, 40, 61)
        lower = (29, 66, 40, 80)
        draw.rounded_rectangle(upper, radius=3, fill=colors["active"] if renderer == "thumb_upper" else colors["fill"], outline=colors["outline"], width=2)
        draw.rounded_rectangle(lower, radius=3, fill=colors["active"] if renderer == "thumb_lower" else colors["fill"], outline=colors["outline"], width=2)
    elif renderer == "wheel_up":
        draw.rounded_rectangle((60, 25, 70, 48), radius=3, fill=colors["active"])
        draw_arrow(draw, (65, 13), "up", 18, colors["active"], 2)
    elif renderer == "wheel_down":
        draw.rounded_rectangle((60, 25, 70, 48), radius=3, fill=colors["active"])
        draw_arrow(draw, (65, 117), "down", 18, colors["active"], 2)

    # 채움 뒤에 외곽선을 다시 그려 작은 크기에서도 본체 경계가 유지되게 한다.
    draw.rounded_rectangle(body, radius=24, outline=colors["outline"], width=3)
    draw.line((65, 19, 65, 54), fill=colors["outline"], width=2)
    draw.line((40, 55, 90, 55), fill=colors["outline"], width=2)
    return image


def dpad_points(center: tuple[int, int], arm: int, half: int) -> list[tuple[int, int]]:
    x, y = center
    return [
        (x - half, y - arm),
        (x + half, y - arm),
        (x + half, y - half),
        (x + arm, y - half),
        (x + arm, y + half),
        (x + half, y + half),
        (x + half, y + arm),
        (x - half, y + arm),
        (x - half, y + half),
        (x - arm, y + half),
        (x - arm, y - half),
        (x - half, y - half),
    ]


def dpad_active_polygon(center: tuple[int, int], direction: str, arm: int, half: int) -> list[tuple[int, int]]:
    x, y = center
    polygons = {
        "up": [(x - half + 2, y - arm + 2), (x + half - 2, y - arm + 2), (x + half - 2, y - half), (x - half + 2, y - half)],
        "down": [(x - half + 2, y + half), (x + half - 2, y + half), (x + half - 2, y + arm - 2), (x - half + 2, y + arm - 2)],
        "left": [(x - arm + 2, y - half + 2), (x - half, y - half + 2), (x - half, y + half - 2), (x - arm + 2, y + half - 2)],
        "right": [(x + half, y - half + 2), (x + arm - 2, y - half + 2), (x + arm - 2, y + half - 2), (x + half, y + half - 2)],
    }
    return polygons[direction]


def render_xbox(entry: dict[str, Any], colors: dict[str, tuple[int, int, int, int]]) -> Image.Image:
    image = Image.new("RGBA", tuple(entry["source_size"]), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    renderer = entry["renderer"]
    label = entry["display"]

    if renderer == "face":
        draw.ellipse((22, 22, 106, 106), fill=colors["fill"], outline=colors["outline"], width=3)
        centered_text(draw, (22, 22, 106, 106), label, load_font(42, bold=True), colors["text"])
    elif renderer.startswith("dpad_"):
        direction = renderer[5:]
        points = dpad_points((64, 64), 43, 17)
        draw.polygon(points, fill=colors["fill"], outline=colors["outline"])
        draw.line(points + [points[0]], fill=colors["outline"], width=3, joint="curve")
        draw.polygon(dpad_active_polygon((64, 64), direction, 43, 17), fill=colors["active"])
        draw.line(points + [points[0]], fill=colors["outline"], width=3, joint="curve")
    elif renderer in {"shoulder", "trigger"}:
        chamfered_box(draw, (14, 35, 114, 91), 10, colors["fill"], colors["outline"], 3)
        centered_text(draw, (14, 35, 114, 91), label, load_font(28, bold=True), colors["text"])
    elif renderer in {"menu", "view"}:
        draw.rounded_rectangle((22, 38, 106, 90), radius=8, fill=colors["fill"], outline=colors["outline"], width=3)
        if renderer == "menu":
            for y in (52, 64, 76):
                draw.line((45, y, 83, y), fill=colors["text"], width=3)
        else:
            draw.rounded_rectangle((43, 49, 70, 72), radius=3, outline=colors["text"], width=3)
            draw.rounded_rectangle((57, 56, 84, 79), radius=3, outline=colors["text"], width=3)
    elif renderer.startswith("stick_click"):
        draw.ellipse((18, 18, 110, 110), fill=colors["fill"], outline=colors["outline"], width=3)
        draw.ellipse((31, 31, 97, 97), outline=colors["muted"], width=2)
        centered_text(draw, (31, 31, 97, 97), label, load_font(25, bold=True), colors["text"])
    elif renderer.startswith("stick_direction_"):
        direction = renderer.rsplit("_", 1)[-1]
        draw.ellipse((22, 22, 106, 106), fill=colors["fill"], outline=colors["outline"], width=3)
        draw_arrow(draw, (64, 64), direction, 42, colors["active"], 4)
    elif renderer.startswith("stick_axis_"):
        axis = renderer.rsplit("_", 1)[-1]
        directions = ("left", "right") if axis == "x" else ("up", "down")
        for direction in directions:
            center = {"left": (38, 64), "right": (90, 64), "up": (64, 38), "down": (64, 90)}[direction]
            draw_arrow(draw, center, direction, 27, colors["outline"], 3)
        draw.ellipse((58, 58, 70, 70), outline=colors["outline"], width=2)
    elif renderer == "stick_2d":
        for direction, center in (("up", (64, 38)), ("down", (64, 90)), ("left", (38, 64)), ("right", (90, 64))):
            draw_arrow(draw, center, direction, 27, colors["outline"], 3)
        draw.ellipse((58, 58, 70, 70), outline=colors["outline"], width=2)
    else:
        raise ValueError(f"지원하지 않는 Xbox renderer: {renderer}")
    return image


def render_entry(entry: dict[str, Any], colors: dict[str, tuple[int, int, int, int]]) -> Image.Image:
    device = entry["device"]
    if device == "Keyboard":
        return render_keyboard(entry, colors)
    if device == "Mouse":
        return render_mouse(entry, colors)
    if device == "Xbox":
        return render_xbox(entry, colors)
    raise ValueError(f"지원하지 않는 device: {device}")


def validate_output(path: Path, expected_size: list[int]) -> None:
    with Image.open(path) as image:
        image.verify()
    with Image.open(path) as image:
        if image.mode != "RGBA":
            raise ValueError(f"{path.name}: RGBA가 아닙니다({image.mode}).")
        if list(image.size) != expected_size:
            raise ValueError(f"{path.name}: 크기 불일치({image.size}).")
        alpha = image.getchannel("A")
        alpha_extrema = alpha.getextrema()
        if alpha_extrema[0] != 0 or alpha_extrema[1] == 0:
            raise ValueError(f"{path.name}: 투명 여백 또는 불투명 픽셀이 없습니다.")


def main() -> None:
    args = parse_args()
    manifest = load_manifest(args.manifest)
    entries = validate_manifest(manifest)
    colors = {name: rgba(value) for name, value in manifest["palette"].items()}

    if args.validate_only:
        print(f"manifest ok: {len(entries)} glyphs")
        return

    args.output_dir.mkdir(parents=True, exist_ok=True)
    expected_files: set[Path] = set()
    for entry in entries:
        output = args.output_dir / f"{entry['asset_name']}.png"
        render_entry(entry, colors).save(output, format="PNG", optimize=True)
        validate_output(output, entry["source_size"])
        expected_files.add(output.resolve())

    for stale_file in args.output_dir.glob("T_UI_Input_*.png"):
        if stale_file.resolve() not in expected_files:
            stale_file.unlink()

    print(f"rendered {len(entries)} glyphs: {args.output_dir}")


if __name__ == "__main__":
    main()
