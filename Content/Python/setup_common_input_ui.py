"""DualFire Common Input 글리프와 UI 입력 에셋을 구성한다."""

from __future__ import annotations

import json
from pathlib import Path

import unreal


PROJECT_ROOT = Path(unreal.Paths.project_dir()).resolve()
MANIFEST_PATH = (
    PROJECT_ROOT
    / "ArtPipeline"
    / "assets"
    / "ui_input_glyphs"
    / "input_glyph_manifest.json"
)
OUTPUT_DIR = MANIFEST_PATH.parent / "output" / "textures"

INPUT_FOLDER = "/Game/Input/UI"
INPUT_DATA_FOLDER = "/Game/Blueprint/UI/Input"
STYLE_FOLDER = "/Game/Blueprint/UI/Style"

IA_CONFIRM_PATH = f"{INPUT_FOLDER}/IA_UI_Confirm"
IA_BACK_PATH = f"{INPUT_FOLDER}/IA_UI_Back"
IMC_MENU_PATH = f"{INPUT_FOLDER}/IMC_UI_Menu"
INPUT_DATA_PATH = f"{INPUT_DATA_FOLDER}/BP_CommonUIInputData"
KEYBOARD_DATA_PATH = f"{INPUT_DATA_FOLDER}/BP_CommonInputControllerData_KeyboardMouse"
XBOX_DATA_PATH = f"{INPUT_DATA_FOLDER}/BP_CommonInputControllerData_Xbox"
ACTION_TEXT_STYLE_PATH = f"{STYLE_FOLDER}/BP_ActionTextStyle"
ACTION_BUTTON_STYLE_PATH = f"{STYLE_FOLDER}/BP_ActionButtonStyle"

START_ROOT_PATH = "/Game/Blueprint/UI/Root/WBP_StartRoot"
START_MENU_PATH = "/Game/Blueprint/UI/Screen/WBP_StartMenu"
SETTINGS_PATH = "/Game/Blueprint/UI/Screen/WBP_Settings"
EXIT_CONFIRM_PATH = "/Game/Blueprint/UI/Screen/WBP_ExitConfirm"
ACTION_BAR_PATH = "/Game/Blueprint/UI/Components/WBP_ActionBar"
START_PLAYER_CONTROLLER_PATH = "/Game/Blueprint/UI/PlayerController/BP_PC_Start"


def log(message: str) -> None:
    unreal.log(f"[DF_INPUT_SETUP] {message}")


def fail(message: str) -> None:
    raise RuntimeError(f"[DF_INPUT_SETUP] {message}")


def load_manifest() -> dict:
    if not MANIFEST_PATH.is_file():
        fail(f"매니페스트를 찾을 수 없습니다: {MANIFEST_PATH}")

    manifest = json.loads(MANIFEST_PATH.read_text(encoding="utf-8"))
    entries = manifest.get("entries", [])
    expected_total = sum(manifest["expected_counts"].values())
    if len(entries) != expected_total:
        fail(f"글리프 수가 일치하지 않습니다: {len(entries)} != {expected_total}")

    asset_names = [entry["asset_name"] for entry in entries]
    if len(asset_names) != len(set(asset_names)):
        fail("중복된 Texture2D 에셋 이름이 있습니다.")

    return manifest


def import_textures(manifest: dict) -> dict[str, unreal.Texture2D]:
    import_settings = manifest["unreal_import"]
    destination = import_settings["folder"]
    tasks = []

    for entry in manifest["entries"]:
        source_file = OUTPUT_DIR / f"{entry['asset_name']}.png"
        if not source_file.is_file():
            fail(f"생성된 PNG가 없습니다: {source_file}")

        task = unreal.AssetImportTask()
        task.set_editor_property("filename", str(source_file))
        task.set_editor_property("destination_path", destination)
        task.set_editor_property("destination_name", entry["asset_name"])
        task.set_editor_property("automated", True)
        task.set_editor_property("replace_existing", True)
        task.set_editor_property("replace_existing_settings", False)
        task.set_editor_property("save", False)
        tasks.append(task)

    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

    textures: dict[str, unreal.Texture2D] = {}
    for entry in manifest["entries"]:
        asset_path = f"{destination}/{entry['asset_name']}"
        texture = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(texture, unreal.Texture2D):
            fail(f"Texture2D 임포트에 실패했습니다: {asset_path}")

        texture.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        texture.set_editor_property(
            "mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS
        )
        texture.set_editor_property("never_stream", True)
        texture.set_editor_property("srgb", True)
        texture.modify()
        textures[entry["asset_name"]] = texture

    log(f"Texture2D {len(textures)}개를 임포트했습니다.")
    return textures


