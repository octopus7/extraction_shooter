"""Copy the existing side-frame stripe cells into the deployable rail UV cells.

The first three top-row frame cells are preserved exactly. Each deployable rail
copies its matching side frame into a separate UV cell, while the unused
bottom-right cell stays plain for the recessed pocket's internal faces.
"""

from pathlib import Path

from PIL import Image


ATLAS_COLUMNS = 4
ATLAS_ROWS = 3
RAIL_CELL_COPIES = (
    ((1, 0), (2, 1)),
    ((2, 0), (3, 1)),
)

ROOT = Path(__file__).resolve().parents[2]
ATLAS_PATH = (
    ROOT
    / "TunaSweeper"
    / "SourceArt"
    / "Environment"
    / "BunkerGarageDoor"
    / "Textures"
    / "T_GarageDoor_PartsAtlas_BaseColor.png"
)
def main() -> None:
    image = Image.open(ATLAS_PATH).convert("RGB")
    width, height = image.size
    if width % ATLAS_COLUMNS != 0 or height % ATLAS_ROWS != 0:
        raise ValueError(f"Atlas size must divide into 4x3 cells: {image.size}")

    cell_width = width // ATLAS_COLUMNS
    cell_height = height // ATLAS_ROWS
    # Rails retain their own UV cells but visually match the corresponding side
    # frame. Existing frame pixels are never modified.
    for (source_column, source_row), (target_column, target_row) in RAIL_CELL_COPIES:
        source_box = (
            source_column * cell_width,
            source_row * cell_height,
            (source_column + 1) * cell_width,
            (source_row + 1) * cell_height,
        )
        image.paste(
            image.crop(source_box),
            (target_column * cell_width, target_row * cell_height),
        )

    image.save(ATLAS_PATH, optimize=True)
    print(
        f"Updated {ATLAS_PATH}: preserved original frame cells, "
        "copied matching stripes to rail cells, recess interior remains plain"
    )


if __name__ == "__main__":
    main()
