import unreal


MATERIAL_PATH = "/Game/Characters/Robot/Materials"
MASTER_MATERIAL_PATH = f"{MATERIAL_PATH}/M_Robot_Customizable"
BASE_ENEMY_BLUEPRINT_PATH = "/Game/Blueprints/BP_QuadrupedGunEnemy"
VARIANT_BLUEPRINT_PATH = "/Game/Blueprints/Enemies/QuadrupedVariants"

EXPECTED_VARIANTS = {
    "BrightGray": {
        "color": unreal.LinearColor(0.82, 0.85, 0.90, 1.0),
        "metallic": 0.0,
        "roughness": 0.42,
        "specular": 0.50,
    },
    "Red": {
        "color": unreal.LinearColor(0.82, 0.035, 0.025, 1.0),
        "metallic": 0.0,
        "roughness": 0.34,
        "specular": 0.50,
    },
    "Blue": {
        "color": unreal.LinearColor(0.025, 0.18, 0.85, 1.0),
        "metallic": 0.05,
        "roughness": 0.28,
        "specular": 0.50,
    },
    "Gold": {
        "color": unreal.LinearColor(1.0, 0.48, 0.035, 1.0),
        "metallic": 1.0,
        "roughness": 0.22,
        "specular": 0.50,
    },
}


def require_asset(asset_path):
    asset = unreal.load_asset(asset_path)
    if asset is None:
        raise RuntimeError(f"Missing asset: {asset_path}")
    return asset


def nearly_equal(left, right, tolerance=0.0001):
    return abs(left - right) <= tolerance


def validate_color(actual, expected, label):
    for channel in ("r", "g", "b", "a"):
        actual_value = getattr(actual, channel)
        expected_value = getattr(expected, channel)
        if not nearly_equal(actual_value, expected_value):
            raise RuntimeError(
                f"{label} {channel} mismatch: actual={actual_value} expected={expected_value}"
            )


def main():
    master_material = require_asset(MASTER_MATERIAL_PATH)
    base_blueprint = require_asset(BASE_ENEMY_BLUEPRINT_PATH)
    base_generated_class = base_blueprint.generated_class()
    if base_generated_class is None:
        raise RuntimeError("Base quadruped enemy Blueprint has no generated class")

    required_master_scalars = (
        "TintStrength",
        "BaseMetallic",
        "BaseRoughness",
        "BaseSpecular",
        "BodyMetallic",
        "BodyRoughness",
        "BodySpecular",
    )
    for parameter_name in required_master_scalars:
        value = unreal.MaterialEditingLibrary.get_material_default_scalar_parameter_value(
            master_material,
            parameter_name,
        )
        if value is None:
            raise RuntimeError(f"Master material is missing scalar parameter: {parameter_name}")

    for suffix, expected in EXPECTED_VARIANTS.items():
        material_path = f"{MATERIAL_PATH}/MI_Robot_{suffix}"
        blueprint_path = (
            f"{VARIANT_BLUEPRINT_PATH}/BP_QuadrupedGunEnemy_{suffix}"
        )
        material_instance = require_asset(material_path)
        blueprint = require_asset(blueprint_path)

        parent = material_instance.get_editor_property("parent")
        if parent is None or parent.get_path_name() != master_material.get_path_name():
            raise RuntimeError(f"Wrong MI parent: {material_path}")

        actual_color = (
            unreal.MaterialEditingLibrary.get_material_instance_vector_parameter_value(
                material_instance,
                "BodyColor",
            )
        )
        validate_color(actual_color, expected["color"], f"{material_path} BodyColor")

        for parameter_name, expected_value in (
            ("BodyMetallic", expected["metallic"]),
            ("BodyRoughness", expected["roughness"]),
            ("BodySpecular", expected["specular"]),
        ):
            actual_value = (
                unreal.MaterialEditingLibrary.get_material_instance_scalar_parameter_value(
                    material_instance,
                    parameter_name,
                )
            )
            if not nearly_equal(actual_value, expected_value):
                raise RuntimeError(
                    f"{material_path} {parameter_name} mismatch: "
                    f"actual={actual_value} expected={expected_value}"
                )

        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
        generated_class = blueprint.generated_class()
        if generated_class is None:
            raise RuntimeError(f"Blueprint compile failed: {blueprint_path}")
        defaults = unreal.get_default_object(generated_class)
        mesh = defaults.get_component_by_class(unreal.SkeletalMeshComponent)
        if mesh is None:
            raise RuntimeError(f"Blueprint has no skeletal mesh: {blueprint_path}")
        assigned_material = mesh.get_material(0)
        if (
            assigned_material is None
            or assigned_material.get_path_name() != material_instance.get_path_name()
        ):
            raise RuntimeError(
                f"Blueprint material mismatch: {blueprint_path}; "
                f"actual={assigned_material} expected={material_instance.get_path_name()}"
            )

        unreal.log(
            f"ROBOT_VARIANT_VALID={suffix};MI={material_instance.get_path_name()};"
            f"BP={blueprint.get_path_name()};Material={assigned_material.get_path_name()}"
        )

    unreal.log("ROBOT_VARIANT_VALIDATION_SUCCESS")


main()
