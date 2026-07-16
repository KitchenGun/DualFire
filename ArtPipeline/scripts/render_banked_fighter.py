"""Blender background renderer for the seven deterministic fighter bank poses."""

import argparse
import sys
from pathlib import Path

import bmesh
import bpy
from mathutils import Quaternion, Vector


POSES = (
    ("BankLeft30", -30), ("BankLeft20", -20), ("BankLeft10", -10), ("Neutral", 0),
    ("BankRight10", 10), ("BankRight20", 20), ("BankRight30", 30),
)


def parse_args():
    arguments = sys.argv[sys.argv.index("--") + 1 :]
    parser = argparse.ArgumentParser()
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--width", type=int, required=True)
    parser.add_argument("--height", type=int, required=True)
    parser.add_argument("--forward-axis", choices=("X", "Y", "Z"), default="X")
    parser.add_argument(
        "--source-top-axis",
        choices=("X", "Y", "Z", "-X", "-Y", "-Z"),
        default="Z",
    )
    parser.add_argument("--ortho-scale-factor", type=float, default=1.65)
    parser.add_argument("--light-energy-scale", type=float, default=1.0)
    parser.add_argument("--cabin-material-slot", type=int, default=0)
    parser.add_argument("--hull-material-slot", type=int, default=1)
    parser.add_argument("--recalculate-normals", action="store_true")
    parser.add_argument("--override-all-materials", action="store_true")
    parser.add_argument("--preserve-source-materials", action="store_true")
    parser.add_argument("--mesh-size-threshold", type=float, default=0.20)
    parser.add_argument("--exclude-mesh-prefixes", nargs="*", default=())
    parser.add_argument("--canonical-style", type=Path)
    parser.add_argument("--canonical-blend", type=float, default=0.0)
    parser.add_argument("--canonical-u-axis", choices=("X", "Y", "Z", "-X", "-Y", "-Z"), default="Y")
    parser.add_argument("--canonical-v-axis", choices=("X", "Y", "Z", "-X", "-Y", "-Z"), default="-X")
    parser.add_argument("--canonical-normal-fade-min", type=float, default=0.10)
    parser.add_argument("--canonical-normal-fade-max", type=float, default=0.45)
    parser.add_argument("--roll-degrees", type=float, nargs=7)
    parser.add_argument("--pose-names", nargs=7)
    return parser.parse_args(arguments)


def clear_scene():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)


def import_model(source: Path):
    suffix = source.suffix.lower()
    if suffix == ".blend":
        with bpy.data.libraries.load(str(source), link=False) as data:
            objects = list(data.objects)
        for obj in objects:
            if obj is not None:
                bpy.context.collection.objects.link(obj)
        return
    if suffix in {".glb", ".gltf"}:
        bpy.ops.import_scene.gltf(filepath=str(source))
    elif suffix == ".fbx":
        bpy.ops.import_scene.fbx(filepath=str(source))
    elif suffix == ".obj":
        bpy.ops.wm.obj_import(filepath=str(source))
    else:
        raise RuntimeError(f"Unsupported input: {source}")


def create_camera(scene, radius, ortho_scale_factor):
    camera_data = bpy.data.cameras.new("DualFireOrthoCamera")
    camera_data.type = "ORTHO"
    camera_data.ortho_scale = radius * ortho_scale_factor
    camera = bpy.data.objects.new("DualFireOrthoCamera", camera_data)
    scene.collection.objects.link(camera)
    camera.location = (0.0, 0.0, radius * 4.0)
    # Blender cameras look along local -Z, so the identity rotation is top-down.
    camera.rotation_mode = "QUATERNION"
    camera.rotation_quaternion = (1.0, 0.0, 0.0, 0.0)
    scene.camera = camera


def _aim_at(light, target=Vector((0.0, 0.0, 0.0))):
    direction = target - light.location
    light.rotation_mode = "QUATERNION"
    light.rotation_quaternion = direction.to_track_quat("-Z", "Y")


