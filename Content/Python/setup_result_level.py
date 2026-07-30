import unreal


GAME_MODE_PATH = "/Game/Blueprint/GameMode/BP_GM_Result"
RESULT_LEVEL_PATH = "/Game/Level/LV_Result"


def create_result_game_mode():
    existing = unreal.load_asset(GAME_MODE_PATH)
    if existing is not None:
        return existing

    parent_class = unreal.load_class(
        None, "/Script/DualFire.DualFireResultGameMode"
    )
    if parent_class is None:
        raise RuntimeError("DualFireResultGameMode native class is unavailable")

    factory = unreal.BlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "BP_GM_Result",
        "/Game/Blueprint/GameMode",
        unreal.Blueprint,
        factory,
    )
    if blueprint is None:
        raise RuntimeError(f"Failed to create {GAME_MODE_PATH}")
    if not unreal.BlueprintEditorLibrary.compile_blueprint(blueprint):
        raise RuntimeError(f"Failed to compile {GAME_MODE_PATH}")
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)
    return blueprint


def create_result_level(game_mode_blueprint):
    if unreal.EditorAssetLibrary.does_asset_exist(RESULT_LEVEL_PATH):
        world = unreal.EditorLoadingAndSavingUtils.load_map(RESULT_LEVEL_PATH)
    else:
        world = unreal.EditorLoadingAndSavingUtils.new_blank_map(False)
    if world is None:
        raise RuntimeError(f"Failed to create or load {RESULT_LEVEL_PATH}")

    world_settings = world.get_world_settings()
    world_settings.set_editor_property(
        "default_game_mode", game_mode_blueprint.generated_class()
    )
    if not unreal.EditorLoadingAndSavingUtils.save_map(world, RESULT_LEVEL_PATH):
        raise RuntimeError(f"Failed to save {RESULT_LEVEL_PATH}")


result_game_mode = create_result_game_mode()
create_result_level(result_game_mode)
unreal.log("[DualFire] Result level and GameMode created")
