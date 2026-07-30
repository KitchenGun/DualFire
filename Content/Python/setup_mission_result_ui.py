import unreal


SCREEN_PATH = "/Game/Blueprint/UI/Screen/WBP_MissionResult"
ROW_PATH = "/Game/Blueprint/UI/Components/WBP_MissionResultMetricRow"
UMG_TOOLSET = unreal.get_default_object(unreal.UMGToolSet)


def load_required(path):
    asset = unreal.load_asset(path)
    if asset is None:
        raise RuntimeError(f"Required asset not found: {path}")
    return asset


def create_widget_blueprint(asset_path, parent_class):
    existing = unreal.load_asset(asset_path)
    if existing is not None:
        return existing

    package_path, asset_name = asset_path.rsplit("/", 1)
    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_class)
    blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name,
        package_path,
        unreal.WidgetBlueprint,
        factory,
    )
    if blueprint is None:
        raise RuntimeError(f"Failed to create {asset_path}")
    return blueprint


def set_variable(blueprint, widget):
    UMG_TOOLSET.call_method("ToggleWidgetAsVariable", (blueprint, widget, True))


def set_font_size(text_widget, size):
    font = text_widget.get_editor_property("font")
    font.set_editor_property("size", size)
    text_widget.set_editor_property("font", font)


def set_text(text_widget, value, size):
    text_widget.set_text(value)
    set_font_size(text_widget, size)


def add_widget(blueprint, widget_class, name, parent=None, child_index=-1):
    info = UMG_TOOLSET.call_method(
        "AddWidget", (blueprint, widget_class, name, parent, child_index)
    )
    if info.widget is None:
        raise RuntimeError(f"Failed to add widget: {name}")
    return info.widget, info.slot


def add_canvas_widget(blueprint, canvas, widget_class, name, position, size, anchors=(0.0, 0.0, 0.0, 0.0), alignment=(0.0, 0.0)):
    widget, slot = add_widget(blueprint, widget_class, name, canvas)
    slot.set_position(unreal.Vector2D(position[0], position[1]))
    slot.set_size(unreal.Vector2D(size[0], size[1]))
    slot.set_alignment(unreal.Vector2D(alignment[0], alignment[1]))
    slot.set_anchors(unreal.Anchors(
        minimum=unreal.Vector2D(anchors[0], anchors[1]),
        maximum=unreal.Vector2D(anchors[2], anchors[3]),
    ))
    return widget


def add_full_screen(blueprint, canvas, widget_class, name):
    widget, slot = add_widget(blueprint, widget_class, name, canvas)
    slot.set_anchors(unreal.Anchors(
        minimum=unreal.Vector2D(0.0, 0.0),
        maximum=unreal.Vector2D(1.0, 1.0),
    ))
    slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
    return widget


def build_metric_row(blueprint):
    if UMG_TOOLSET.call_method("GetWidgets", (blueprint,)).info.widget_count > 0:
        return

    root, _ = add_widget(blueprint, unreal.SizeBox, "MetricRowRoot")
    root.set_width_override(1030.0)
    root.set_height_override(92.0)
    canvas, _ = add_widget(blueprint, unreal.CanvasPanel, "MetricRowCanvas", root)

    background = add_full_screen(blueprint, canvas, unreal.Image, "MetricRowBackground")
    background.set_brush_from_texture(load_required("/Game/UI/Textures/Result/T_UI_Result_EvaluationRow"), True)

    metric_name = add_canvas_widget(blueprint, canvas, unreal.CommonTextBlock, "Text_MetricName", (28.0, 23.0), (270.0, 46.0))
    set_variable(blueprint, metric_name)
    set_text(metric_name, "METRIC", 24)

    progress = add_canvas_widget(blueprint, canvas, unreal.ProgressBar, "Progress_Metric", (314.0, 36.0), (270.0, 18.0))
    set_variable(blueprint, progress)
    progress.set_percent(0.75)

    metric_value = add_canvas_widget(blueprint, canvas, unreal.CommonTextBlock, "Text_MetricValue", (610.0, 23.0), (230.0, 46.0))
    set_variable(blueprint, metric_value)
    set_text(metric_value, "0 / 0", 24)

    rank_image = add_canvas_widget(blueprint, canvas, unreal.Image, "Image_Rank", (918.0, 10.0), (72.0, 72.0))
    set_variable(blueprint, rank_image)

    rank_fallback = add_canvas_widget(blueprint, canvas, unreal.CommonTextBlock, "Text_RankFallback", (900.0, 23.0), (100.0, 46.0))
    set_variable(blueprint, rank_fallback)
    set_text(rank_fallback, "N/A", 22)


