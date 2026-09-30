"""Read-only UE Editor commandlet audit for raid map migration.

Run with UnrealEditor-Cmd.exe <uproject> -ExecutePythonScript=<this file>.
RAID_AUDIT_OUTPUT may override the output directory. The native Editor hook
TunaSweeper.RaidLevelAudit exports all non-transient reflected FProperties,
including non-Python-visible Level Blueprint graph/node and component fields.
"""
import hashlib
import json
import os
import pathlib
import re
import unreal

DEFAULT_MAPS = (
    "/Game/Maps/DemoBoxRaidMap",
    "/Game/Maps/DemoRaidMap",
    "/Game/MainRaid/RaidMap",
)
DEFAULT_ROOT_ASSETS = (
    "/RaidLevelKit/Placement/BP_RaidPlacementAnchor",
    "/RaidLevelKit/Placement/DA_LootAnchorPreviews",
)
MAPS = tuple(json.loads(os.environ["RAID_AUDIT_MAPS"])) if "RAID_AUDIT_MAPS" in os.environ else DEFAULT_MAPS
ROOT_ASSETS = tuple(json.loads(os.environ["RAID_AUDIT_ROOT_ASSETS"])) if "RAID_AUDIT_ROOT_ASSETS" in os.environ else DEFAULT_ROOT_ASSETS
LOGICAL = {package: ("DemoRaidMap" if package.rsplit("/", 1)[-1] in ("DemoBoxRaidMap", "DemoRaidMap") else "RaidMap") for package in MAPS}
LOGICAL.update(json.loads(os.environ.get("RAID_AUDIT_LOGICAL_IDS", "{}")))
OUTPUT_FILE = os.environ.get("RAID_AUDIT_OUTPUT_FILE", "before.json")
GAME_DATA = ("EnemySpawns.json", "LootContainerSpawns.json", "MemoSpawns.json")
KIND_BY_DATA = {"EnemySpawns.json": "Enemy", "LootContainerSpawns.json": "LootContainer", "MemoSpawns.json": "Memo"}
PROJECT = pathlib.Path(unreal.Paths.project_dir())
OUTPUT = pathlib.Path(os.environ.get("RAID_AUDIT_OUTPUT", str(PROJECT / "Saved" / "RaidLevelMigration")))
OUTPUT.mkdir(parents=True, exist_ok=True)


def save(name, value):
    (OUTPUT / name).write_text(json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + "\n", encoding="utf-8")


def options(hard, soft):
    result = unreal.AssetRegistryDependencyOptions()
    result.set_editor_properties({
        "include_hard_package_references": hard,
        "include_soft_package_references": soft,
        "include_hard_management_references": False,
        "include_soft_management_references": False,
        "include_searchable_names": False,
        "include_editor_only_package_references": True,
        "include_game_package_references": True,
    })
    return result


def query(registry, method, package, opts):
    return sorted({str(v) for v in getattr(registry, method)(package, opts) or []})


def dependency_graph(registry, roots):
    hard = options(True, False)
    soft = options(False, True)
    graph = {}
    todo = list(roots)
    while todo:
        package = todo.pop()
        if package in graph:
            continue
        if package.startswith("/Script/") or package.startswith("/Engine/") or package.startswith("/Engine"):
            continue
        if "ProductionPayload" in package:
            raise RuntimeError("Restricted package reference encountered; stop audit without reading it")
        hard_deps = query(registry, "get_dependencies", package, hard)
        soft_deps = query(registry, "get_dependencies", package, soft)
        graph[package] = {"hard": hard_deps, "soft": soft_deps}
        for dep in hard_deps + soft_deps:
            if dep.startswith("/") and not dep.startswith(("/Engine/", "/Script/")) and dep not in graph:
                todo.append(dep)
    return dict(sorted(graph.items()))


def load_external_actors(map_package, initial):
    if not initial["uses_external_actors"]:
        return {"present": False, "descriptors": 0, "loaded": 0}
    descriptions = unreal.WorldPartitionBlueprintLibrary.get_actor_descs() or []
    guids = []
    for desc in descriptions:
        try:
            guids.append(desc.get_editor_property("guid"))
        except Exception as exc:
            raise RuntimeError("Cannot read every World Partition actor GUID in " + map_package + ": " + str(exc))
    if guids:
        unreal.WorldPartitionBlueprintLibrary.load_actors(guids)
    return {"present": True, "descriptors": len(guids), "loaded": len(guids)}