def load_or_create_asset(asset_path: str, asset_class, factory):
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing is not None:
        if not isinstance(existing, asset_class):
            fail(f"기존 에셋의 타입이 다릅니다: {asset_path}")
        return existing

    folder, asset_name = asset_path.rsplit("/", 1)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, folder, asset_class, factory
    )
    if asset is None:
        fail(f"에셋 생성에 실패했습니다: {asset_path}")
    return asset


def load_or_create_blueprint(asset_path: str, parent_class) -> unreal.Blueprint:
    existing = unreal.EditorAssetLibrary.load_asset(asset_path)
    if existing is not None:
        if not isinstance(existing, unreal.Blueprint):
            fail(f"기존 에셋이 Blueprint가 아닙니다: {asset_path}")
        return existing

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    return load_or_create_asset(asset_path, unreal.Blueprint, factory)


def generated_class(asset_path: str):
    blueprint = unreal.EditorAssetLibrary.load_asset(asset_path)
    if not isinstance(blueprint, unreal.Blueprint):
        fail(f"Blueprint를 찾을 수 없습니다: {asset_path}")
    return blueprint.generated_class()


def make_key(key_name: str) -> unreal.Key:
    key = unreal.Key()
    key.set_editor_property("key_name", unreal.Name(key_name))
    return key


def configure_input_assets():
    confirm = load_or_create_asset(
        IA_CONFIRM_PATH, unreal.InputAction, unreal.InputAction_Factory()
    )
    back = load_or_create_asset(IA_BACK_PATH, unreal.InputAction, unreal.InputAction_Factory())
    mapping = load_or_create_asset(
        IMC_MENU_PATH, unreal.InputMappingContext, unreal.InputMappingContext_Factory()
    )

    confirm.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)
    back.set_editor_property("value_type", unreal.InputActionValueType.BOOLEAN)

    mapping.unmap_all()
    mapping.map_key(confirm, make_key("Enter"))
    mapping.map_key(confirm, make_key("Gamepad_FaceButton_Bottom"))
    mapping.map_key(back, make_key("Escape"))
    mapping.map_key(back, make_key("Gamepad_FaceButton_Right"))

    input_data_bp = load_or_create_blueprint(INPUT_DATA_PATH, unreal.CommonUIInputData)
    input_data_cdo = unreal.get_default_object(input_data_bp.generated_class())
    input_data_cdo.set_editor_property("enhanced_input_click_action", confirm)
    input_data_cdo.set_editor_property("enhanced_input_back_action", back)

    log("IA_UI_Confirm, IA_UI_Back, IMC_UI_Menu, BP_CommonUIInputData를 구성했습니다.")
    return confirm, back, mapping


def make_brush(texture: unreal.Texture2D, brush_size: list[int]) -> unreal.SlateBrush:
    brush = unreal.SlateBrush()
    image_size = unreal.DeprecateSlateVector2D()
    image_size.set_editor_property("x", float(brush_size[0]))
    image_size.set_editor_property("y", float(brush_size[1]))
    brush.set_editor_property("resource_object", texture)
    brush.set_editor_property("image_size", image_size)
    return brush


