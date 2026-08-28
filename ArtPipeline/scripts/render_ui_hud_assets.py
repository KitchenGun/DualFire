"""Render tintable modular textures and previews for the in-game HUD."""

from __future__ import annotations

import json
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
OUTPUT_DIR = ROOT / "assets" / "ui_hud" / "output" / "textures"
MANIFEST_PATH = ROOT / "assets" / "ui_hud" / "ui_hud_manifest.json"
PREVIEW_DIR = ROOT.parent / "Saved" / "DesignPreviews" / "InGameHUD"
FONT_PATH = Path("C:/Windows/Fonts/bahnschrift.ttf")

WHITE = (255, 255, 255, 255)
WHITE_DIM = (255, 255, 255, 150)
CYAN = (82, 231, 242, 255)
CYAN_DIM = (49, 128, 142, 255)
TEXT = (231, 244, 248, 255)
TEXT_DIM = (121, 151, 161, 255)
SHIELD = (90, 184, 212, 255)
AMBER = (232, 180, 74, 255)
RED = (225, 91, 100, 255)
PANEL = (3, 10, 15, 190)


TEXTURE_SPECS = {
    "T_UI_HUD_LifeIcon": (128, 128),
    "T_UI_HUD_HealthSegment": (128, 32),
    "T_UI_HUD_ShieldSegment": (128, 32),
    "T_UI_HUD_WeaponSlotFrame": (256, 192),
    "T_UI_HUD_ChainGaugeFrame": (512, 32),
    "T_UI_HUD_BossBarFrame": (1024, 64),
    "T_UI_HUD_SystemBannerFrame": (768, 160),
}


def font(size: int) -> ImageFont.FreeTypeFont:
    return ImageFont.truetype(str(FONT_PATH), size=size)


def save_texture(name: str, image: Image.Image) -> Path:
    assert image.mode == "RGBA"
    assert image.size == TEXTURE_SPECS[name]
    destination = OUTPUT_DIR / f"{name}.png"
    image.save(destination, format="PNG", optimize=True)
    return destination


