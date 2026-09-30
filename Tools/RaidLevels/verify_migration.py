"""Compare saved UE raid audit manifests and enforce plugin independence.

Usage:
  python verify_migration.py --check-only Saved/RaidLevelMigration/before.json
  python verify_migration.py --before before.json --after after.json [--equivalence rules.json]

A deliberate actor replacement must be named in rules.json. The replacement
snapshot may be in after.json's maps[].actors or runtime_actors; all retained
serialized properties/components/references are compared unless an explicit
per-actor field exception is listed. This is intentionally strict.
"""
import argparse
import json
import pathlib
import sys

PLUGIN_PREFIXES = ("/RaidLevelKit/", "/TunaRaidMaps/")
FORBIDDEN_CLASS_PREFIX = "/Script/TunaSweeper."


def string_leaves(value):
    if isinstance(value, str):
        yield value
    elif isinstance(value, list):
        for child in value:
            yield from string_leaves(child)
    elif isinstance(value, dict):
        for child in value.values():
            yield from string_leaves(child)


def validate_manifest(manifest):
    errors = []
    if manifest.get("schema") != 1:
        errors.append("unsupported or missing manifest schema")
    graph = manifest.get("dependency_graph")
    if not isinstance(graph, dict) or not graph:
        errors.append("missing dependency graph")
        return errors
    roots = manifest.get("root_packages")
    if not isinstance(roots, list) or not roots or any(not isinstance(root, str) for root in roots):
        errors.append("missing or invalid root_packages")
        return errors
    for root in roots:
        if root not in graph:
            errors.append("missing dependency graph root %s" % root)
    for package in graph:
        if package.startswith(PLUGIN_PREFIXES) and package not in roots:
            errors.append("plugin package omitted from root_packages: %s" % package)
    for package, deps in graph.items():
        if not isinstance(deps, dict) or any(not isinstance(deps.get(kind), list) for kind in ("hard", "soft")):
            errors.append("incomplete dependency edges for %s" % package)
            continue
        for target in deps["hard"] + deps["soft"]:
            if not isinstance(target, str):
                errors.append("invalid dependency target for %s" % package)
            elif target.startswith("/") and not target.startswith(("/Engine/", "/Script/")) and target not in graph:
                errors.append("missing transitive dependency node %s -> %s" % (package, target))
    maps = manifest.get("maps")
    if not isinstance(maps, list) or not maps:
        errors.append("missing audited maps")
        return errors
    for map_data in maps:
        if not isinstance(map_data, dict):
            errors.append("invalid map entry")
            continue
        package = map_data.get("physical_map_id", "<missing>")
        if not all(key in map_data for key in ("physical_map_id", "actors", "levels", "world_partition", "actor_references")):
            errors.append("incomplete map snapshot %s" % package)
            continue
        if package not in roots or package not in graph:
            errors.append("missing dependency graph root for %s" % package)
        saved = manifest.get("saved_root_assets", {}).get(package)
        if not isinstance(saved, dict) or len(saved.get("sha256", "")) != 64 or saved.get("bytes", 0) <= 0:
            errors.append("missing saved map hash for %s" % package)
        if not isinstance(map_data["levels"], list) or not map_data["levels"]:
            errors.append("missing Level Blueprint snapshot for %s" % package)
        for actor in map_data["actors"]:
            required = ("path", "class", "class_chain", "properties", "components", "transform", "attachment_parent_actor", "tags", "enum_values")
            if not isinstance(actor, dict) or any(key not in actor for key in required):
                errors.append("incomplete actor snapshot in %s" % package)
                break
            if "RaidPlacementAnchor" in actor["class"]:
                symbol = actor["properties"].get("AnchorKind", "").split("::")[-1].strip('"')
                ordinal = actor["enum_values"].get("AnchorKind")
                expected = {"Enemy": 0, "LootContainer": 1, "Memo": 2, "AuthoredActor": 3}.get(symbol)
                if expected is None or type(ordinal) is not int or ordinal != expected:
                    errors.append("AnchorKind serialized ordinal mismatch for %s: %r=%r" %
                                  (actor["path"], symbol, ordinal))
            if not actor["class_chain"]:
                errors.append("missing class hierarchy for actor %s" % actor["path"])
            for component in actor["components"]:
                if not isinstance(component, dict) or any(key not in component for key in ("path", "class", "properties")):
                    errors.append("incomplete component snapshot for %s" % actor["path"])
                    break
    return errors

