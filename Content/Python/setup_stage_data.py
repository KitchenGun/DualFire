"""STAGE_TEST Scroll Curve, pause trigger DataTable, GameInstance reference를 구성한다."""

import json

import unreal


CURVE_PATH = "/Game/Data/Stage/CRV_StageTestScroll"
STAGE_TABLE_PATH = "/Game/Data/Stage/DT_Stages"
GAME_INSTANCE_PATH = "/Game/Blueprint/GameInstance/BP_GameInstance"
STAGE_CONTROLLER_PATH = "/Game/Blueprint/Stage/BP_StageController"


def fail(message):
    raise RuntimeError(f"[DF_STAGE_DATA] {message}")


def create_curve():
    curve = unreal.load_asset(CURVE_PATH)
    if curve is None:
        factory = unreal.CurveFactory()
        factory.set_editor_property("curve_class", unreal.CurveFloat)
        curve = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "CRV_StageTestScroll", "/Game/Data/Stage", unreal.CurveFloat, factory
        )
    if not isinstance(curve, unreal.CurveFloat):
        fail("CRV_StageTestScroll creation failed")

    keys = [
        unreal.Vector2D(0.0, 1.0),
        unreal.Vector2D(25.0, 0.0),
        unreal.Vector2D(40.0, 0.0),
        unreal.Vector2D(55.0, 1.0),
    ]
    if not unreal.DualFireStageDataLibrary.set_normalized_curve_keys(curve, keys):
        fail("CRV_StageTestScroll key setup failed")
    unreal.EditorAssetLibrary.save_loaded_asset(curve, only_if_is_dirty=False)
    return curve


def create_stage_table(curve):
    table = unreal.load_asset(STAGE_TABLE_PATH)
    if table is None:
        factory = unreal.DataTableFactory()
        factory.set_editor_property("struct", unreal.StageRow.static_struct())
        table = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DT_Stages", "/Game/Data/Stage", unreal.DataTable, factory
        )
    if not isinstance(table, unreal.DataTable):
        fail("DT_Stages creation failed")

    rows = [
        {
            "Name": "STAGE_TEST",
            "StageID": "STAGE_TEST",
            "MinScrollSpeed": 0.0,
            "MaxScrollSpeed": 200.0,
            "NormalizedScrollCurve": curve.get_path_name(),
            "PauseTriggers": [
                {
                    "TriggerTime": 18.0,
                    "ResumeCondition": "RealTime",
                    "ResumeDelay": 1.0,
                    "TargetEnemyID": "None",
                },
                {
                    "TriggerTime": 22.0,
                    "ResumeCondition": "WaveDefeated",
                    "ResumeDelay": 0.0,
                    "TargetEnemyID": "None",
                },
                {
                    "TriggerTime": 42.0,
                    "ResumeCondition": "EnemyDefeated",
                    "ResumeDelay": 0.0,
                    "TargetEnemyID": "ENEMY_AIR",
                },
            ],
        }
    ]
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(
        table, json.dumps(rows)
    ):
        fail("DT_Stages JSON import failed")
    unreal.EditorAssetLibrary.save_loaded_asset(table, only_if_is_dirty=False)
    return table


def connect_assets(stage_table):
    game_instance = unreal.load_asset(GAME_INSTANCE_PATH)
    stage_controller = unreal.load_asset(STAGE_CONTROLLER_PATH)
    if game_instance is None or stage_controller is None:
        fail("BP_GameInstance or BP_StageController is missing")
    unreal.get_default_object(game_instance.generated_class()).set_editor_property(
        "stage_data_table", stage_table
    )
    unreal.EditorAssetLibrary.save_loaded_asset(game_instance, only_if_is_dirty=False)
    unreal.BlueprintEditorLibrary.compile_blueprint(stage_controller)
    unreal.EditorAssetLibrary.save_loaded_asset(stage_controller, only_if_is_dirty=False)


def verify_assets(curve, stage_table):
    expected_values = {0.0: 1.0, 25.0: 0.0, 30.0: 0.0, 40.0: 0.0, 55.0: 1.0}
    for time, expected in expected_values.items():
        actual = curve.get_float_value(time)
        if abs(actual - expected) > 0.001:
            fail(f"Curve verification failed at {time}: {actual} != {expected}")
    row_names = unreal.DataTableFunctionLibrary.get_data_table_row_names(stage_table)
    if [str(name) for name in row_names] != ["STAGE_TEST"]:
        fail(f"Unexpected DT_Stages rows: {row_names}")


def main():
    curve = create_curve()
    stage_table = create_stage_table(curve)
    connect_assets(stage_table)
    verify_assets(curve, stage_table)
    unreal.log("[DF_STAGE_DATA] STAGE_TEST curve, pause triggers, and GameInstance reference configured")


if __name__ == "__main__":
    main()