def render_life_icon() -> Image.Image:
    image = Image.new("RGBA", TEXTURE_SPECS["T_UI_HUD_LifeIcon"], (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    points = ((64, 6), (76, 50), (114, 78), (108, 92), (75, 78), (69, 118),
              (59, 118), (53, 78), (20, 92), (14, 78), (52, 50))
    draw.polygon(points, outline=WHITE)
    draw.line((64, 9, 64, 112), fill=WHITE, width=5)
    draw.line((39, 73, 89, 73), fill=WHITE_DIM, width=2)
    return image


def render_health_segment() -> Image.Image:
    image = Image.new("RGBA", TEXTURE_SPECS["T_UI_HUD_HealthSegment"], (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.polygon(((2, 4), (116, 4), (126, 16), (116, 28), (2, 28)), fill=WHITE)
    draw.line((8, 8, 108, 8), fill=(255, 255, 255, 110), width=2)
    return image


def render_shield_segment() -> Image.Image:
    image = Image.new("RGBA", TEXTURE_SPECS["T_UI_HUD_ShieldSegment"], (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.polygon(((8, 3), (120, 3), (127, 16), (120, 29), (8, 29), (1, 16)), fill=(255, 255, 255, 205))
    draw.polygon(((14, 8), (114, 8), (119, 16), (114, 24), (14, 24), (9, 16)), outline=WHITE, width=2)
    return image


def render_weapon_slot_frame() -> Image.Image:
    image = Image.new("RGBA", TEXTURE_SPECS["T_UI_HUD_WeaponSlotFrame"], (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.line((2, 34, 2, 2, 62, 2), fill=WHITE, width=3)
    draw.line((194, 190, 254, 190, 254, 158), fill=WHITE_DIM, width=2)
    draw.rectangle((16, 50, 104, 138), outline=WHITE_DIM, width=2)
    draw.line((122, 98, 240, 98), fill=WHITE_DIM, width=2)
    draw.line((122, 110, 240, 110), fill=(255, 255, 255, 72), width=1)
    return image


def render_chain_gauge_frame() -> Image.Image:
    image = Image.new("RGBA", TEXTURE_SPECS["T_UI_HUD_ChainGaugeFrame"], (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rectangle((1, 7, 510, 25), outline=WHITE_DIM, width=2)
    draw.line((1, 7, 42, 7), fill=WHITE, width=3)
    draw.line((470, 25, 510, 25), fill=WHITE, width=3)
    return image


def render_boss_bar_frame() -> Image.Image:
    image = Image.new("RGBA", TEXTURE_SPECS["T_UI_HUD_BossBarFrame"], (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rectangle((2, 14, 1021, 51), outline=WHITE_DIM, width=2)
    draw.line((2, 14, 72, 14), fill=WHITE, width=3)
    draw.line((951, 51, 1021, 51), fill=WHITE, width=3)
    draw.line((2, 8, 2, 28), fill=WHITE, width=3)
    draw.line((1021, 37, 1021, 57), fill=WHITE, width=3)
    return image


def render_system_banner_frame() -> Image.Image:
    image = Image.new("RGBA", TEXTURE_SPECS["T_UI_HUD_SystemBannerFrame"], (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    length = 74
    draw.line((2, length, 2, 2, length, 2), fill=WHITE, width=3)
    draw.line((768 - length, 2, 765, 2, 765, length), fill=WHITE_DIM, width=2)
    draw.line((2, 160 - length, 2, 157, length, 157), fill=WHITE_DIM, width=2)
    draw.line((768 - length, 157, 765, 157, 765, 160 - length), fill=WHITE, width=3)
    draw.line((140, 80, 628, 80), fill=(255, 255, 255, 54), width=1)
    return image


def tint(image: Image.Image, color: tuple[int, int, int, int]) -> Image.Image:
    tinted = Image.new("RGBA", image.size, color)
    tinted.putalpha(image.getchannel("A"))
    return tinted


def fit_tinted(name: str, size: tuple[int, int], color) -> Image.Image:
    with Image.open(OUTPUT_DIR / f"{name}.png") as source:
        resized = source.convert("RGBA").resize(size, Image.Resampling.LANCZOS)
    return tint(resized, color)


def add_text(draw: ImageDraw.ImageDraw, xy, value: str, size: int, color=TEXT, anchor=None) -> None:
    draw.text(xy, value, fill=color, font=font(size), anchor=anchor)


def render_contact_sheet() -> Path:
    width, height = 1600, 900
    sheet = Image.new("RGBA", (width, height), (5, 12, 18, 255))
    draw = ImageDraw.Draw(sheet, "RGBA")
    for x in range(0, width, 64):
        draw.line((x, 0, x, height), fill=(30, 82, 91, 32), width=1)
    for y in range(0, height, 64):
        draw.line((0, y, width, y), fill=(30, 82, 91, 32), width=1)

    add_text(draw, (56, 42), "DUALFIRE // HUD RESOURCE LAYERS", 30)
    add_text(draw, (56, 82), "TINTABLE RGBA / NO TEXT / NO DYNAMIC FILL", 15, CYAN_DIM)

    tiles = (
        ("T_UI_HUD_LifeIcon", (56, 130, 376, 370), (128, 128)),
        ("T_UI_HUD_HealthSegment", (396, 130, 746, 370), (256, 64)),
        ("T_UI_HUD_ShieldSegment", (766, 130, 1116, 370), (256, 64)),
        ("T_UI_HUD_WeaponSlotFrame", (1136, 130, 1544, 370), (256, 192)),
        ("T_UI_HUD_ChainGaugeFrame", (56, 410, 746, 620), (512, 32)),
        ("T_UI_HUD_BossBarFrame", (766, 410, 1544, 620), (720, 45)),
        ("T_UI_HUD_SystemBannerFrame", (56, 660, 1544, 850), (768, 160)),
    )
    colors = (CYAN, CYAN, SHIELD, CYAN, AMBER, RED, CYAN)
    for (name, box, display_size), color in zip(tiles, colors):
        left, top, right, bottom = box
        draw.rectangle(box, fill=PANEL)
        draw.line((left, top, left + 52, top), fill=color, width=2)
        add_text(draw, (left + 14, top + 13), name, 14, color)
        image = fit_tinted(name, display_size, color)
        x = left + (right - left - image.width) // 2
        y = top + 48 + (bottom - top - 48 - image.height) // 2
        sheet.alpha_composite(image, (x, y))

    destination = PREVIEW_DIR / "HUD_LayerContactSheet.png"
    sheet.convert("RGB").save(destination, format="PNG", optimize=True)
    return destination


def draw_segments(canvas: Image.Image, x: int, y: int, count: int, color, size=(42, 10), gap=5) -> None:
    segment = fit_tinted("T_UI_HUD_HealthSegment", size, color)
    for index in range(count):
        canvas.alpha_composite(segment, (x + index * (size[0] + gap), y))


def render_placement_preview() -> Path:
    width, height = 1920, 1080
    preview = Image.new("RGBA", (width, height), (5, 12, 18, 255))
    draw = ImageDraw.Draw(preview, "RGBA")
    field_left, field_right = 557, 1363

    for x in range(0, width, 64):
        draw.line((x, 0, x, height), fill=(30, 82, 91, 24), width=1)
    for y in range(0, height, 64):
        draw.line((0, y, width, y), fill=(30, 82, 91, 24), width=1)
    draw.rectangle((field_left, 28, field_right, 1052), outline=(82, 231, 242, 68), width=1)
    add_text(draw, (field_left + 12, 40), "B-2 PLAYFIELD GUIDE / EXPECTED HUD PLACEMENT", 13, CYAN_DIM)
    add_text(draw, (field_left - 18, 540), "NO HUD", 13, TEXT_DIM, anchor="ra")
    add_text(draw, (field_right + 18, 540), "NO HUD", 13, TEXT_DIM)

    # Score and chain, top-left inside the playfield.
    draw.rectangle((581, 78, 750, 182), fill=PANEL)
    add_text(draw, (595, 91), "SCORE", 12, TEXT_DIM)
    add_text(draw, (595, 112), "00284,610", 25)
    add_text(draw, (595, 147), "x1.28", 18, AMBER)
    chain = fit_tinted("T_UI_HUD_ChainGaugeFrame", (140, 9), AMBER)
    preview.alpha_composite(chain, (595, 170))

    # Boss target, top-center.
    draw.rectangle((770, 72, 1339, 166), fill=PANEL)
    add_text(draw, (784, 84), "TARGET // PHASE 02 / 03", 13, RED)
    add_text(draw, (1325, 84), "68%", 13, RED, anchor="ra")
    draw.rectangle((784, 119, 1160, 137), fill=RED)
    boss = fit_tinted("T_UI_HUD_BossBarFrame", (541, 34), RED)
    preview.alpha_composite(boss, (784, 111))
    for ratio in (1 / 3, 2 / 3):
        tick_x = 784 + round(541 * ratio)
        draw.line((tick_x, 111, tick_x, 145), fill=TEXT, width=2)

    # Survival, bottom-left.
    draw.rectangle((581, 850, 820, 1024), fill=PANEL)
    add_text(draw, (595, 862), "HP", 12, TEXT_DIM)
    draw_segments(preview, 595, 886, 5, CYAN)
    add_text(draw, (595, 914), "SHIELD", 12, TEXT_DIM)
    shield = fit_tinted("T_UI_HUD_ShieldSegment", (42, 10), SHIELD)
    preview.alpha_composite(shield, (595, 938))
    preview.alpha_composite(shield, (642, 938))
    life = fit_tinted("T_UI_HUD_LifeIcon", (42, 42), TEXT)
    preview.alpha_composite(life, (595, 968))
    add_text(draw, (646, 981), "x 1", 18)

    # Three weapon slots, bottom-right.
    weapon_info = (
        ("PRI", "READY", CYAN),
        ("SP1", "55%", AMBER),
        ("SP2", "READY", CYAN),
    )
    start_x = 965
    for index, (label, state, color) in enumerate(weapon_info):
        x = start_x + index * 124
        draw.rectangle((x, 850, x + 112, 1024), fill=PANEL)
        slot = fit_tinted("T_UI_HUD_WeaponSlotFrame", (112, 84), color)
        preview.alpha_composite(slot, (x, 864))
        add_text(draw, (x + 12, 872), label, 12, color)
        add_text(draw, (x + 100, 988), state, 11, color, anchor="ra")

    # Transient system message, center.
    draw.rectangle((684, 452, 1236, 582), fill=(3, 10, 15, 150))
    system_frame = fit_tinted("T_UI_HUD_SystemBannerFrame", (552, 115), AMBER)
    preview.alpha_composite(system_frame, (684, 460))
    add_text(draw, (960, 516), "WARNING", 31, AMBER, anchor="mm")
    add_text(draw, (960, 560), "TRANSIENT SYSTEM MESSAGE", 12, TEXT_DIM, anchor="mm")

    destination = PREVIEW_DIR / "HUD_ExpectedPlacement_1920x1080.png"
    preview.convert("RGB").save(destination, format="PNG", optimize=True)
    return destination


def validate(outputs: list[Path]) -> None:
    for output in outputs:
        with Image.open(output) as image:
            assert image.mode == "RGBA", output
            assert image.size == TEXTURE_SPECS[output.stem], output
            assert image.getchannel("A").getextrema() == (0, 255), output


def write_manifest(outputs: list[Path]) -> None:
    textures = []
    for output in outputs:
        with Image.open(output) as image:
            textures.append(
                {
                    "asset_name": output.stem,
                    "source": output.relative_to(ROOT).as_posix(),
                    "size": list(image.size),
                    "mode": image.mode,
                    "unreal_destination": "/Game/UI/Textures/HUD",
                    "texture_group": "UI",
                    "mip_gen_settings": "NoMipmaps",
                    "never_stream": True,
                    "srgb": True,
                    "tintable": True,
                }
            )
    MANIFEST_PATH.parent.mkdir(parents=True, exist_ok=True)
    MANIFEST_PATH.write_text(json.dumps({"version": 1, "textures": textures}, indent=2) + "\n", encoding="utf-8")


def main() -> None:
    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)
    PREVIEW_DIR.mkdir(parents=True, exist_ok=True)
    renderers = {
        "T_UI_HUD_LifeIcon": render_life_icon,
        "T_UI_HUD_HealthSegment": render_health_segment,
        "T_UI_HUD_ShieldSegment": render_shield_segment,
        "T_UI_HUD_WeaponSlotFrame": render_weapon_slot_frame,
        "T_UI_HUD_ChainGaugeFrame": render_chain_gauge_frame,
        "T_UI_HUD_BossBarFrame": render_boss_bar_frame,
        "T_UI_HUD_SystemBannerFrame": render_system_banner_frame,
    }
    outputs = [save_texture(name, renderers[name]()) for name in TEXTURE_SPECS]
    validate(outputs)
    write_manifest(outputs)
    contact_sheet = render_contact_sheet()
    placement_preview = render_placement_preview()
    for output in outputs:
        print(output)
    print(MANIFEST_PATH)
    print(contact_sheet)
    print(placement_preview)


if __name__ == "__main__":
    main()
