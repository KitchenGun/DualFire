"""F22/EuroFighter 7포즈 Paper2D 에셋을 생성한다.

실행: Unreal Editor Python 콘솔에서
    py E:/DualFire/Content/Python/create_player_aircraft_2d.py
"""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir()))
SOURCE_ROOT = PROJECT_ROOT / "ArtPipeline" / "assets" / "player_aircraft_2d"
FRAME_NAMES = (
    "Right45",
    "Right30",
    "Right15",
    "Neutral",
    "Left15",
    "Left30",
    "Left45",
)


def _import_texture(source: Path, destination: str, name: str):
    task = unreal.AssetImportTask()
    task.set_editor_property("automated", True)
    task.set_editor_property("destination_name", name)
    task.set_editor_property("destination_path", destination)
    task.set_editor_property("filename", str(source))
    task.set_editor_property("replace_existing", True)
    task.set_editor_property("replace_existing_settings", True)
    task.set_editor_property("save", False)
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])

    texture = unreal.load_asset(f"{destination}/{name}")
    if texture is None:
        raise RuntimeError(f"텍스처 가져오기 실패: {source}")

    texture.set_editor_property("filter", unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
    texture.set_editor_property("srgb", True)
    return texture


def _create_or_update_sprite(folder: str, name: str, texture, frame_index: int):
    asset_path = f"{folder}/{name}"
    sprite = unreal.load_asset(asset_path)
    if sprite is None:
        sprite = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name,
            folder,
            unreal.PaperSprite,
            unreal.PaperSpriteFactory(),
        )
    if sprite is None:
        raise RuntimeError(f"스프라이트 생성 실패: {asset_path}")

    sprite.set_editor_property("source_texture", texture)
    sprite.set_editor_property("source_uv", unreal.Vector2D(frame_index * 128, 0))
    sprite.set_editor_property("source_dimension", unreal.Vector2D(128, 128))
    sprite.set_editor_property("source_texture_dimension", unreal.Vector2D(896, 128))
    sprite.set_editor_property("pixels_per_unreal_unit", 1.0)
    sprite.set_editor_property("pivot_mode", unreal.SpritePivotMode.CENTER_CENTER)
    return sprite


def _create_or_update_flipbook(folder: str, name: str, sprites):
    asset_path = f"{folder}/{name}"
    flipbook = unreal.load_asset(asset_path)
    if flipbook is None:
        flipbook = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name,
            folder,
            unreal.PaperFlipbook,
            unreal.PaperFlipbookFactory(),
        )
    if flipbook is None:
        raise RuntimeError(f"Flipbook 생성 실패: {asset_path}")

    key_frames = []
    for sprite in sprites:
        key_frame = unreal.PaperFlipbookKeyFrame()
        key_frame.set_editor_property("sprite", sprite)
        key_frame.set_editor_property("frame_run", 1)
        key_frames.append(key_frame)
    flipbook.set_editor_property("frames_per_second", 1.0)
    flipbook.set_editor_property("key_frames", key_frames)
    return flipbook


def _validate_aircraft_assets(asset_id: str, sheet, flipbook, sprites):
    if flipbook.get_num_frames() != 7:
        raise RuntimeError(f"{asset_id} Flipbook 프레임 수 오류: {flipbook.get_num_frames()}")
    if sheet.get_editor_property("filter") != unreal.TextureFilter.TF_NEAREST:
        raise RuntimeError(f"{asset_id} 텍스처 필터가 Nearest가 아님")
    if sheet.get_editor_property("mip_gen_settings") != unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS:
        raise RuntimeError(f"{asset_id} 텍스처 밉맵이 비활성화되지 않음")

    for index, (pose, sprite) in enumerate(zip(FRAME_NAMES, sprites)):
        expected_name = f"SPR_{asset_id}_{pose}"
        if sprite.get_name() != expected_name:
            raise RuntimeError(f"{asset_id} 프레임 이름 오류: {sprite.get_name()} != {expected_name}")
        if flipbook.get_sprite_at_frame(index) != sprite:
            raise RuntimeError(f"{asset_id} Flipbook 프레임 순서 오류: {index} {pose}")
        expected_uv = unreal.Vector2D(index * 128, 0)
        if sprite.get_editor_property("source_uv") != expected_uv:
            raise RuntimeError(f"{asset_id} 스프라이트 UV 오류: {pose}")

    unreal.log(f"[Aircraft2D] {asset_id}: frame order and texture settings validated")


def _build_aircraft(asset_id: str, source_name: str, icon_name: str):
    folder = f"/Game/Player/Aircraft/{asset_id}"
    unreal.EditorAssetLibrary.make_directory(folder)

    sheet = _import_texture(
        SOURCE_ROOT / "input" / source_name,
        folder,
        f"T_{asset_id}_BankSheet",
    )
    icon = _import_texture(
        SOURCE_ROOT / "output" / "icons" / icon_name,
        folder,
        f"T_{asset_id}_Icon",
    )

    sprites = [
        _create_or_update_sprite(folder, f"SPR_{asset_id}_{pose}", sheet, index)
        for index, pose in enumerate(FRAME_NAMES)
    ]
    flipbook = _create_or_update_flipbook(folder, f"PFB_{asset_id}_Bank", sprites)
    _validate_aircraft_assets(asset_id, sheet, flipbook, sprites)

    paths = [sheet.get_path_name(), icon.get_path_name(), flipbook.get_path_name()]
    paths.extend(sprite.get_path_name() for sprite in sprites)
    if not unreal.EditorAssetLibrary.save_loaded_assets([sheet, icon, flipbook, *sprites], only_if_is_dirty=False):
        raise RuntimeError(f"에셋 저장 실패: {asset_id}")
    unreal.log(f"[Aircraft2D] {asset_id}: {len(paths)} assets saved")
    return paths


def main():
    all_paths = []
    all_paths.extend(_build_aircraft("F22", "F22_128x128_sheet.png", "F22_Neutral.png"))
    all_paths.extend(
        _build_aircraft(
            "EuroFighter",
            "EuroFighter_128x128_sheet.png",
            "EuroFighter_Neutral.png",
        )
    )
    unreal.log_warning(f"AIRCRAFT2D_RESULT created_or_updated={len(all_paths)}")


main()
