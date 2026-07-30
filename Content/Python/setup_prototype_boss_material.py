import unreal


ASSET_PATH = "/Game/Materials/M_PrototypeBossCube"


def create_or_update_material():
    material = unreal.load_asset(ASSET_PATH)
    if material is None:
        asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
        material = asset_tools.create_asset(
            "M_PrototypeBossCube",
            "/Game/Materials",
            unreal.Material,
            unreal.MaterialFactoryNew(),
        )

    if material is None:
        raise RuntimeError(f"Failed to create {ASSET_PATH}")

    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_OPAQUE)
    material.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)

    unreal.MaterialEditingLibrary.delete_all_material_expressions(material)
    emissive = unreal.MaterialEditingLibrary.create_material_expression(
        material,
        unreal.MaterialExpressionConstant3Vector,
        -280,
        0,
    )
    emissive.set_editor_property("constant", unreal.LinearColor(0.0, 5.5, 8.0, 1.0))
    unreal.MaterialEditingLibrary.connect_material_property(
        emissive,
        "",
        unreal.MaterialProperty.MP_EMISSIVE_COLOR,
    )

    unreal.MaterialEditingLibrary.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material, only_if_is_dirty=False)
    unreal.log(f"[DualFire] Created {ASSET_PATH}")


create_or_update_material()