def configure_controller_data(manifest: dict, textures: dict[str, unreal.Texture2D]) -> None:
    controller_parent = unreal.CommonInputBaseControllerData
    keyboard_bp = load_or_create_blueprint(KEYBOARD_DATA_PATH, controller_parent)
    xbox_bp = load_or_create_blueprint(XBOX_DATA_PATH, controller_parent)
    keyboard_cdo = unreal.get_default_object(keyboard_bp.generated_class())
    xbox_cdo = unreal.get_default_object(xbox_bp.generated_class())

    keyboard_configs = []
    xbox_configs = []
    mapped_keys: dict[str, set[str]] = {"KeyboardMouse": set(), "Xbox": set()}

    for entry in manifest["entries"]:
        if entry.get("map_to_controller_data", True) is False:
            continue

        key_name = entry.get("unreal_fkey")
        if not key_name:
            fail(f"FKey가 없는 매핑 대상입니다: {entry['asset_name']}")

        group = "Xbox" if entry["device"] == "Xbox" else "KeyboardMouse"
        if key_name in mapped_keys[group]:
            fail(f"Controller Data에 중복 FKey가 있습니다: {group}/{key_name}")
        mapped_keys[group].add(key_name)

        config = unreal.CommonInputKeyBrushConfiguration()
        config.set_editor_property("key", make_key(key_name))
        config.set_editor_property(
            "key_brush", make_brush(textures[entry["asset_name"]], entry["brush_size"])
        )
        if group == "Xbox":
            xbox_configs.append(config)
        else:
            keyboard_configs.append(config)

    keyboard_cdo.set_editor_property(
        "input_type", unreal.CommonInputType.MOUSE_AND_KEYBOARD
    )
    keyboard_cdo.set_editor_property("input_brush_data_map", keyboard_configs)

    xbox_cdo.set_editor_property("input_type", unreal.CommonInputType.GAMEPAD)
    xbox_cdo.set_editor_property("gamepad_name", unreal.Name("Generic"))
    xbox_cdo.set_editor_property("gamepad_display_name", "Xbox")
    xbox_cdo.set_editor_property("input_brush_data_map", xbox_configs)

    log(
        "Controller Data를 구성했습니다: "
        f"KeyboardMouse={len(keyboard_configs)}, Xbox={len(xbox_configs)}"
    )


def transparent_brush() -> unreal.SlateBrush:
    brush = unreal.SlateBrush()
    brush.set_editor_property(
        "tint_color",
        unreal.SlateColor(specified_color=unreal.LinearColor(0.0, 0.0, 0.0, 0.0)),
    )
    return brush


def configure_action_styles() -> None:
    text_bp = load_or_create_blueprint(ACTION_TEXT_STYLE_PATH, unreal.CommonTextStyle)
    text_cdo = unreal.get_default_object(text_bp.generated_class())
    text_cdo.set_editor_property("color", unreal.LinearColor(0.73, 0.88, 1.0, 1.0))
    font_asset = unreal.EditorAssetLibrary.load_asset("/Engine/EngineFonts/Roboto")
    if not font_asset:
        fail("Action Bar 폰트 에셋을 찾을 수 없습니다: /Engine/EngineFonts/Roboto")

    font = text_cdo.get_editor_property("font")
    font.set_editor_property(
        "font_object", font_asset
    )
    font.set_editor_property("typeface_font_name", unreal.Name("Regular"))
    font.set_editor_property("size", 22)
    text_cdo.set_editor_property("font", font)

    button_bp = load_or_create_blueprint(ACTION_BUTTON_STYLE_PATH, unreal.CommonButtonStyle)
    button_cdo = unreal.get_default_object(button_bp.generated_class())
    for property_name in (
        "normal_base",
        "normal_hovered",
        "normal_pressed",
        "selected_base",
        "selected_hovered",
        "selected_pressed",
        "disabled",
    ):
        button_cdo.set_editor_property(property_name, transparent_brush())

    text_style_class = text_bp.generated_class()
    for property_name in (
        "normal_text_style",
        "normal_hovered_text_style",
        "selected_text_style",
        "selected_hovered_text_style",
        "disabled_text_style",
    ):
        button_cdo.set_editor_property(property_name, text_style_class)

    button_cdo.set_editor_property("button_padding", unreal.Margin(8.0, 4.0, 8.0, 4.0))
    button_cdo.set_editor_property("min_height", 36)
    log("BP_ActionTextStyle과 BP_ActionButtonStyle을 구성했습니다.")


