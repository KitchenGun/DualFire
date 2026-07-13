from __future__ import annotations

import warnings
from pathlib import Path

from PIL import Image, ImageChops, ImageFilter, UnidentifiedImageError

from .errors import PipelineError
from .util import require_new_path, resolve_within, sha256_file


MAX_FILE_SIZE = 50 * 1024 * 1024
MAX_DIMENSION = 8192
SUPPORTED_EXTENSIONS = {".png", ".jpg", ".jpeg", ".webp"}
SUPPORTED_FORMATS = {"PNG", "JPEG", "WEBP"}


def threshold_to_binary_mask(image: Image.Image, threshold: int) -> Image.Image:
    """Return an L mask with only fully transparent or fully opaque pixels."""
    if not 0 <= threshold <= 255:
        raise PipelineError("threshold must be between 0 and 255")
    return image.convert("L").point(lambda value: 255 if value >= threshold else 0, mode="L")


def resize_nearest(image: Image.Image, size: tuple[int, int]) -> Image.Image:
    """Resize to an exact pixel size without interpolating source colors."""
    if len(size) != 2 or any(not isinstance(value, int) or value < 1 for value in size):
        raise PipelineError("resize size must contain two positive integers")
    return image.resize(size, Image.Resampling.NEAREST)


def quantize_limited_palette(image: Image.Image, palette_limit: int) -> Image.Image:
    """Quantize RGB colors with a bounded palette and no dithering."""
    if not 1 <= palette_limit <= 256:
        raise PipelineError("palette_limit must be between 1 and 256")
    return image.convert("RGB").quantize(
        colors=palette_limit,
        method=Image.Quantize.MEDIANCUT,
        dither=Image.Dither.NONE,
    )


def composite_rgba_on_rgb_background(subject: Image.Image, background: Image.Image) -> Image.Image:
    """Composite a same-sized subject onto an RGB background without resizing either image."""
    if background.mode != "RGB":
        raise PipelineError("background must use RGB mode")
    if subject.size != background.size:
        raise PipelineError("subject and background must have the same size")
    return Image.alpha_composite(background.convert("RGBA"), subject.convert("RGBA")).convert("RGB")


def prepare_source(asset_dir: Path, asset: dict, force: bool = False) -> dict[str, str]:
    source_config = asset["source"]
    source_path = resolve_within(asset_dir, source_config["file"])
    validate_input_file(source_path)

    normalized_dir = asset_dir / "work" / "normalized"
    normalized_dir.mkdir(parents=True, exist_ok=True)
    normalized_path = normalized_dir / "source.png"
    mask_path = normalized_dir / "mask.png"
    require_new_path(normalized_path, force)
    require_new_path(mask_path, force)

    image = load_rgba(source_path)
    alpha = resolve_source_alpha(asset_dir, image, source_config)

    threshold = int(asset["target"]["alpha_threshold"])
    binary_mask = threshold_to_binary_mask(alpha, threshold)
    normalized = image.copy()
    normalized.putalpha(binary_mask)
    normalized.save(normalized_path, format="PNG", optimize=False)
    binary_mask.save(mask_path, format="PNG", optimize=False)
    hashes = {
        "source_sha256": sha256_file(source_path),
        "normalized_sha256": sha256_file(normalized_path),
        "mask_sha256": sha256_file(mask_path),
    }
    frame_fit = source_config.get("frame_fit")
    if frame_fit:
        fitted_path = normalized_dir / "fitted.png"
        fitted_mask_path = normalized_dir / "fitted_mask.png"
        require_new_path(fitted_path, force)
        require_new_path(fitted_mask_path, force)
        fitted = fit_to_frame(
            normalized,
            binary_mask,
            (int(asset["target"]["cell_width"]), int(asset["target"]["cell_height"])),
            int(frame_fit["padding"]),
        )
        fitted.save(fitted_path, format="PNG", optimize=False)
        threshold_to_binary_mask(fitted.getchannel("A"), threshold).save(fitted_mask_path, format="PNG", optimize=False)
        hashes["fitted_sha256"] = sha256_file(fitted_path)
        hashes["fitted_mask_sha256"] = sha256_file(fitted_mask_path)
    return hashes


def resolve_source_alpha(asset_dir: Path, image: Image.Image, source_config: dict) -> Image.Image:
    mask_value = source_config.get("mask_file")
    if mask_value:
        return load_mask(resolve_within(asset_dir, mask_value), image.size)

    native_alpha = image.getchannel("A")
    chroma_key = source_config.get("chroma_key")
    if chroma_key:
        chroma_alpha = chroma_key_mask(
            image.convert("RGB"),
            tuple(chroma_key["color"]),
            int(chroma_key["tolerance"]),
        )
        return ImageChops.multiply(native_alpha, chroma_alpha)

    minimum, maximum = native_alpha.getextrema()
    if minimum == maximum == 255:
        raise PipelineError("Opaque input requires source.mask_file or source.chroma_key")
    return native_alpha


