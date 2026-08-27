"""정식 1미션 CommonUI 화면과 Mission DataTable 참조를 구성한다."""

import json

import unreal


SCREEN_DIR = "/Game/Blueprint/UI/Screen"
CAMPAIGN_PATH = f"{SCREEN_DIR}/WBP_CampaignMap"
BRIEFING_PATH = f"{SCREEN_DIR}/WBP_MissionBriefing"
HANGAR_PATH = f"{SCREEN_DIR}/WBP_Hangar"
START_MENU_PATH = f"{SCREEN_DIR}/WBP_StartMenu"
RESULT_PATH = f"{SCREEN_DIR}/WBP_MissionResult"
MENU_BUTTON_PATH = "/Game/Blueprint/UI/Components/WBP_MenuButton"
START_CONTROLLER_PATH = "/Game/Blueprint/UI/PlayerController/BP_PC_Start"
GAME_INSTANCE_PATH = "/Game/Blueprint/GameInstance/BP_GameInstance"
MISSION_TABLE_PATH = "/Game/Data/Mission/DT_Missions"
STAGE_TABLE_PATH = "/Game/Data/Stage/DT_Stages"
UMG = unreal.get_default_object(unreal.UMGToolSet)


def fail(message):
    raise RuntimeError(f"[DF_MISSION_FLOW] {message}")


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        fail(f"Required asset not found: {path}")
    return asset


def create_widget_blueprint(path, parent_class):
    existing = unreal.load_asset(path)
    if existing is not None:
        return existing
    folder, name = path.rsplit("/", 1)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, folder, unreal.WidgetBlueprint, factory
    )
    if asset is None:
        fail(f"Widget Blueprint creation failed: {path}")
    return asset


def add_widget(blueprint, widget_class, name, parent=None, child_index=-1):
    info = UMG.call_method(
        "AddWidget", (blueprint, widget_class, name, parent, child_index)
    )
    if info.widget is None:
        fail(f"Widget creation failed: {blueprint.get_name()}/{name}")
    return info.widget, info.slot


def add_canvas_widget(blueprint, canvas, widget_class, name, position, size):
    widget, slot = add_widget(blueprint, widget_class, name, canvas)
    slot.set_position(unreal.Vector2D(*position))
    slot.set_size(unreal.Vector2D(*size))
    return widget


def add_full_screen(blueprint, canvas, widget_class, name):
    widget, slot = add_widget(blueprint, widget_class, name, canvas)
    slot.set_anchors(
        unreal.Anchors(
            minimum=unreal.Vector2D(0.0, 0.0),
            maximum=unreal.Vector2D(1.0, 1.0),
        )
    )
    slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
    return widget


def set_variable(blueprint, widget):
    UMG.call_method("ToggleWidgetAsVariable", (blueprint, widget, True))


def set_text(widget, value, size):
    widget.set_text(value)
    font = widget.get_editor_property("font")
    font.set_editor_property("size", size)
    widget.set_editor_property("font", font)


def find_widget(blueprint, name):
    for info in UMG.call_method("GetWidgets", (blueprint,)).widgets:
        if info.widget is not None and info.widget.get_name() == name:
            return info.widget
    return None


def add_text(blueprint, canvas, name, position, size, value, font_size, variable=True):
    widget = add_canvas_widget(
        blueprint, canvas, unreal.CommonTextBlock, name, position, size
    )
    if variable:
        set_variable(blueprint, widget)
    set_text(widget, value, font_size)
    return widget


def add_button(blueprint, canvas, button_class, name, position, size):
    widget = add_canvas_widget(blueprint, canvas, button_class, name, position, size)
    set_variable(blueprint, widget)
    return widget


def wrap_reference_resolution(blueprint, canvas):
    size_infos = UMG.call_method("WrapWidgets", (blueprint, [canvas], unreal.SizeBox))
    if len(size_infos) != 1 or size_infos[0].widget is None:
        fail(f"SizeBox wrapping failed: {blueprint.get_name()}")
    reference = size_infos[0].widget
    reference.set_width_override(1920.0)
    reference.set_height_override(1080.0)

    scale_infos = UMG.call_method("WrapWidgets", (blueprint, [reference], unreal.ScaleBox))
    if len(scale_infos) != 1 or scale_infos[0].widget is None:
        fail(f"ScaleBox wrapping failed: {blueprint.get_name()}")
    scale_infos[0].widget.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)


