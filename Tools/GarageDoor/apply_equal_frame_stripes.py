"""Apply exact equal-width 45-degree gray stripes to garage-door frame atlas cells.

The existing image-generated metal atlas remains the texture source. Only the
first three cells in the top row are modified; all other pixels are preserved.
"""

from pathlib import Path

from PIL import Image


ATLAS_COLUMNS = 4
ATLAS_ROWS = 3
FRAME_CELL_COUNT = 3
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

    for cell_index in range(FRAME_CELL_COUNT):
        cell_x = cell_index * cell_width
        for local_y in range(cell_height):
            for local_x in range(cell_width):
                # Constant local_x + local_y produces an exact 45-degree line on
                # square pixels. Alternating equal intervals give a 1:1 duty cycle.
                stripe_index = ((local_x + local_y) // STRIPE_WIDTH_PX) % 2
                tint = LIGHT_TINT if stripe_index == 0 else DARK_TINT
                # Use the corresponding unmarked middle-row cell as this frame
                # cell's unique metal grain so the previous double stripes cannot
                # bleed through the new equal-width pattern.
                source = source_pixels[cell_x + local_x, cell_height + local_y]
                pixels[cell_x + local_x, local_y] = tuple(
                    blend_channel(source[channel], tint[channel])
                    for channel in range(3)
                )

    image.save(ATLAS_PATH, optimize=True)
    print(
        f"Updated {ATLAS_PATH}: 45-degree stripes, "
        f"{STRIPE_WIDTH_PX}px light / {STRIPE_WIDTH_PX}px dark"
    )


if __name__ == "__main__":
    main()
