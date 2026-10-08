"""Read-only contracts for the two independent axes: edition and distribution channel."""
from pathlib import Path
import configparser
import unittest

ROOT = Path(__file__).resolve().parents[2] / "TunaSweeper"
TARGETS = {
    "NoStoreFull": ("TunaSweeperNoStore", "NoStore", "None", "Full"),
    "NoStoreDemo": ("TunaSweeperNoStoreDemo", "NoStoreDemo", "None", "Demo"),
    "SteamFull": ("TunaSweeper", "Full", "Steam", "Full"),
    "SteamDemo": ("TunaSweeperDemo", "Demo", "Steam", "Demo"),
    "StoveFull": ("TunaSweeperStove", "Stove", "Stove", "Full"),
    "StoveDemo": ("TunaSweeperStoveDemo", "StoveDemo", "Stove", "Demo"),
}

def ini(path):
    parser = configparser.ConfigParser(strict=False, interpolation=None)
    parser.read(path, encoding="utf-8-sig")
    return parser

class BuildTargetTests(unittest.TestCase):
    def test_saved_preview_matches_packaging_target(self):
        config = ini(ROOT / "Config/DefaultGame.ini")
        selected = config["/Script/TunaSweeper.TunaSweeperBuildTargetSettings"]["BuildTarget"]
        self.assertEqual(config["/Script/UnrealEd.ProjectPackagingSettings"]["BuildTarget"], TARGETS[selected][0])

    def test_all_six_target_editions_and_channels(self):
        for selected, (target, custom, channel, edition) in TARGETS.items():
            with self.subTest(target=selected):
                source = (ROOT / f"Source/{target}.Target.cs").read_text(encoding="utf-8-sig")
                self.assertIn(f'CustomConfig = "{custom}"', source)
                self.assertIn(f'TUNASWEEPER_DEMO={int(edition == "Demo")}', source)
                config = ini(ROOT / f"Config/Custom/{custom}/DefaultGame.ini")
                self.assertEqual(config["TunaSweeper.Distribution"]["DistributionChannel"], channel)
                self.assertEqual(config["TunaSweeper.Distribution"]["BuildType"], edition)

if __name__ == "__main__":
    unittest.main()