def create_three_point_lighting(scene, radius, energy_scale):
    """Use fixed world-space lights so roll changes self-shadowing coherently."""
    setup = (
        ("DualFireKey", 320, (radius * -1.10, radius * -0.90, radius * 3.0), radius * 1.10, (1.0, 0.93, 0.86), True),
        ("DualFireFill", 140, (radius * 1.20, radius * -0.10, radius * 2.3), radius * 1.45, (0.76, 0.86, 1.0), False),
        ("DualFireRim", 180, (radius * 0.10, radius * 1.30, radius * 2.7), radius * 0.90, (0.88, 0.94, 1.0), False),
    )
    for name, energy, location, size, color, casts_shadow in setup:
        light_data = bpy.data.lights.new(name, type="AREA")
        light_data.energy = energy * energy_scale
        light_data.shape = "DISK"
        light_data.size = size
        light_data.color = color
        light_data.use_shadow = casts_shadow
        light = bpy.data.objects.new(name, light_data)
        scene.collection.objects.link(light)
        light.location = location
        _aim_at(light)


def _image(path: Path, colorspace: str):
    image = bpy.data.images.load(str(path), check_existing=True)
    image.colorspace_settings.name = colorspace
    return image


def _axis_output(nodes, links, separated, axis):
    component = separated.outputs[axis[-1]]
    if not axis.startswith("-"):
        return component
    invert = nodes.new("ShaderNodeMath")
    invert.operation = "SUBTRACT"
    invert.inputs[0].default_value = 1.0
    links.new(component, invert.inputs[1])
    return invert.outputs[0]


def _project_canonical_surface(
    nodes, links, albedo, canonical_path, blend, u_axis, v_axis, normal_fade_min, normal_fade_max
):
    coordinates = nodes.new("ShaderNodeTexCoord")
    separated = nodes.new("ShaderNodeSeparateXYZ")
    combined = nodes.new("ShaderNodeCombineXYZ")
    links.new(coordinates.outputs["Generated"], separated.inputs["Vector"])
    links.new(_axis_output(nodes, links, separated, u_axis), combined.inputs["X"])
    links.new(_axis_output(nodes, links, separated, v_axis), combined.inputs["Y"])

    canonical = nodes.new("ShaderNodeTexImage")
    canonical.image = _image(canonical_path, "sRGB")
    canonical.interpolation = "Closest"
    canonical.extension = "CLIP"
    links.new(combined.outputs["Vector"], canonical.inputs["Vector"])

    strength = nodes.new("ShaderNodeMath")
    strength.operation = "MULTIPLY"
    strength.inputs[1].default_value = blend
    links.new(canonical.outputs["Alpha"], strength.inputs[0])

    local_normal = nodes.new("ShaderNodeSeparateXYZ")
    links.new(coordinates.outputs["Normal"], local_normal.inputs["Vector"])
    normal_offset = nodes.new("ShaderNodeMath")
    normal_offset.operation = "SUBTRACT"
    normal_offset.inputs[1].default_value = normal_fade_min
    links.new(local_normal.outputs["Z"], normal_offset.inputs[0])
    normal_scale = nodes.new("ShaderNodeMath")
    normal_scale.operation = "MULTIPLY"
    normal_scale.inputs[1].default_value = 1.0 / (normal_fade_max - normal_fade_min)
    links.new(normal_offset.outputs[0], normal_scale.inputs[0])
    normal_clamp = nodes.new("ShaderNodeClamp")
    normal_clamp.inputs["Min"].default_value = 0.0
    normal_clamp.inputs["Max"].default_value = 1.0
    links.new(normal_scale.outputs[0], normal_clamp.inputs["Value"])
    masked_strength = nodes.new("ShaderNodeMath")
    masked_strength.operation = "MULTIPLY"
    links.new(strength.outputs[0], masked_strength.inputs[0])
    links.new(normal_clamp.outputs["Result"], masked_strength.inputs[1])

    mixed = nodes.new("ShaderNodeMixRGB")
    mixed.blend_type = "MIX"
    links.new(masked_strength.outputs[0], mixed.inputs["Fac"])
    links.new(albedo.outputs["Color"], mixed.inputs[1])
    links.new(canonical.outputs["Color"], mixed.inputs[2])
    return mixed.outputs["Color"]


