"""Import generated mission-result textures with Unreal UI settings."""

from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir())
SOURCE_DIR = PROJECT_ROOT / "ArtPipeline" / "assets" / "ui_result" / "output" / "textures"
DESTINATION_PATH = "/Game/UI/Textures/Result"


def configure_texture(texture: unreal.Texture2D) -> None:
    texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property("never_stream", True)
    texture.set_editor_property("srgb", True)
    texture.modify()
    assert texture.get_editor_property("lod_group") == unreal.TextureGroup.TEXTUREGROUP_UI
    assert texture.get_editor_property("mip_gen_settings") == unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
    assert texture.get_editor_property("never_stream") is True
    assert texture.get_editor_property("srgb") is True


def main() -> None:
    unreal.EditorAssetLibrary.make_directory(DESTINATION_PATH)
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()

    for source in sorted(SOURCE_DIR.glob("T_UI_Result_*.png")):
        asset_path = f"{DESTINATION_PATH}/{source.stem}"
        texture = None
        if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
            texture = unreal.EditorAssetLibrary.load_asset(asset_path)
        else:
            task = unreal.AssetImportTask()
            task.set_editor_property("filename", str(source))
            task.set_editor_property("destination_path", DESTINATION_PATH)
            task.set_editor_property("destination_name", source.stem)
            task.set_editor_property("automated", True)
            task.set_editor_property("replace_existing", False)
            task.set_editor_property("save", False)
            asset_tools.import_asset_tasks([task])
            texture = unreal.EditorAssetLibrary.load_asset(asset_path)

        if not isinstance(texture, unreal.Texture2D):
            raise RuntimeError(f"Texture import failed: {asset_path}")

        configure_texture(texture)
        if not unreal.EditorAssetLibrary.save_loaded_asset(texture, only_if_is_dirty=False):
            raise RuntimeError(f"Texture save failed: {asset_path}")
        unreal.log(f"Result UI texture ready: {asset_path}")


if __name__ == "__main__":
    main()