def build_campaign(blueprint, button_class):
    if UMG.call_method("GetWidgets", (blueprint,)).info.widget_count > 0:
        return
    root, _ = add_widget(blueprint, unreal.CanvasPanel, "CampaignRoot")
    background = add_full_screen(blueprint, root, unreal.Image, "CampaignBackground")
    background.set_brush_from_texture(
        load_required("/Game/UI/Textures/Result/T_UI_Result_Background"), True
    )
    add_text(blueprint, root, "CampaignTitle", (112.0, 72.0), (900.0, 80.0), "CAMPAIGN MAP", 54, False)
    add_text(blueprint, root, "CampaignSubtitle", (118.0, 150.0), (900.0, 42.0), "SELECT SORTIE AREA", 22, False)
    add_button(blueprint, root, button_class, "MissionButton", (160.0, 300.0), (480.0, 84.0))
    add_text(blueprint, root, "Text_MissionName", (810.0, 268.0), (860.0, 68.0), "CHARGE ASSAULT", 42)
    add_text(blueprint, root, "Text_Environment", (814.0, 366.0), (700.0, 42.0), "SKY  /  GROUND", 24)
    add_text(blueprint, root, "Text_Difficulty", (814.0, 422.0), (420.0, 42.0), "NORMAL", 24)
    add_text(blueprint, root, "Text_Status", (814.0, 520.0), (820.0, 90.0), "", 20)
    add_button(blueprint, root, button_class, "BackButton", (112.0, 918.0), (300.0, 72.0))
    wrap_reference_resolution(blueprint, root)


def build_briefing(blueprint, button_class):
    if UMG.call_method("GetWidgets", (blueprint,)).info.widget_count > 0:
        return
    root, _ = add_widget(blueprint, unreal.CanvasPanel, "BriefingRoot")
    background = add_full_screen(blueprint, root, unreal.Image, "BriefingBackground")
    background.set_brush_from_texture(
        load_required("/Game/UI/Textures/Result/T_UI_Result_Background"), True
    )
    add_text(blueprint, root, "BriefingTitle", (112.0, 62.0), (900.0, 60.0), "MISSION BRIEFING", 46, False)
    add_text(blueprint, root, "Text_MissionName", (116.0, 136.0), (1320.0, 56.0), "MISSION 01  //  CHARGE ASSAULT", 30)
    add_text(blueprint, root, "Text_Environment", (116.0, 210.0), (760.0, 42.0), "SKY  /  GROUND", 22)
    add_text(blueprint, root, "Text_Difficulty", (940.0, 210.0), (360.0, 42.0), "NORMAL", 22)
    add_text(blueprint, root, "BriefingLabel", (116.0, 306.0), (360.0, 36.0), "BRIEFING", 18, False)
    add_text(blueprint, root, "Text_Briefing", (116.0, 348.0), (1480.0, 130.0), "", 24)
    add_text(blueprint, root, "ObjectiveLabel", (116.0, 506.0), (360.0, 36.0), "OBJECTIVE", 18, False)
    add_text(blueprint, root, "Text_Objective", (116.0, 548.0), (1480.0, 96.0), "", 24)
    add_text(blueprint, root, "EnemyHintLabel", (116.0, 674.0), (420.0, 36.0), "ENEMY COMPOSITION", 18, False)
    add_text(blueprint, root, "Text_EnemyHint", (116.0, 716.0), (1480.0, 72.0), "", 24)
    add_text(blueprint, root, "Text_Status", (116.0, 802.0), (980.0, 50.0), "", 18)
    add_button(blueprint, root, button_class, "BackButton", (112.0, 920.0), (260.0, 68.0))
    add_button(blueprint, root, button_class, "SkipButton", (1320.0, 920.0), (220.0, 68.0))
    add_button(blueprint, root, button_class, "ContinueButton", (1560.0, 920.0), (260.0, 68.0))
    wrap_reference_resolution(blueprint, root)


def extend_result_screen(blueprint, button_class):
    canvas = find_widget(blueprint, "ResultRoot")
    if canvas is None:
        fail("WBP_MissionResult ResultRoot CanvasPanel not found")
    if find_widget(blueprint, "Text_FailureReason") is None:
        add_text(
            blueprint,
            canvas,
            "Text_FailureReason",
            (1392.0, 692.0),
            (410.0, 52.0),
            "STAGE CONDITION FAILED",
            22,
        )
    if find_widget(blueprint, "MissionSelectButton") is None:
        add_button(
            blueprint, canvas, button_class, "MissionSelectButton", (1210.0, 880.0), (300.0, 72.0)
        )
    if find_widget(blueprint, "ReplayButton") is None:
        add_button(
            blueprint, canvas, button_class, "ReplayButton", (1530.0, 880.0), (260.0, 72.0)
        )