def _material_with_maps(
    name,
    albedo_path,
    normal_path=None,
    metallic_path=None,
    emission_path=None,
    canonical_path=None,
    canonical_blend=0.0,
    canonical_u_axis="Y",
    canonical_v_axis="-X",
    canonical_normal_fade_min=0.10,
    canonical_normal_fade_max=0.45,
):
    material = bpy.data.materials.new(name)
    material.use_nodes = True
    nodes = material.node_tree.nodes
    links = material.node_tree.links
    nodes.clear()
    principled = nodes.new("ShaderNodeBsdfPrincipled")
    output = nodes.new("ShaderNodeOutputMaterial")
    links.new(principled.outputs["BSDF"], output.inputs["Surface"])

    albedo = nodes.new("ShaderNodeTexImage")
    albedo.image = _image(albedo_path, "sRGB")
    albedo.interpolation = "Linear"
    base_color = albedo.outputs["Color"]
    if canonical_path is not None:
        base_color = _project_canonical_surface(
            nodes,
            links,
            albedo,
            canonical_path,
            canonical_blend,
            canonical_u_axis,
            canonical_v_axis,
            canonical_normal_fade_min,
            canonical_normal_fade_max,
        )
    links.new(base_color, principled.inputs["Base Color"])

    if normal_path and normal_path.is_file():
        normal = nodes.new("ShaderNodeTexImage")
        normal.image = _image(normal_path, "Non-Color")
        normal.interpolation = "Linear"
        normal_map = nodes.new("ShaderNodeNormalMap")
        normal_map.inputs["Strength"].default_value = 0.28
        links.new(normal.outputs["Color"], normal_map.inputs["Color"])
        links.new(normal_map.outputs["Normal"], principled.inputs["Normal"])

    if metallic_path and metallic_path.is_file():
        packed = nodes.new("ShaderNodeTexImage")
        packed.image = _image(metallic_path, "Non-Color")
        packed.interpolation = "Linear"
        links.new(packed.outputs["Color"], principled.inputs["Metallic"])
        invert_smoothness = nodes.new("ShaderNodeMath")
        invert_smoothness.operation = "SUBTRACT"
        invert_smoothness.inputs[0].default_value = 1.0
        links.new(packed.outputs["Alpha"], invert_smoothness.inputs[1])
        links.new(invert_smoothness.outputs[0], principled.inputs["Roughness"])
    else:
        principled.inputs["Metallic"].default_value = 0.35
        principled.inputs["Roughness"].default_value = 0.62

    if emission_path and emission_path.is_file():
        emission = nodes.new("ShaderNodeTexImage")
        emission.image = _image(emission_path, "sRGB")
        emission.interpolation = "Linear"
        emission_input = principled.inputs.get("Emission Color")
        if emission_input is None:
            emission_input = principled.inputs.get("Emission")
        strength_input = principled.inputs.get("Emission Strength")
        if emission_input:
            links.new(emission.outputs["Color"], emission_input)
        if strength_input:
            strength_input.default_value = 0.25
    return material