def chroma_key_mask(image: Image.Image, color: tuple[int, int, int], tolerance: int) -> Image.Image:
    threshold = tolerance * tolerance
    mask = Image.new("L", image.size, 0)
    pixels = image.load()
    mask_pixels = mask.load()
    for y in range(image.height):
        for x in range(image.width):
            red, green, blue = pixels[x, y]
            distance_squared = (red - color[0]) ** 2 + (green - color[1]) ** 2 + (blue - color[2]) ** 2
            mask_pixels[x, y] = 0 if distance_squared <= threshold else 255
    return mask


def fit_to_frame(source: Image.Image, mask: Image.Image, size: tuple[int, int], padding: int) -> Image.Image:
    bounds = mask.getbbox()
    if bounds is None:
        raise PipelineError("Source mask contains no visible pixels")
    cropped = source.crop(bounds)
    available_width = size[0] - padding * 2
    available_height = size[1] - padding * 2
    scale = min(available_width / cropped.width, available_height / cropped.height)
    fitted_size = (
        max(1, round(cropped.width * scale)),
        max(1, round(cropped.height * scale)),
    )
    fitted = resize_nearest(cropped, fitted_size)
    canvas = Image.new("RGBA", size, (0, 0, 0, 0))
    canvas.alpha_composite(fitted, ((size[0] - fitted.width) // 2, (size[1] - fitted.height) // 2))
    return canvas


def validate_input_file(path: Path) -> None:
    if not path.is_file():
        raise PipelineError(f"Missing source image: {path}")
    if path.is_symlink():
        raise PipelineError(f"Symbolic links are not accepted as input: {path}")
    if path.suffix.lower() not in SUPPORTED_EXTENSIONS:
        raise PipelineError(f"Unsupported image extension: {path.suffix}")
    if path.stat().st_size > MAX_FILE_SIZE:
        raise PipelineError(f"Image exceeds 50 MB limit: {path}")
    try:
        with warnings.catch_warnings():
            warnings.simplefilter("error", Image.DecompressionBombWarning)
            with Image.open(path) as image:
                if image.format not in SUPPORTED_FORMATS:
                    raise PipelineError(f"Unsupported image format: {image.format}")
                width, height = image.size
                if width > MAX_DIMENSION or height > MAX_DIMENSION:
                    raise PipelineError(f"Image exceeds {MAX_DIMENSION}x{MAX_DIMENSION}: {path}")
                image.verify()
    except (UnidentifiedImageError, OSError, Image.DecompressionBombError, Image.DecompressionBombWarning) as error:
        raise PipelineError(f"Invalid image file: {path}: {error}") from error


def load_rgba(path: Path) -> Image.Image:
    try:
        with Image.open(path) as image:
            return image.convert("RGBA")
    except (UnidentifiedImageError, OSError) as error:
        raise PipelineError(f"Unable to load image: {path}: {error}") from error


def load_mask(path: Path, expected_size: tuple[int, int]) -> Image.Image:
    validate_input_file(path)
    with Image.open(path) as image:
        mask = image.convert("L")
    if mask.size != expected_size:
        raise PipelineError(f"Mask size {mask.size} does not match source size {expected_size}")
    return mask


def pixelize(
    image: Image.Image,
    mask: Image.Image,
    size: tuple[int, int],
    palette_limit: int,
    alpha_threshold: int,
    reserved_palette_colors: list[tuple[int, int, int]] | None = None,
) -> Image.Image:
    rgba = image.convert("RGBA").resize(size, Image.Resampling.LANCZOS)
    rgb = rgba.convert("RGB")
    resized_mask = resize_nearest(mask, size)
    binary_alpha = threshold_to_binary_mask(resized_mask, alpha_threshold)
    visible_bounds = binary_alpha.getbbox()
    if visible_bounds is None:
        return Image.new("RGBA", size, (0, 0, 0, 0))

    # Transparent pixels can contain chroma-key RGB values; they must not consume palette entries.
    reserved = reserved_palette_colors or []
    palette_source = quantize_limited_palette(rgb.crop(visible_bounds), palette_limit - len(reserved))
    palette_colors = build_palette_colors(palette_source, reserved)
    result = map_to_palette(rgb, palette_colors).convert("RGBA")
    result.putalpha(binary_alpha)
    return result


def stylize_source_preserving_pixel_art(
    image: Image.Image,
    pixel_scale: int = 2,
    palette_limit: int = 64,
    outline_width: int = 0,
) -> Image.Image:
    """Apply a pixel-art finish without changing the source silhouette or framing."""
    if pixel_scale < 1:
        raise PipelineError("pixel_scale must be positive")
    if not 2 <= palette_limit <= 256:
        raise PipelineError("palette_limit must be between 2 and 256")
    if outline_width < 0:
        raise PipelineError("outline_width cannot be negative")

    rgba = image.convert("RGBA")
    alpha = rgba.getchannel("A").point(lambda value: 255 if value else 0, mode="L")
    if alpha.getbbox() is None:
        raise PipelineError("Source-preserving pixel art requires visible source pixels")

    reduced_size = (
        max(1, rgba.width // pixel_scale),
        max(1, rgba.height // pixel_scale),
    )
    reduced_rgb = rgba.convert("RGB").resize(reduced_size, Image.Resampling.BOX)
    reduced = quantize_limited_palette(reduced_rgb, palette_limit).convert("RGBA")
    sprite = resize_nearest(reduced, rgba.size)
    # Color clusters are reduced, but collision and sprite framing keep the source alpha exactly.
    sprite.putalpha(alpha)

    if outline_width == 0:
        return sprite
    outline_alpha = sprite.getchannel("A").filter(ImageFilter.MaxFilter(outline_width * 2 + 1))
    outlined = Image.new("RGBA", rgba.size, (20, 25, 31, 0))
    outlined.putalpha(outline_alpha)
    outlined.alpha_composite(sprite)
    return outlined


def build_palette_colors(source: Image.Image, reserved: list[tuple[int, int, int]]) -> list[tuple[int, int, int]]:
    palette_data = source.getpalette()
    assert palette_data is not None
    used_indices = sorted(index for _, index in source.getcolors(maxcolors=source.width * source.height) or [])
    body_colors = [tuple(palette_data[index * 3 : index * 3 + 3]) for index in used_indices]
    return list(dict.fromkeys([*reserved, *body_colors]))


def map_to_palette(image: Image.Image, palette: list[tuple[int, int, int]]) -> Image.Image:
    mapped: dict[tuple[int, int, int], tuple[int, int, int]] = {}

    def nearest(color: tuple[int, int, int]) -> tuple[int, int, int]:
        if color not in mapped:
            mapped[color] = min(
                palette,
                key=lambda candidate: sum((channel - reference) ** 2 for channel, reference in zip(color, candidate)),
            )
        return mapped[color]

    result = Image.new("RGB", image.size)
    pixels = image.get_flattened_data() if hasattr(image, "get_flattened_data") else image.getdata()
    result.putdata([nearest(color) for color in pixels])
    return result


def offset_frame(image: Image.Image, offset: tuple[int, int]) -> Image.Image:
    canvas = Image.new("RGBA", image.size, (0, 0, 0, 0))
    canvas.alpha_composite(image, offset)
    return canvas


def composite_with_mask(base: Image.Image, candidate: Image.Image, mask: Image.Image) -> Image.Image:
    candidate = candidate.convert("RGBA").resize(base.size, Image.Resampling.LANCZOS)
    binary_mask = mask.convert("L").resize(base.size, Image.Resampling.NEAREST)
    return Image.composite(candidate, base.convert("RGBA"), binary_mask)


def count_visible_colors(image: Image.Image) -> int:
    rgba = image.convert("RGBA")
    colors = rgba.getcolors(maxcolors=rgba.width * rgba.height) or []
    return len({color[:3] for _, color in colors if color[3] > 0})


def canonicalize_qwen_candidate(
    image: Image.Image,
    size: tuple[int, int],
    chroma_key: dict[str, object] | None,
    mask: Image.Image | None,
    padding: int,
    palette_limit: int,
    alpha_threshold: int,
    reserved_palette_colors: list[tuple[int, int, int]],
    color_mode: str = "indexed",
    preserve_canvas: bool = False,
) -> Image.Image:
    """Convert an approved Qwen candidate into a stable, engine-ready sprite keyframe."""
    rgba = image.convert("RGBA")
    if mask is not None:
        alpha = mask.convert("L")
        if alpha.size != rgba.size:
            raise PipelineError("Candidate mask size does not match the approved candidate")
    elif chroma_key is not None:
        color = chroma_key.get("color")
        tolerance = chroma_key.get("tolerance")
        assert isinstance(color, list) and isinstance(tolerance, int)
        alpha = ImageChops.multiply(rgba.getchannel("A"), chroma_key_mask(rgba.convert("RGB"), tuple(color), tolerance))
    else:
        alpha = rgba.getchannel("A")
        if alpha.getextrema() == (255, 255):
            raise PipelineError("Opaque Qwen candidates require canonical.chroma_key or --mask")

    binary_alpha = threshold_to_binary_mask(alpha, alpha_threshold)
    if binary_alpha.getbbox() is None:
        raise PipelineError("Qwen candidate has no visible pixels after alpha extraction")
    rgba.putalpha(binary_alpha)
    fitted = rgba if preserve_canvas and rgba.size == size else fit_to_frame(rgba, binary_alpha, size, padding)
    if color_mode == "preserve":
        fitted.putalpha(threshold_to_binary_mask(fitted.getchannel("A"), alpha_threshold))
        return fitted
    if color_mode != "indexed":
        raise PipelineError(f"Unsupported target.color_mode: {color_mode}")
    return pixelize(
        fitted,
        fitted.getchannel("A"),
        size,
        palette_limit,
        alpha_threshold,
        reserved_palette_colors,
    )