def snapshot_map(package, index):
    world = unreal.EditorLoadingAndSavingUtils.load_map(package)
    if not world:
        raise RuntimeError("Map load failed: " + package)
    raw_path = OUTPUT / ("map_%d_raw.json" % index)
    command = "TunaSweeper.RaidLevelAudit " + raw_path.as_posix()
    unreal.SystemLibrary.execute_console_command(world, command)
    if not raw_path.is_file():
        raise RuntimeError("Native audit hook did not write " + str(raw_path))
    first = json.loads(raw_path.read_text(encoding="utf-8-sig"))
    wp = load_external_actors(package, first)
    if wp["present"]:
        raw_path.unlink()
        unreal.SystemLibrary.execute_console_command(world, command)
        if not raw_path.is_file():
            raise RuntimeError("Native audit hook did not write World Partition snapshot")
    result = json.loads(raw_path.read_text(encoding="utf-8-sig"))
    if result["map"] != package:
        raise RuntimeError("Native audit selected wrong world: " + result["map"])
    if wp["present"] and len(result["actors"]) < wp["descriptors"]:
        raise RuntimeError("World Partition actor snapshot incomplete: " + package)
    result["world_partition"] = wp
    result["physical_map_id"] = package
    result["logical_level_id"] = LOGICAL[package]
    result["actors"].sort(key=lambda a: a["path"])
    for actor in result["actors"]:
        actor["components"].sort(key=lambda c: c["path"])
    result["actor_references"] = actor_reference_edges(result)
    return result


def actor_reference_edges(map_data):
    """Resolve exported object paths to other saved actors in the same level."""
    actors = {actor["path"] for actor in map_data["actors"]}
    pattern = re.compile(r"(/[^'\"(),\s]+:PersistentLevel\.[A-Za-z0-9_]+)")
    edges = []
    for actor in map_data["actors"]:
        sources = [("actor/" + name, value) for name, value in actor["properties"].items()]
        for component in actor["components"]:
            sources.extend(("component/" + component["name"] + "/" + name, value)
                           for name, value in component["properties"].items())
            sources.append(("component/" + component["name"] + "/attach_parent", component.get("attach_parent", "")))
        sources.append(("attachment_parent_actor", actor.get("attachment_parent_actor", "")))
        for field, text in sources:
            for target in sorted(set(pattern.findall(text))):
                if target in actors and target != actor["path"]:
                    edges.append({"from_actor": actor["path"], "field": field, "to_actor": target})
    return sorted(edges, key=lambda edge: (edge["from_actor"], edge["field"], edge["to_actor"]))

def anchor_info(actor):
    cls = actor["class"]
    if "RaidPlacementAnchor" not in cls:
        return None
    props = actor["properties"]
    required = ("PlacementId", "AnchorKind", "bAllowDuplicatePlacementId")
    missing = [name for name in required if name not in props]
    if missing:
        raise RuntimeError("Anchor reflected properties missing: " + actor["path"] + " " + repr(missing))
    try:
        placement_id = int(props["PlacementId"])
    except ValueError as exc:
        raise RuntimeError("Invalid PlacementId export " + actor["path"]) from exc
    kind_text = props["AnchorKind"]
    kind = kind_text.split("::")[-1].strip('"')
    expected_ordinals = {"Enemy": 0, "LootContainer": 1, "Memo": 2, "AuthoredActor": 3}
    if kind not in expected_ordinals:
        raise RuntimeError("Unrecognized AnchorKind: " + kind_text)
    actual_ordinal = actor.get("enum_values", {}).get("AnchorKind")
    if type(actual_ordinal) is not int or actual_ordinal != expected_ordinals[kind]:
        raise RuntimeError("AnchorKind serialized enum ordinal mismatch for %s: %s=%r, expected %d" %
                           (actor["path"], kind, actual_ordinal, expected_ordinals[kind]))
    return {"actor": actor["path"], "placement_id": placement_id, "kind": kind,
            "anchor_kind_value": actual_ordinal,
            "allow_duplicate": props["bAllowDuplicatePlacementId"].lower() in ("true", "1")}


def placement_data():
    all_rows = []
    base = PROJECT / "Content" / "Data"
    for dataset in ("Public", "MainRuntimeDefaults"):
        directory = base if dataset == "Public" else base / dataset
        for filename in GAME_DATA:
            path = directory / filename
            if not path.is_file():
                continue
            content = json.loads(path.read_text(encoding="utf-8-sig"))
            if not isinstance(content, list):
                raise RuntimeError("Placement data is not a JSON array: " + str(path))
            for row in content:
                all_rows.append({"dataset": dataset, "file": path.relative_to(PROJECT).as_posix(),
                                 "kind": KIND_BY_DATA[filename], "logical_level_id": row.get("level_name"),
                                 "placement_id": row.get("placement_id"), "profile_id": row.get("profile_id") or row.get("container_definition_id") or row.get("memo_id"),
                                 "row": row})
    return all_rows