def check_independence(manifest):
    errors = validate_manifest(manifest)
    if errors:
        return errors
    graph = manifest.get("dependency_graph", {})
    for origin in graph:
        if not origin.startswith(PLUGIN_PREFIXES):
            continue
        todo = [origin]
        seen = set()
        while todo:
            package = todo.pop()
            if package in seen:
                continue
            seen.add(package)
            deps = graph[package]
            for target in deps.get("hard", []) + deps.get("soft", []):
                if target.startswith("/Game/") or target.startswith("/Script/TunaSweeper"):
                    errors.append("plugin dependency %s -> %s (via %s)" % (origin, target, package))
                elif target in graph and target not in seen:
                    todo.append(target)
    for map_data in manifest.get("maps", []):
        map_id = map_data["physical_map_id"]
        for actor in map_data.get("actors", []):
            if any(cls.startswith(FORBIDDEN_CLASS_PREFIX) for cls in actor.get("class_chain", [])):
                errors.append("game Actor %s in %s" % (actor["path"], map_id))
            elif not actor.get("class_chain"):
                errors.append("missing class hierarchy for actor %s" % actor["path"])
        for level in map_data.get("levels", []):
            for leaf in string_leaves(level.get("blueprint", {})):
                if "/Script/TunaSweeper" in leaf:
                    errors.append("game Level Blueprint reference in %s: %s" % (map_id, leaf[:160]))
                    break
        deps = graph[map_id]
        if any(dep.startswith("/Script/TunaSweeper") for dep in deps.get("hard", []) + deps.get("soft", [])):
            errors.append("game script package dependency in %s" % map_id)
    return sorted(set(errors))


def remap(value, replacements):
    if isinstance(value, str):
        for before, after in replacements.items():
            value = value.replace(before, after)
        return value
    if isinstance(value, list):
        return [remap(item, replacements) for item in value]
    if isinstance(value, dict):
        return {key: remap(item, replacements) for key, item in value.items()}
    return value


def compare_values(before, after, context, differences, allowed):
    if context in allowed:
        return
    if type(before) is not type(after):
        differences.append("%s: type %s -> %s" % (context, type(before).__name__, type(after).__name__))
    elif isinstance(before, dict):
        for key in sorted(set(before) | set(after)):
            path = context + "/" + key
            if key not in before or key not in after:
                if path not in allowed:
                    differences.append("%s: field added/removed" % path)
            else:
                compare_values(before[key], after[key], path, differences, allowed)
    elif isinstance(before, list):
        if len(before) != len(after):
            differences.append("%s: length %d -> %d" % (context, len(before), len(after)))
        for index, (left, right) in enumerate(zip(before, after)):
            compare_values(left, right, context + "/" + str(index), differences, allowed)
    elif before != after:
        differences.append("%s: %r -> %r" % (context, before, after))


def comparable_actor(actor):
    result = dict(actor)
    result.pop("level", None)
    result.pop("label", None)  # Editor display labels are not serialized actor identity.
    result["components"] = sorted(result.get("components", []), key=lambda item: item["name"])
    return result


def compare_manifests(before, after, rules):
    changes = rules.get("path_remaps", {})
    actor_replacements = rules.get("actor_replacements", {})
    permitted = set(rules.get("allow_differences", []))
    differences = []
    current_actors = {}
    for map_data in after.get("maps", []):
        for actor in map_data.get("actors", []):
            current_actors[actor["path"]] = actor
    for actor in after.get("runtime_actors", []):
        current_actors[actor["path"]] = actor
    for old_map in before.get("maps", []):
        old_package = old_map["physical_map_id"]
        expected_package = remap(old_package, changes)
        new_map = next((m for m in after.get("maps", []) if m["physical_map_id"] == expected_package), None)
        if not new_map:
            differences.append("missing map %s" % expected_package)
            continue
        compare_values(remap(old_map.get("levels", []), changes), new_map.get("levels", []),
                       "map/" + old_package + "/levels", differences, permitted)
        compare_values(remap(old_map.get("actor_references", []), changes), new_map.get("actor_references", []),
                       "map/" + old_package + "/actor_references", differences, permitted)
        for old_actor in old_map.get("actors", []):
            old_path = old_actor["path"]
            new_path = actor_replacements.get(old_path, remap(old_path, changes))
            new_actor = current_actors.get(new_path)
            if not new_actor:
                differences.append("missing actor or runtime reconstruction %s -> %s" % (old_path, new_path))
                continue
            compare_values(remap(comparable_actor(old_actor), changes), comparable_actor(new_actor),
                           "actor/" + old_path, differences, permitted)
    return differences


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check-only", type=pathlib.Path)
    parser.add_argument("--before", type=pathlib.Path)
    parser.add_argument("--after", type=pathlib.Path)
    parser.add_argument("--equivalence", type=pathlib.Path)
    args = parser.parse_args(argv)
    if args.check_only and (args.before or args.after):
        parser.error("--check-only excludes --before/--after")
    if not args.check_only and not (args.before and args.after):
        parser.error("provide --check-only or both --before and --after")
    source = args.check_only or args.before
    before = json.loads(source.read_text(encoding="utf-8-sig"))
    after = before if args.check_only else json.loads(args.after.read_text(encoding="utf-8-sig"))
    rules = json.loads(args.equivalence.read_text(encoding="utf-8-sig")) if args.equivalence else {}
    errors = check_independence(after)
    if not args.check_only:
        errors.extend(compare_manifests(before, after, rules))
    for error in errors[:100]:
        print("FAIL:", error)
    if len(errors) > 100:
        print("... %d further differences" % (len(errors) - 100))
    print("verification errors=%d" % len(errors))
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main())
