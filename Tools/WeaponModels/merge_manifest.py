"""Merge independently-authored weapon family manifests."""
from pathlib import Path
import json


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "TunaSweeper/SourceArt/Weapons/TunaWeaponCollection"
families = []
for name in ("SMG", "AR", "Pistol"):
    path = SOURCE / "Manifests" / f"{name}.json"
    families.append(json.loads(path.read_text(encoding="utf-8")))

first = families[0]
manifest = {
    "collection": "TunaWeaponCollection",
    "engine": "Unreal Engine 5.7",
    "additive_only": True,
    "axis_contract": first["axis_contract"],
    "unit_contract": first["unit_contract"],
    "pivot_contract": first["pivot_contract"],
    "texture": first["texture"],
    "ue_material_authoring": first["ue_material_authoring"],
    "materials": first["materials"],
    "assets": [],
}
for family in families:
    for field in ("axis_contract", "unit_contract", "pivot_contract", "texture", "ue_material_authoring", "materials"):
        assert family[field] == first[field], (family["family"], field)
    manifest["assets"].extend(family["assets"])

names = [entry["name"] for entry in manifest["assets"]]
assert len(names) == 6 and len(set(names)) == 6, names
assert {entry["category"] for entry in manifest["assets"]} == {"SMG", "AR", "Pistol"}
assert {entry["style"] for entry in manifest["assets"]} == {"Standard", "Premium"}
manifest["total_triangles"] = sum(entry["triangles"] for entry in manifest["assets"])
(SOURCE / "model_manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
print(f"WEAPON_MANIFEST_MERGED {len(names)} assets {manifest['total_triangles']} triangles")