def audit_ids(maps, data_rows):
    findings = []
    anchors = []
    links = []
    for map_data in maps:
        physical = map_data["physical_map_id"]
        logical = map_data["logical_level_id"]
        groups = {}
        for actor in map_data["actors"]:
            anchor = anchor_info(actor)
            if not anchor:
                continue
            anchor["physical_map_id"] = physical
            anchor["logical_level_id"] = logical
            anchors.append(anchor)
            groups.setdefault(anchor["placement_id"], []).append(anchor)
            bounds = {"Enemy": (1, 999), "LootContainer": (1000, 1999), "Memo": (2000, 2999)}[anchor["kind"]]
            if not bounds[0] <= anchor["placement_id"] <= bounds[1]:
                findings.append({"type": "range", "anchor": anchor})
        for ident, group in groups.items():
            if len(group) > 1 and not all(a["kind"] == "Enemy" and a["allow_duplicate"] for a in group):
                findings.append({"type": "invalid_duplicate", "physical_map_id": physical, "placement_id": ident, "actors": [a["actor"] for a in group]})
        for row in data_rows:
            if row["logical_level_id"] != logical:
                continue
            matched = [a for a in groups.get(row["placement_id"], []) if a["kind"] == row["kind"]]
            links.append({"physical_map_id": physical, "logical_level_id": logical,
                          "placement_id": row["placement_id"], "kind": row["kind"],
                          "profile_id": row["profile_id"], "dataset": row["dataset"],
                          "anchor_actors": [a["actor"] for a in matched]})
            if not matched:
                findings.append({"type": "data_without_matching_anchor", "physical_map_id": physical,
                                 "logical_level_id": logical, "placement_id": row["placement_id"],
                                 "kind": row["kind"], "dataset": row["dataset"]})
            if any(key in row["row"] for key in ("location", "rotation", "scale")):
                findings.append({"type": "data_contains_transform", "file": row["file"], "placement_id": row["placement_id"]})
    for anchor in anchors:
        if not any(link["physical_map_id"] == anchor["physical_map_id"] and link["placement_id"] == anchor["placement_id"] and link["kind"] == anchor["kind"] for link in links):
            findings.append({"type": "anchor_without_public_profile", "anchor": anchor})
    return {"anchors": anchors, "profile_links": links, "findings": findings}


def saved_asset_hashes(packages):
    hashes = {}
    for package in packages:
        if package.startswith("/Game/"):
            base = PROJECT / "Content" / package[len("/Game/"):]
        elif package.startswith(("/RaidLevelKit/", "/TunaRaidMaps/")):
            mount = package.split("/")[1]
            base = PROJECT / "Plugins" / mount / "Content" / package[len(mount) + 2:]
        else:
            continue
        candidates = (base.with_suffix(".umap"), base.with_suffix(".uasset"))
        existing = [path for path in candidates if path.is_file()]
        if len(existing) != 1:
            raise RuntimeError("Expected exactly one saved asset for " + package)
        path = existing[0]
        hashes[package] = {"file": path.relative_to(PROJECT).as_posix(),
                           "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                           "bytes": path.stat().st_size}
    return hashes

def main():
    registry = unreal.AssetRegistryHelpers.get_asset_registry()
    registry.wait_for_completion()
    roots = list(MAPS + ROOT_ASSETS)
    for mount in ("/RaidLevelKit", "/TunaRaidMaps"):
        roots.extend(str(asset.package_name) for asset in registry.get_assets_by_path(mount, recursive=True) or [])
    maps = [snapshot_map(path, index) for index, path in enumerate(MAPS)]
    rows = placement_data()
    ids = audit_ids(maps, rows)
    graph = dependency_graph(registry, roots)
    hard, soft = options(True, False), options(False, True)
    reverse = {root: {"hard": query(registry, "get_referencers", root, hard),
                      "soft": query(registry, "get_referencers", root, soft)} for root in roots}
    save("reverse_references.json", {"schema": 1, "roots": reverse})
    save(OUTPUT_FILE, {"schema": 1, "maps": maps, "root_packages": sorted(set(roots)),
                         "dependency_graph": graph, "placement_data": rows, "id_audit": ids,
                         "saved_root_assets": saved_asset_hashes(roots)})
    unreal.log_warning("RAID_AUDIT_COMPLETE maps=%d actors=%d graph_packages=%d anchors=%d findings=%d" %
                       (len(maps), sum(len(m["actors"]) for m in maps), len(graph), len(ids["anchors"]), len(ids["findings"])))


main()
