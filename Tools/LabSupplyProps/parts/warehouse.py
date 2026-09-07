"""Warehouse geometry for the shared LabSupplyProps builder.

Coordinates are meters; fronts face -Y and asset origins are bottom center.
Detail markings, tape, latches and wear come from the ImageGen shared atlas.
"""


def _box(lo, hi, tile, **face_tiles):
    part = {"type": "box", "lo": list(lo), "hi": list(hi), "tile": tile}
    if face_tiles:
        part["face_tiles"] = face_tiles
    return part


def _pallet():
    parts = []
    # Bottom runners, nine blocks, and transverse bearers produce real fork
    # passages from all four sides. Collision follows the individual solids.
    for x in (-0.525, 0.0, 0.525):
        parts.append(_box((x - 0.075, -0.50, 0.0),
                          (x + 0.075, 0.50, 0.025), 8))
        for y in (-0.425, 0.0, 0.425):
            parts.append(_box((x - 0.075, y - 0.075, 0.025),
                              (x + 0.075, y + 0.075, 0.105), 8))
    for y in (-0.425, 0.0, 0.425):
        parts.append(_box((-0.60, y - 0.075, 0.105),
                          (0.60, y + 0.075, 0.125), 8))
    # Five broad deck slats; 25 mm gaps remain visible from top-down.
    for x in (-0.49, -0.245, 0.0, 0.245, 0.49):
        parts.append(_box((x - 0.11, -0.50, 0.125),
                          (x + 0.11, 0.50, 0.15), 8))
    return {
        "key": "Pallet",
        "label": "Empty wooden pallet",
        "dimensions": [1.20, 1.00, 0.15],
        "parts": parts,
        "collision": [[p["lo"][:], p["hi"][:]] for p in parts],
        "notes": [
            "Deck top Z=0.15 m; center a BoxBundle here or place two SupplyCrates side by side.",
            "Four-way fork clearance: Y approach has two 0.375 m-wide openings; X approach has two 0.275 m-wide openings. Both have 0.080 m clear height.",
            "Twenty box collision pieces preserve deck gaps and fork passages. Nails and wood wear are texture-only.",
        ],
    }


def _supply_crate():
    parts = [
        # A solid closed crate; no lid mechanism or loot functionality.
        _box((-0.29, -0.19, 0.025), (0.29, 0.19, 0.35), 0, front=11),
        _box((-0.295, -0.195, 0.35), (0.295, 0.195, 0.393), 1, top=12),
        _box((-0.295, -0.195, 0.0), (0.295, 0.195, 0.025), 2),
    ]
    # Broad protective corners affect silhouette; latches/handles stay baked.
    # Lid is inset 7 mm below the caps to avoid coplanar top surfaces.
    for xlo, xhi in ((-0.30, -0.235), (0.235, 0.30)):
        for ylo, yhi in ((-0.20, -0.14), (0.14, 0.20)):
            parts.append(_box((xlo, ylo, 0.305), (xhi, yhi, 0.40), 1))
            parts.append(_box((xlo, ylo, 0.0), (xhi, yhi, 0.055), 1))
    return {
        "key": "SupplyCrate",
        "label": "Sealed supply crate",
        "dimensions": [0.60, 0.40, 0.40],
        "parts": parts,
        "collision": [[[-0.30, -0.20, 0.0], [0.30, 0.20, 0.40]]],
        "notes": [
            "Stack at 0.40 m pitch. Corner caps provide aligned bearing faces; lid stays inset.",
            "Two crates span 1.20 m across a rack tier with at least 1.28 m clear width; depth 0.40 m fits its at least 0.48 m usable depth.",
            "One convex box collision is suitable for this sealed solid prop. Latches, recessed handles, labels and lid markings are texture detail only.",
            "Independent environment prop; existing loot crate assets, blueprints and gameplay remain untouched.",
        ],
    }


def _box_bundle():
    parts = [
        _box((-0.50, -0.35, 0.0), (-0.12, 0.20, 0.48), 9, front=10),
        _box((-0.10, -0.025, 0.0), (0.50, 0.35, 0.70), 9, front=10),
        _box((-0.06, -0.35, 0.0), (0.40, -0.045, 0.33), 9, front=10),
    ]
    return {
        "key": "BoxBundle",
        "label": "Supply box bundle (three cartons)",
        "dimensions": [1.00, 0.70, 0.70],
        "parts": parts,
        "collision": [[p["lo"][:], p["hi"][:]] for p in parts],
        "notes": [
            "Three differently sized upright closed cartons, joined as one prop with bottom-centered pivot.",
            "Center on the pallet at Z=0.15 m: margins are 0.10 m in X and 0.15 m in Y; overall loaded height is 0.85 m.",
            "This pallet bundle exceeds rack tier depth; use separate SupplyCrates and SampleTray for shelf loads.",
            "Three box collision pieces follow the stepped profile. Shipping labels appear on -Y faces only; tape, folds and wear are baked atlas detail.",
        ],
    }


def assets():
    """Return geometry data only; never touch Blender state or shared files."""
    return [_pallet(), _supply_crate(), _box_bundle()]