def apply_airframe_materials(
    models,
    source: Path,
    cabin_material_slot: int,
    hull_material_slot: int,
    canonical_path: Path | None,
    canonical_blend: float,
    canonical_u_axis: str,
    canonical_v_axis: str,
    canonical_normal_fade_min: float,
    canonical_normal_fade_max: float,
    override_all_materials: bool,
):
    texture_dir = source.parent.parent / "textures"
    if not texture_dir.is_dir():
        return
    albedo_candidates = sorted(texture_dir.glob("*AlbedoTransparency*.png"))
    hull_albedo = next((path for path in albedo_candidates if "Cabina" not in path.name), None)
    cabin_albedo = next(iter(sorted(texture_dir.glob("*Cabina_AlbedoTransparency*.png"))), None)
    if hull_albedo is None:
        return
    normal = next(iter(sorted(texture_dir.glob("*Normal*.png"))), None)
    metallic = next(iter(sorted(texture_dir.glob("*MetallicSmoothness*.png"))), None)
    emission = next(iter(sorted(texture_dir.glob("*Emission*.png"))), None)
    hull_material = _material_with_maps(
        "DualFireHull", hull_albedo, normal, metallic, emission,
        canonical_path=canonical_path, canonical_blend=canonical_blend,
        canonical_u_axis=canonical_u_axis, canonical_v_axis=canonical_v_axis,
        canonical_normal_fade_min=canonical_normal_fade_min,
        canonical_normal_fade_max=canonical_normal_fade_max,
    )
    cabin_material = _material_with_maps(
        "DualFireCabin", cabin_albedo, None, None, None,
        canonical_path=canonical_path, canonical_blend=canonical_blend,
        canonical_u_axis=canonical_u_axis, canonical_v_axis=canonical_v_axis,
        canonical_normal_fade_min=canonical_normal_fade_min,
        canonical_normal_fade_max=canonical_normal_fade_max,
    ) if cabin_albedo else hull_material

    for obj in models:
        if len(obj.data.materials) > 1 and (
            cabin_material_slot >= len(obj.data.materials) or hull_material_slot >= len(obj.data.materials)
        ):
            raise RuntimeError(
                f"Configured material slots exceed {obj.name} slot count: "
                f"cabin={cabin_material_slot}, hull={hull_material_slot}, count={len(obj.data.materials)}"
            )
        for slot_index in range(len(obj.data.materials)):
            if override_all_materials or len(obj.data.materials) == 1 or slot_index == hull_material_slot:
                obj.data.materials[slot_index] = hull_material
            elif slot_index == cabin_material_slot:
                obj.data.materials[slot_index] = cabin_material


def select_airframe_meshes(scene, size_threshold, exclude_prefixes):
    all_meshes = [obj for obj in scene.objects if obj.type == "MESH" and len(obj.data.polygons)]
    if not all_meshes:
        return []
    excluded = tuple(prefix.casefold() for prefix in exclude_prefixes)
    meshes = [obj for obj in all_meshes if not obj.name.casefold().startswith(excluded)]
    if not meshes:
        return []
    diagonals = {
        obj: max((Vector(corner) - Vector(obj.bound_box[0])).length for corner in obj.bound_box)
        for obj in meshes
    }
    largest_diagonal = max(diagonals.values())
    airframe = [obj for obj in meshes if diagonals[obj] >= largest_diagonal * size_threshold]
    for obj in all_meshes:
        if obj not in airframe:
            bpy.data.objects.remove(obj, do_unlink=True)
    return airframe


def recalculate_normals(models):
    for obj in models:
        mesh = bmesh.new()
        mesh.from_mesh(obj.data)
        bmesh.ops.recalc_face_normals(mesh, faces=mesh.faces)
        mesh.to_mesh(obj.data)
        mesh.free()
        obj.data.update()