def build_result_screen(blueprint):
    if UMG_TOOLSET.call_method("GetWidgets", (blueprint,)).info.widget_count > 0:
        return

    root, _ = add_widget(blueprint, unreal.CanvasPanel, "ResultRoot")

    background = add_full_screen(blueprint, root, unreal.Image, "ResultBackground")
    background.set_brush_from_texture(load_required("/Game/UI/Textures/Result/T_UI_Result_Background"), True)

    eyebrow = add_canvas_widget(blueprint, root, unreal.CommonTextBlock, "Text_ReportEyebrow", (104.0, 58.0), (720.0, 34.0))
    set_text(eyebrow, "AIR OPERATIONS  //  AFTER ACTION REPORT", 20)

    result_title = add_canvas_widget(blueprint, root, unreal.CommonTextBlock, "Text_ResultTitle", (104.0, 98.0), (980.0, 76.0))
    set_variable(blueprint, result_title)
    set_text(result_title, "MISSION COMPLETE", 54)

    mission = add_canvas_widget(blueprint, root, unreal.CommonTextBlock, "Text_Mission", (108.0, 180.0), (980.0, 42.0))
    set_variable(blueprint, mission)
    set_text(mission, "MISSION 01  //  CHARGE ASSAULT", 24)

    metrics = add_canvas_widget(blueprint, root, unreal.VerticalBox, "MetricsBox", (104.0, 260.0), (1030.0, 470.0))
    set_variable(blueprint, metrics)

    rank_panel = add_canvas_widget(blueprint, root, unreal.Image, "RankPanel", (1380.0, 155.0), (410.0, 640.0))
    set_variable(blueprint, rank_panel)
    rank_panel.set_brush_from_texture(load_required("/Game/UI/Textures/Result/T_UI_Result_RankPanel"), True)

    rank_label = add_canvas_widget(blueprint, root, unreal.CommonTextBlock, "Text_RankLabel", (1450.0, 205.0), (280.0, 40.0))
    set_variable(blueprint, rank_label)
    set_text(rank_label, "OVERALL RANK", 21)

    overall_rank = add_canvas_widget(blueprint, root, unreal.Image, "Image_OverallRank", (1455.0, 260.0), (260.0, 260.0))
    set_variable(blueprint, overall_rank)

    overall_fallback = add_canvas_widget(blueprint, root, unreal.CommonTextBlock, "Text_OverallRankFallback", (1500.0, 330.0), (180.0, 100.0))
    set_variable(blueprint, overall_fallback)
    set_text(overall_fallback, "N/A", 72)

    clear_time = add_canvas_widget(blueprint, root, unreal.CommonTextBlock, "Text_ClearTime", (1445.0, 566.0), (300.0, 42.0))
    set_variable(blueprint, clear_time)
    set_text(clear_time, "TIME  00:00.000", 22)

    difficulty = add_canvas_widget(blueprint, root, unreal.CommonTextBlock, "Text_Difficulty", (1445.0, 618.0), (310.0, 42.0))
    set_variable(blueprint, difficulty)
    set_text(difficulty, "DIFFICULTY  NORMAL", 22)

    unlock_panel = add_canvas_widget(blueprint, root, unreal.Image, "UnlockPanel", (104.0, 820.0), (768.0, 112.0))
    set_variable(blueprint, unlock_panel)
    unlock_panel.set_brush_from_texture(load_required("/Game/UI/Textures/Result/T_UI_Result_UnlockPanel"), True)
    unlock_panel.set_visibility(unreal.SlateVisibility.COLLAPSED)

    unlock_text = add_canvas_widget(blueprint, root, unreal.CommonTextBlock, "Text_Unlock", (144.0, 852.0), (680.0, 44.0))
    set_variable(blueprint, unlock_text)
    set_text(unlock_text, "NEW CONTENT UNLOCKED", 22)
    unlock_text.set_visibility(unreal.SlateVisibility.COLLAPSED)


