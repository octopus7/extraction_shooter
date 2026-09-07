"""Stack the three pair-comparison renders into one review catalog."""
from pathlib import Path
import shutil
import subprocess


ROOT = Path(__file__).resolve().parents[2]
PREVIEWS = ROOT / "TunaSweeper/SourceArt/Weapons/TunaWeaponCollection/Previews"
sources = [PREVIEWS / f"{name}_PairComparison.png" for name in ("SMG", "AR", "Pistol")]
assert all(path.is_file() for path in sources), sources
magick = shutil.which("magick")
assert magick, "ImageMagick is required to assemble the comparison catalog"
destination = PREVIEWS / "All_Comparison.png"
subprocess.run([magick, "-background", "#10171d", "-gravity", "center", *map(str, sources), "-append", str(destination)], check=True)
print("WEAPON_CATALOG_RENDERED", destination)