def configure_widget_defaults(confirm, mapping) -> None:
    exit_class = generated_class(EXIT_CONFIRM_PATH)
    action_bar_class = generated_class(ACTION_BAR_PATH)

    start_menu = unreal.get_default_object(generated_class(START_MENU_PATH))
    start_menu.set_editor_property("ConfirmInputAction", confirm)
    start_menu.set_editor_property("InputMapping", None)
    start_menu.set_editor_property("ExitConfirmWidgetClass", exit_class)
    start_menu.set_editor_property("bIsBackHandler", True)
    start_menu.set_editor_property("bIsBackActionDisplayedInActionBar", True)
    start_menu.set_editor_property("OverrideBackActionDisplayName", "BACK")

    settings = unreal.get_default_object(generated_class(SETTINGS_PATH))
    settings.set_editor_property("ConfirmInputAction", confirm)
    settings.set_editor_property("InputMapping", None)
    settings.set_editor_property("bIsBackHandler", True)
    settings.set_editor_property("bIsBackActionDisplayedInActionBar", True)
    settings.set_editor_property("OverrideBackActionDisplayName", "BACK")

    exit_confirm = unreal.get_default_object(exit_class)
    exit_confirm.set_editor_property("ConfirmInputAction", confirm)
    exit_confirm.set_editor_property("InputMapping", None)
    exit_confirm.set_editor_property("bIsBackHandler", True)
    exit_confirm.set_editor_property("bIsBackActionDisplayedInActionBar", True)
    exit_confirm.set_editor_property("OverrideBackActionDisplayName", "BACK")
    exit_confirm.set_editor_property("bIsModal", True)

    start_root = unreal.get_default_object(generated_class(START_ROOT_PATH))
    start_root.set_editor_property("ActionBarWidgetClass", action_bar_class)

    start_controller = unreal.get_default_object(
        generated_class(START_PLAYER_CONTROLLER_PATH)
    )
    start_controller.set_editor_property("UIInputMapping", mapping)
    start_controller.set_editor_property("UIInputMappingPriority", 100)
    log("시작 PlayerController에 UI 입력 매핑을 연결하고 화면별 중복 매핑을 제거했습니다.")


def save_assets(manifest: dict) -> None:
    texture_folder = manifest["unreal_import"]["folder"]
    asset_paths = [
        IA_CONFIRM_PATH,
        IA_BACK_PATH,
        IMC_MENU_PATH,
        INPUT_DATA_PATH,
        KEYBOARD_DATA_PATH,
        XBOX_DATA_PATH,
        ACTION_TEXT_STYLE_PATH,
        ACTION_BUTTON_STYLE_PATH,
        START_ROOT_PATH,
        START_MENU_PATH,
        SETTINGS_PATH,
        EXIT_CONFIRM_PATH,
        START_PLAYER_CONTROLLER_PATH,
    ]
    asset_paths.extend(
        f"{texture_folder}/{entry['asset_name']}" for entry in manifest["entries"]
    )

    failures = [
        path
        for path in asset_paths
        if not unreal.EditorAssetLibrary.save_asset(path, only_if_is_dirty=False)
    ]
    if failures:
        fail(f"저장에 실패한 에셋이 있습니다: {failures[:5]}")
    log(f"에셋 {len(asset_paths)}개를 저장했습니다.")


def main() -> None:
    editor_subsystem = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
    get_game_world = getattr(editor_subsystem, "get_game_world", None)
    if callable(get_game_world) and get_game_world() is not None:
        fail("PIE 실행 중에는 Common Input 에셋을 변경하지 않습니다.")

    manifest = load_manifest()
    textures = import_textures(manifest)
    confirm, _back, mapping = configure_input_assets()
    configure_controller_data(manifest, textures)
    configure_action_styles()
    configure_widget_defaults(confirm, mapping)
    save_assets(manifest)
    log("Common Input UI 구성이 완료되었습니다.")


if __name__ == "__main__":
    main()
