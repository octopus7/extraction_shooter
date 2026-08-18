"""Apply exact equal-width 45-degree gray stripes to frame and rail atlas cells.

The image-generated atlas remains the texture source. Frame cells use stable
plain-metal source cells from the bottom row, and each deployable rail copies
its matching side frame into a separate UV cell. The unused bottom-right cell
stays plain for the recessed pocket's internal faces.
"""

from pathlib import Path

from PIL import Image


ATLAS_COLUMNS = 4
ATLAS_ROWS = 3
FRAME_CELLS = ((0, 0), (1, 0), (2, 0))
FRAME_SOURCE_CELLS = ((0, 2), (1, 2), (2, 2))
RAIL_CELL_COPIES = (
    ((1, 0), (2, 1)),
    ((2, 0), (3, 1)),
)
STRIPE_WIDTH_PX = 48
LIGHT_TINT = (202, 201, 196)
DARK_TINT = (132, 134, 132)
TINT_MIX = 0.58

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


def blend_channel(source: int, tint: int) -> int:
    return round(source * (1.0 - TINT_MIX) + tint * TINT_MIX)


def main() -> None:
    image = Image.open(ATLAS_PATH).convert("RGB")
    width, height = image.size
    if width % ATLAS_COLUMNS != 0 or height % ATLAS_ROWS != 0:
        raise ValueError(f"Atlas size must divide into 4x3 cells: {image.size}")

    cell_width = width // ATLAS_COLUMNS
    cell_height = height // ATLAS_ROWS
    source_image = image.copy()
    source_pixels = source_image.load()
    pixels = image.load()

    for (cell_column, cell_row), (source_column, source_row) in zip(
        FRAME_CELLS,
        FRAME_SOURCE_CELLS,
        strict=True,
    ):
        cell_x = cell_column * cell_width
        cell_y = cell_row * cell_height
        source_x = source_column * cell_width
        source_y = source_row * cell_height
        for local_y in range(cell_height):
            for local_x in range(cell_width):
                # Constant local_x + local_y produces an exact 45-degree line on
                # square pixels. Alternating equal intervals give a 1:1 duty cycle.
                stripe_index = ((local_x + local_y) // STRIPE_WIDTH_PX) % 2
                tint = LIGHT_TINT if stripe_index == 0 else DARK_TINT
                # The bottom row is stable, plain source metal. This makes the
                # operation repeatable even after the rail cells become striped.
                source = source_pixels[source_x + local_x, source_y + local_y]
                pixels[cell_x + local_x, cell_y + local_y] = tuple(
                    blend_channel(source[channel], tint[channel])
                    for channel in range(3)
                )

    # Rails retain their own UV cells but visually match the corresponding side
    # frame. Copy after regenerating the frame cells so this is also idempotent.
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
        f"Updated {ATLAS_PATH}: frame + rail 45-degree stripes, "
        f"{STRIPE_WIDTH_PX}px light / {STRIPE_WIDTH_PX}px dark; "
        "recess interior cell remains plain"
    )


if __name__ == "__main__":
    main()
