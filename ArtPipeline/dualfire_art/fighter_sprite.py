from __future__ import annotations

from pathlib import Path

from PIL import Image, ImageChops, ImageDraw


_BANK_SIGNS = {"left": -1, "neutral": 0, "right": 1}
FLIGHT_STATE_CONFIG = {
    "north_afterburner_off": ("neutral", False),
    "move_left_afterburner_off": ("left", False),
    "move_right_afterburner_off": ("right", False),
    "north_afterburner_on": ("neutral", True),
    "move_left_afterburner_on": ("left", True),
    "move_right_afterburner_on": ("right", True),
}


def load_fitted_reference(path: Path, expected_size: tuple[int, int] | None = None) -> Image.Image:
    image = Image.open(path).convert("RGBA")
    if expected_size is not None and image.size != expected_size:
        raise ValueError(f"Prepared base must be {expected_size[0]}x{expected_size[1]}: {path}")
    if image.getchannel("A").getbbox() is None:
        raise ValueError(f"Prepared base has no visible fighter pixels: {path}")
    return image


def decorate_airframe(base: Image.Image, control_phase: int, afterburner: bool, bank_direction: str) -> Image.Image:
    result = base.copy()
    alpha = result.getchannel("A")
    bounds = alpha.getbbox()
    if bounds is None:
        return result
    bank_sign = _bank_sign(bank_direction)
    left, top, right, bottom = bounds
    center_x = (left + right - 1) // 2
    wing_y = top + int((bottom - top) * 0.58)
    unit = _motion_unit(result)
    tail_y = bottom - max(3 * unit, (bottom - top) // 7)
    span = max(5 * unit, (right - left) // 3)

    control = Image.new("RGBA", result.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(control)
    dark = (17, 29, 45, 255)
    light = (125, 166, 184, 255)
    if control_phase:
        neutral_deflection = 1 if bank_sign == 0 else bank_sign
        left_deflection = unit * neutral_deflection
        right_deflection = unit if bank_sign == 0 else -unit * bank_sign
        flap_length = max(5 * unit, span // 3)
        left_start = center_x - span
        right_end = center_x + span
        draw.line((left_start, wing_y, left_start + flap_length, wing_y + left_deflection), fill=dark, width=unit)
        draw.line((left_start, wing_y + unit, left_start + flap_length, wing_y + unit + left_deflection), fill=light, width=unit)
        draw.line((right_end - flap_length, wing_y + right_deflection, right_end, wing_y), fill=dark, width=unit)
        draw.line((right_end - flap_length, wing_y + unit + right_deflection, right_end, wing_y + unit), fill=light, width=unit)
        draw.line((center_x - 2 * unit, tail_y, center_x - 3 * unit, tail_y - unit * neutral_deflection), fill=dark, width=unit)
        draw.line((center_x + 2 * unit, tail_y, center_x + 3 * unit, tail_y - unit * neutral_deflection), fill=dark, width=unit)
    control.putalpha(ImageChops.multiply(control.getchannel("A"), alpha))
    result.alpha_composite(control)

    if afterburner:
        draw_twin_afterburner(result, bottom, center_x, right - left, control_phase, unit)
    return result


def apply_bank_lean(image: Image.Image, bank_direction: str) -> Image.Image:
    bank_sign = _bank_sign(bank_direction)
    if bank_sign == 0:
        return image
    bounds = image.getchannel("A").getbbox()
    if bounds is None:
        return image
    _, top, _, bottom = bounds
    center_y = (top + bottom - 1) / 2
    half_height = max(1.0, (bottom - top) / 2)
    leaned = Image.new("RGBA", image.size, (0, 0, 0, 0))
    for y in range(image.height):
        lean = round(bank_sign * (center_y - y) / half_height * 2)
        strip = image.crop((0, y, image.width, y + 1))
        leaned.alpha_composite(strip, (lean, y))
    return leaned


def draw_twin_afterburner(image: Image.Image, bottom: int, center_x: int, fuselage_width: int, control_phase: int, unit: int) -> None:
    # Image reference: white core, yellow middle, and orange taper behind each nozzle.
    # Reserve the final two rows for transparent padding in every packed frame.
    length = min((5 if control_phase else 3) * unit, max(0, image.height - 3 * unit - bottom))
    if length == 0:
        return
    flame = Image.new("RGBA", image.size, (0, 0, 0, 0))
    draw = ImageDraw.Draw(flame)
    nozzle_spacing = max(2 * unit, fuselage_width // 8)
    for nozzle_x in (center_x - nozzle_spacing, center_x + nozzle_spacing):
        for step in range(length):
            y = bottom + step
            outer_width = unit if step < length - unit else 0
            draw.line((nozzle_x - outer_width, y, nozzle_x + outer_width, y), fill=(255, 112, 36, 255), width=unit)
            draw.line((nozzle_x, y, nozzle_x, y + unit - 1), fill=(255, 242, 200, 255), width=unit)
        draw.line((nozzle_x, bottom + length, nozzle_x, bottom + length + unit - 1), fill=(255, 188, 74, 255), width=unit)
    image.alpha_composite(flame)


def _bank_sign(bank_direction: str) -> int:
    try:
        return _BANK_SIGNS[bank_direction]
    except KeyError as error:
        raise ValueError(f"Unsupported bank direction: {bank_direction}") from error


def _motion_unit(image: Image.Image) -> int:
    return max(1, round(min(image.size) / 256))


def build_flight_keyframe_frame(base: Image.Image, state: str, frame_index: int) -> Image.Image:
    try:
        bank_direction, afterburner = FLIGHT_STATE_CONFIG[state]
    except KeyError as error:
        raise ValueError(f"Unsupported fighter flight state: {state}") from error
    if frame_index not in {0, 1}:
        raise ValueError(f"Fighter flight frame index must be 0 or 1: {frame_index}")
    return decorate_airframe(base, frame_index, afterburner, bank_direction)