def compile_and_save(blueprint):
    if not UMG.call_method("CompileWidgetBlueprint", (blueprint,)):
        fail(f"Widget Blueprint compile failed: {blueprint.get_path_name()}")
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        fail(f"Widget Blueprint save failed: {blueprint.get_path_name()}")


def create_mission_table():
    existing = unreal.load_asset(MISSION_TABLE_PATH)
    if existing is None:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", unreal.MissionRow.static_struct())
        folder, name = MISSION_TABLE_PATH.rsplit("/", 1)
        existing = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            name, folder, unreal.DataTable, factory
        )
    if not isinstance(existing, unreal.DataTable):
        fail("DT_Missions creation failed")

    rows = [
        {
            "Name": "MISSION_01",
            "MissionID": "MISSION_01",
            "MissionCode": "MISSION 01",
            "DisplayName": "CHARGE ASSAULT",
            "EnvironmentTags": ["SKY", "GROUND"],
            "NormalDifficultyID": "NORMAL",
            "BriefingText": "Advance through the combat zone and secure the stage route.",
            "ObjectiveText": "Survive the assault and reach the final arrival point.",
            "EnemyCompositionHint": "Ground and air formations are expected.",
            "StageID": "STAGE_TEST",
            "MissionLevel": "/Game/Level/LV_Test.LV_Test",
        }
    ]
    imported = unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(
        existing, json.dumps(rows)
    )
    if not imported:
        fail("DT_Missions import failed")
    unreal.EditorAssetLibrary.save_loaded_asset(existing, only_if_is_dirty=False)
    return existing


def configure_defaults(campaign, briefing, mission_table):
    hangar = load_required(HANGAR_PATH)
    start_menu = load_required(START_MENU_PATH)
    result = load_required(RESULT_PATH)
    start_controller = load_required(START_CONTROLLER_PATH)
    game_instance = load_required(GAME_INSTANCE_PATH)

    unreal.get_default_object(start_menu.generated_class()).set_editor_property(
        "campaign_widget_class", campaign.generated_class()
    )
    unreal.get_default_object(campaign.generated_class()).set_editor_property(
        "briefing_widget_class", briefing.generated_class()
    )
    unreal.get_default_object(briefing.generated_class()).set_editor_property(
        "hangar_widget_class", hangar.generated_class()
    )
    controller_cdo = unreal.get_default_object(start_controller.generated_class())
    controller_cdo.set_editor_property("campaign_map_widget_class", campaign.generated_class())
    controller_cdo.set_editor_property("mission_briefing_widget_class", briefing.generated_class())
    gi_cdo = unreal.get_default_object(game_instance.generated_class())
    gi_cdo.set_editor_property("mission_data_table", mission_table)
    stage_table = unreal.load_asset(STAGE_TABLE_PATH)
    if stage_table is not None:
        gi_cdo.set_editor_property("stage_data_table", stage_table)

    for asset in (start_menu, campaign, briefing, hangar, result, start_controller, game_instance):
        unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False)


def main():
    campaign_parent = unreal.load_class(None, "/Script/DualFire.DualFireCampaignMapWidget")
    briefing_parent = unreal.load_class(None, "/Script/DualFire.DualFireMissionBriefingWidget")
    if campaign_parent is None or briefing_parent is None:
        fail("Native mission flow widget classes are unavailable")
    button_class = load_required(MENU_BUTTON_PATH).generated_class()
    campaign = create_widget_blueprint(CAMPAIGN_PATH, campaign_parent)
    briefing = create_widget_blueprint(BRIEFING_PATH, briefing_parent)
    result = load_required(RESULT_PATH)
    build_campaign(campaign, button_class)
    build_briefing(briefing, button_class)
    extend_result_screen(result, button_class)
    compile_and_save(campaign)
    compile_and_save(briefing)
    compile_and_save(result)
    mission_table = create_mission_table()
    configure_defaults(campaign, briefing, mission_table)
    unreal.log("[DF_MISSION_FLOW] Campaign, briefing, result actions, and DT_Missions configured")


if __name__ == "__main__":
    main()