def main():
    args = parse_args()
    if (args.roll_degrees is None) != (args.pose_names is None):
        raise RuntimeError("roll degrees and pose names must be provided together")
    poses = tuple(zip(args.pose_names, args.roll_degrees)) if args.roll_degrees else POSES
    if args.canonical_style is not None and not args.canonical_style.is_file():
        raise RuntimeError(f"Missing canonical style texture: {args.canonical_style}")
    if not 0.0 <= args.canonical_blend <= 1.0:
        raise RuntimeError("canonical blend must be between 0 and 1")
    if not -1.0 <= args.canonical_normal_fade_min < args.canonical_normal_fade_max <= 1.0:
        raise RuntimeError("canonical normal fade must be increasing within -1..1")
    clear_scene()
    import_model(args.source)
    scene = bpy.context.scene
    try:
        scene.render.engine = "BLENDER_EEVEE_NEXT"
    except TypeError:
        scene.render.engine = "BLENDER_EEVEE"
    scene.render.film_transparent = True
    scene.render.resolution_x = args.width
    scene.render.resolution_y = args.height
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.render.image_settings.color_mode = "RGBA"
    scene.render.image_settings.color_depth = "8"
    scene.render.image_settings.compression = 15
    scene.view_settings.look = "AgX - Medium High Contrast"
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (0.025, 0.03, 0.04, 1.0)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = 0.03

    models = select_airframe_meshes(scene, args.mesh_size_threshold, args.exclude_mesh_prefixes)
    if not models:
        raise RuntimeError("No mesh object was imported")
    if args.canonical_style is not None and len(models) != 1:
        raise RuntimeError("Object-space canonical projection currently requires one airframe mesh")
    if args.recalculate_normals:
        recalculate_normals(models)
    bpy.ops.object.select_all(action="DESELECT")
    for obj in models:
        obj.select_set(True)
    bpy.context.view_layer.objects.active = models[0]
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    if not args.preserve_source_materials:
        apply_airframe_materials(
            models,
            args.source,
            args.cabin_material_slot,
            args.hull_material_slot,
            args.canonical_style,
            args.canonical_blend,
            args.canonical_u_axis,
            args.canonical_v_axis,
            args.canonical_normal_fade_min,
            args.canonical_normal_fade_max,
            args.override_all_materials,
        )
    bounds = [obj.matrix_world @ Vector(corner) for obj in models for corner in obj.bound_box]
    center = sum(bounds, Vector()) / len(bounds)
    radius = max((point - center).length for point in bounds)
    root = bpy.data.objects.new("DualFireBankRoot", None)
    scene.collection.objects.link(root)
    for obj in models:
        world_transform = obj.matrix_world.copy()
        obj.parent = root
        obj.matrix_world = world_transform
    root.location = -center

    create_camera(scene, radius, args.ortho_scale_factor)
    create_three_point_lighting(scene, radius, args.light_energy_scale)
    args.output.mkdir(parents=True, exist_ok=True)
    axes = {
        "X": Vector((1.0, 0.0, 0.0)), "Y": Vector((0.0, 1.0, 0.0)), "Z": Vector((0.0, 0.0, 1.0)),
        "-X": Vector((-1.0, 0.0, 0.0)), "-Y": Vector((0.0, -1.0, 0.0)), "-Z": Vector((0.0, 0.0, -1.0)),
    }
    axis = axes[args.forward_axis]
    # Normalize this FBX to a nose-up top-down game orientation once, then roll
    # the original 3D geometry around its declared local forward axis.
    top_rotation = axes[args.source_top_axis].rotation_difference(Vector((0.0, 0.0, 1.0)))
    base_rotation = Quaternion((0.0, 0.0, 1.0), -1.5707963267948966) @ top_rotation
    root.rotation_mode = "QUATERNION"
    for label, degrees in poses:
        # Roll imported geometry around its local forward axis, never a 2D image rotation.
        root.rotation_quaternion = base_rotation @ Quaternion(axis, degrees * 0.017453292519943295)
        # Center after the roll. Applying a fixed pre-rotation offset here would
        # make every pose drift away from the canonical sprite's pivot.
        root.location = -(root.rotation_quaternion @ center)
        scene.render.filepath = str(args.output / f"{label}.png")
        bpy.ops.render.render(write_still=True)


if __name__ == "__main__":
    main()
