"""Focused pure-Python contract tests for the raid migration verifier."""
import ast
import copy
import unittest
from pathlib import Path

from verify_migration import check_independence, compare_manifests


def example():
    actor = {"path": "/Raid/Map.Map:PersistentLevel.Box_1", "class": "/Script/Engine.StaticMeshActor",
             "class_chain": ["/Script/Engine.StaticMeshActor", "/Script/Engine.Actor"],
             "level": "/Raid/Map", "label": "Box", "name": "Box_1", "transform": "(1,2,3)",
             "tags": ["Authored"], "attachment_parent_actor": "", "attachment_socket": "None",
             "properties": {"EditableValue": "7", "Reference": "/Raid/Mesh"}, "enum_values": {},
             "components": [{"name": "StaticMeshComponent0", "path": "/Raid/Map.Box_1.Mesh", "class": "/Script/Engine.StaticMeshComponent",
                             "properties": {"StaticMesh": "/Raid/Mesh"}, "attach_parent": ""}]}
    return {"schema": 1, "root_packages": ["/Raid/Map"], "saved_root_assets": {"/Raid/Map": {"sha256": "a" * 64, "bytes": 1}}, "dependency_graph": {"/Raid/Map": {"hard": ["/Script/Engine"], "soft": []}},
            "maps": [{"physical_map_id": "/Raid/Map", "actors": [actor], "levels": [{"blueprint": {"exists": False}}], "world_partition": {"present": False}, "actor_references": []}]}


class MigrationVerifierTests(unittest.TestCase):
    def test_transitive_plugin_game_dependency_fails(self):
        data = example()
        data["dependency_graph"] = {"/Raid/Map": {"hard": [], "soft": []},
            "/RaidLevelKit/Anchor": {"hard": ["/Environment/Material"], "soft": []},
            "/Environment/Material": {"hard": [], "soft": ["/Game/HiddenTexture"]},
        }
        data["root_packages"].append("/RaidLevelKit/Anchor")
        self.assertTrue(any("/Game/HiddenTexture" in error for error in check_independence(data)))

    def test_truncated_transitive_graph_fails(self):
        data = example()
        data["dependency_graph"]["/Raid/Map"]["hard"] = ["/Environment/Missing"]
        self.assertTrue(any("missing transitive dependency node" in error for error in check_independence(data)))

    def test_omitted_plugin_root_fails(self):
        data = example()
        data["dependency_graph"]["/RaidLevelKit/Anchor"] = {"hard": [], "soft": []}
        self.assertTrue(any("plugin package omitted" in error for error in check_independence(data)))

    def test_anchor_numeric_ordinal_mismatch_fails(self):
        data = example()
        actor = data["maps"][0]["actors"][0]
        actor["class"] = "/Game/Raid/BP_RaidPlacementAnchor.BP_RaidPlacementAnchor_C"
        actor["properties"].update({"PlacementId": "1", "AnchorKind": "Enemy", "bAllowDuplicatePlacementId": "False"})
        actor["enum_values"]["AnchorKind"] = 1
        self.assertTrue(any("serialized ordinal mismatch" in error for error in check_independence(data)))

    def test_authored_actor_ordinal_contract(self):
        data = example()
        actor = data["maps"][0]["actors"][0]
        actor["class"] = "/RaidLevelKit/Placement/BP_RaidPlacementAnchor_C"
        actor["properties"]["AnchorKind"] = "AuthoredActor"
        actor["enum_values"]["AnchorKind"] = 3
        self.assertFalse(check_independence(data))
        actor["enum_values"]["AnchorKind"] = 2
        self.assertTrue(any("serialized ordinal mismatch" in error for error in check_independence(data)))

    def test_auditor_reads_numeric_ordinal(self):
        source = Path(__file__).with_name("audit_levels.py").read_text(encoding="utf-8-sig")
        node = next(node for node in ast.parse(source).body if isinstance(node, ast.FunctionDef) and node.name == "anchor_info")
        scope = {}
        exec(compile(ast.Module(body=[node], type_ignores=[]), "audit_levels.py", "exec"), scope)
        actor = {"path": "/Raid/Map.Anchor", "class": "/Game/Raid/BP_RaidPlacementAnchor_C",
                 "properties": {"PlacementId": "1", "AnchorKind": "Enemy", "bAllowDuplicatePlacementId": "False"},
                 "enum_values": {"AnchorKind": 0}}
        for symbol, ordinal in (("Enemy", 0), ("LootContainer", 1), ("Memo", 2), ("AuthoredActor", 3)):
            actor["properties"]["AnchorKind"] = symbol
            actor["enum_values"]["AnchorKind"] = ordinal
            self.assertEqual(ordinal, scope["anchor_info"](actor)["anchor_kind_value"])
        actor["properties"]["AnchorKind"] = "Enemy"
        actor["enum_values"]["AnchorKind"] = 2
        with self.assertRaisesRegex(RuntimeError, "ordinal mismatch"):
            scope["anchor_info"](actor)
    def test_game_ancestor_and_blueprint_reference_fail(self):
        data = example()
        data["maps"][0]["actors"][0]["class_chain"].insert(0, "/Script/TunaSweeper.CustomActor")
        data["maps"][0]["levels"][0]["blueprint"] = {"graph": "/Script/TunaSweeper.GameState"}
        errors = check_independence(data)
        self.assertTrue(any("game Actor" in error for error in errors))
        self.assertTrue(any("Level Blueprint" in error for error in errors))

    def test_missing_reflection_fails_closed(self):
        data = example()
        del data["maps"][0]["actors"][0]["class_chain"]
        self.assertTrue(any("incomplete actor snapshot" in error for error in check_independence(data)))

    def test_missing_saved_map_hash_fails_closed(self):
        data = example()
        del data["saved_root_assets"]["/Raid/Map"]
        self.assertTrue(any("missing saved map hash" in error for error in check_independence(data)))
    def test_unchanged_saved_snapshot_matches(self):
        data = example()
        self.assertEqual([], compare_manifests(data, copy.deepcopy(data), {}))

    def test_property_and_attachment_loss_detected(self):
        before = example()
        after = copy.deepcopy(before)
        after["maps"][0]["actors"][0]["properties"].pop("EditableValue")
        after["maps"][0]["actors"][0]["components"][0]["attach_parent"] = "/Raid/Other"
        differences = compare_manifests(before, after, {})
        self.assertTrue(any("EditableValue" in difference for difference in differences))
        self.assertTrue(any("attach_parent" in difference for difference in differences))

    def test_missing_actor_requires_explicit_runtime_reconstruction(self):
        before = example()
        after = copy.deepcopy(before)
        actor = after["maps"][0]["actors"].pop()
        self.assertTrue(any("missing actor" in difference for difference in compare_manifests(before, after, {})))
        actor["path"] = "/Runtime/Box_1"
        after["runtime_actors"] = [actor]
        rules = {"actor_replacements": {before["maps"][0]["actors"][0]["path"]: actor["path"]},
                 "allow_differences": ["actor//Raid/Map.Map:PersistentLevel.Box_1/path"]}
        self.assertEqual([], compare_manifests(before, after, rules))


if __name__ == "__main__":
    unittest.main()