def ensure_result_screen_scaling(blueprint):
    tree = UMG_TOOLSET.call_method("GetWidgets", (blueprint,))
    root_info = next((info for info in tree.widgets if info.parent is None), None)
    if root_info is None or root_info.widget is None:
        raise RuntimeError("Result screen root widget was not found")

    if isinstance(root_info.widget, unreal.CanvasPanel):
        size_wrappers = UMG_TOOLSET.call_method(
            "WrapWidgets", (blueprint, [root_info.widget], unreal.SizeBox)
        )
        if len(size_wrappers) != 1 or size_wrappers[0].widget is None:
            raise RuntimeError("Failed to add the 1920x1080 reference SizeBox")
        reference_size = size_wrappers[0].widget
        reference_size.set_width_override(1920.0)
        reference_size.set_height_override(1080.0)

        scale_wrappers = UMG_TOOLSET.call_method(
            "WrapWidgets", (blueprint, [reference_size], unreal.ScaleBox)
        )
        if len(scale_wrappers) != 1 or scale_wrappers[0].widget is None:
            raise RuntimeError("Failed to add the result screen ScaleBox")
        content_scale = scale_wrappers[0].widget
    elif isinstance(root_info.widget, unreal.ScaleBox):
        content_scale = root_info.widget
    elif isinstance(root_info.widget, unreal.Overlay):
        content_scale = next(
            (
                info.widget
                for info in tree.widgets
                if info.parent == root_info.widget
                and isinstance(info.widget, unreal.ScaleBox)
            ),
            None,
        )
        if content_scale is None:
            raise RuntimeError("Result screen Overlay has no content ScaleBox")
    else:
        raise RuntimeError(
            f"Unexpected result screen root: {root_info.widget.get_class().get_name()}"
        )

    content_scale.set_editor_property("stretch", unreal.Stretch.SCALE_TO_FIT)

    tree = UMG_TOOLSET.call_method("GetWidgets", (blueprint,))
    root_info = next((info for info in tree.widgets if info.parent is None), None)
    if isinstance(root_info.widget, unreal.ScaleBox):
        overlay_wrappers = UMG_TOOLSET.call_method(
            "WrapWidgets", (blueprint, [root_info.widget], unreal.Overlay)
        )
        if len(overlay_wrappers) != 1 or overlay_wrappers[0].widget is None:
            raise RuntimeError("Failed to add the result screen root Overlay")
        root_overlay = overlay_wrappers[0].widget
        UMG_TOOLSET.call_method(
            "RenameWidget", (blueprint, root_overlay, "ResultRootOverlay")
        )
    elif isinstance(root_info.widget, unreal.Overlay):
        root_overlay = root_info.widget
    else:
        raise RuntimeError("Result screen root migration failed")

    tree = UMG_TOOLSET.call_method("GetWidgets", (blueprint,))
    backgrounds = [
        info
        for info in tree.widgets
        if info.widget is not None and info.widget.get_name() == "ResultBackground"
    ]
    root_background = next(
        (info.widget for info in backgrounds if info.parent == root_overlay), None
    )
    for info in backgrounds:
        if info.widget != root_background:
            UMG_TOOLSET.call_method("RemoveWidget", (blueprint, info.widget))

    if root_background is None:
        root_background, slot = add_widget(
            blueprint, unreal.Image, "ResultBackground", root_overlay, 0
        )
        slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
        slot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)

    root_background.set_brush_from_texture(
        load_required("/Game/UI/Textures/Result/T_UI_Result_Background"), True
    )
    root_background.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)


def compile_and_save(blueprint):
    if not UMG_TOOLSET.call_method("CompileWidgetBlueprint", (blueprint,)):
        raise RuntimeError(f"Failed to compile {blueprint.get_path_name()}")
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False)


row_parent = unreal.load_class(None, "/Script/DualFire.DualFireMissionResultMetricRowWidget")
screen_parent = unreal.load_class(None, "/Script/DualFire.DualFireMissionResultWidget")
if row_parent is None or screen_parent is None:
    raise RuntimeError("Mission result native widget classes are not available")

row_blueprint = create_widget_blueprint(ROW_PATH, row_parent)
build_metric_row(row_blueprint)
compile_and_save(row_blueprint)

screen_blueprint = create_widget_blueprint(SCREEN_PATH, screen_parent)
build_result_screen(screen_blueprint)
ensure_result_screen_scaling(screen_blueprint)
compile_and_save(screen_blueprint)

screen_cdo = unreal.get_default_object(screen_blueprint.generated_class())
screen_cdo.set_editor_property("confirm_input_action", load_required("/Game/Input/UI/IA_UI_Confirm"))
screen_cdo.set_editor_property("metric_row_class", row_blueprint.generated_class())
screen_cdo.set_editor_property("rank_texture_s", load_required("/Game/UI/Textures/Result/T_UI_Result_Rank_S"))
screen_cdo.set_editor_property("rank_texture_a", load_required("/Game/UI/Textures/Result/T_UI_Result_Rank_A"))
screen_cdo.set_editor_property("rank_texture_b", load_required("/Game/UI/Textures/Result/T_UI_Result_Rank_B"))
screen_cdo.set_editor_property("rank_texture_c", load_required("/Game/UI/Textures/Result/T_UI_Result_Rank_C"))
screen_cdo.set_editor_property("rank_texture_d", load_required("/Game/UI/Textures/Result/T_UI_Result_Rank_D"))
unreal.EditorAssetLibrary.save_loaded_asset(screen_blueprint, only_if_is_dirty=False)

row_description = UMG_TOOLSET.call_method("GetWidgetDescription", (row_blueprint, None, -1))
screen_description = UMG_TOOLSET.call_method("GetWidgetDescription", (screen_blueprint, None, -1))
unreal.log(f"[DualFire] Metric row tree\n{row_description.description}")
unreal.log(f"[DualFire] Result screen tree\n{screen_description.description}")
unreal.log("[DualFire] Mission result Widget Blueprints created")
