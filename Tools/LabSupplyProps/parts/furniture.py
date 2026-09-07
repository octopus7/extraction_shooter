"""Five low-poly furniture definitions in meters; origin ground center, front -Y.

Reference: ImageGen_Concept12.png, items 01--05. Cabinet detail is atlas-only.
No Blender operations or filesystem side effects occur in this module.
"""


def _box(lo, hi, tile, **faces):
    part = {"type": "box", "lo": list(lo), "hi": list(hi), "tile": tile}
    if faces:
        part["face_tiles"] = faces
    return part


def _asset(key, label, dimensions, parts, notes, collision=None):
    if collision is None:
        collision = [[part["lo"], part["hi"]] for part in parts
                     if part["type"] == "box"]
    return {"key": key, "label": label, "dimensions": list(dimensions),
            "parts": parts, "collision": collision, "notes": notes}


def _workbench():
    parts = [_box((-.8, -.35, .85), (.8, .35, .9), 0, top=3)]
    # Continuous open underside. Narrow apron and low stretchers support the top.
    for y in (-.315, .285):
        parts.append(_box((-.755, y, .79), (.755, y + .03, .85), 1))
        parts.append(_box((-.755, y, .145), (.755, y + .03, .18), 1))
    for x in (-.755, .715):
        parts.append(_box((x, -.285, .79), (x + .04, .285, .85), 1))
        parts.append(_box((x, -.285, .145), (x + .04, .285, .18), 1))
    for x in (-.755, .705):
        for y in (-.315, .265):
            parts.append(_box((x, y, .035), (x + .05, y + .05, .79), 0))
            parts.append(_box((x, y, 0), (x + .05, y + .05, .035), 14))
    return _asset("Workbench", "Empty metal workbench", (1.6, .7, .9), parts,
                  "Deck Z=.90; usable 1.60 x .70 m. Empty underframe with real "
                  "leg and stretcher gaps. Shares footprint and deck height with sink.")


def _shelf():
    parts = []
    for x in (-.7, .65):
        for y in (-.3, .25):
            parts.append(_box((x, y, 0), (x + .05, y + .05, 2), 1))
    # No back wall, diagonal brace, contents or concealed inner construction.
    for top in (.12, .72, 1.32, 1.92):
        parts.append(_box((-.65, -.25, top - .035), (.65, .25, top), 1, top=3))
        # Front and back teal-edged rails terminate against the corner uprights.
        for y in (-.3, .25):
            parts.append(_box((-.65, y, top - .055), (.65, y + .05, top), 2))
        for x in (-.7, .65):
            parts.append(_box((x, -.25, top - .055), (x + .05, .25, top), 1))
    return _asset("Shelf", "Empty steel shelving rack", (1.4, .6, 2), parts,
                  "Tier tops .12/.72/1.32/1.92 m; inner bay 1.30 x .50 m, "
                  "vertical clear .545 m. Two .60 x .40 x .40 m crates fit "
                  "side by side per tier. Place against room perimeter for camera clearance.")


def _cabinet():
    # Door outlines, drawer breaks, handles and hinges exist only in tile 4.
    parts = [_box((-.57, -.27, 0), (.57, .27, .085), 1),
             _box((-.59, -.29, .085), (.59, .29, .85), 0, front=4),
             _box((-.6, -.3, .85), (.6, .3, .9), 0, top=3)]
    return _asset("Cabinet", "Low storage cabinet", (1.2, .6, .9), parts,
                  "Sealed decorative cabinet; all drawer, door and handle detail "
                  "comes from front tile 4. Deck Z=.90; no interaction or internal geometry.")


def _cart():
    parts = [_box((-.4, -.25, .14), (.4, .25, .175), 1, top=3),
             _box((-.4, -.25, .70), (.4, .25, .735), 0, top=3)]
    for x in (-.385, .355):
        for y in (-.23, .20):
            parts.append(_box((x, y, .12), (x + .03, y + .03, .77), 0))
    # Three small top retaining lips; front remains open for easy placement.
    parts.extend([_box((-.4, .23, .735), (.4, .25, .77), 0),
                  _box((-.4, -.25, .735), (-.38, .23, .77), 0),
                  _box((.38, -.25, .735), (.4, .23, .77), 0)])
    for y in (-.225, .20):
        parts.append(_box((.37, y, .77), (.395, y + .025, .834), 1))
    parts.append({"type": "cylinder", "center": [.384, 0, .834],
                  "radius": .016, "depth": .45, "axis": "Y", "sides": 8, "tile": 2})
    collision = [[p["lo"], p["hi"]] for p in parts if p["type"] == "box"]
    collision.append([[.368, -.225, .818], [.4, .225, .85]])
    for x in (-.355, .355):
        for y in (-.19, .19):
            parts.append({"type": "cylinder", "center": [x, y, .06],
                          "radius": .06, "depth": .05, "axis": "X", "sides": 8,
                          "tile": 14})
            # One conservative box per wheel; no collision hull fills undercart space.
            collision.append([[x - .025, y - .06, 0], [x + .025, y + .06, .12]])
    return _asset("Cart", "Rolling utility cart", (.8, .5, .85), parts,
                  "Top deck Z=.735 and lower deck Z=.175; handle top Z=.85. "
                  "Four static 8-side wheels; no physics or movement feature. "
                  "Sample tray fits both decks.", collision)


def _chair():
    seat = [[-.21, -.26], [.21, -.26], [.24, -.23], [.24, .20],
            [.21, .23], [-.21, .23], [-.24, .20], [-.24, -.23]]
    parts = [{"type": "prism", "polygon": seat, "depth": [.39, .415],
              "plane": "XY", "tile": 1},
             {"type": "prism", "polygon": seat, "depth": [.415, .45],
              "plane": "XY", "tile": 2}]
    collision = [[[-.24, -.26, .39], [.24, .23, .45]]]
    # Slanted leg prisms splay in Y with a four-sided section, avoiding high-cost tubes.
    for x in (-.205, .175):
        for foot_y, upper_y in ((-.25, -.195), (.225, .17)):
            poly = [[foot_y, 0], [foot_y + .03, 0],
                    [upper_y + .03, .39], [upper_y, .39]]
            parts.append({"type": "prism", "polygon": poly,
                          "depth": [x, x + .03], "plane": "YZ", "tile": 1})
            # Split slanted collision into three narrow boxes, preserving leg space.
            for tier in range(3):
                z0, z1 = .39 * tier / 3, .39 * (tier + 1) / 3
                y0 = foot_y + (upper_y - foot_y) * tier / 3
                y1 = foot_y + (upper_y - foot_y) * (tier + 1) / 3
                collision.append([[x, min(y0, y1), z0],
                                  [x + .03, max(y0, y1) + .03, z1]])
    for x in (-.185, .16):
        post = _box((x, .205, .45), (x + .025, .23, .605), 1)
        parts.append(post)
        collision.append([post["lo"], post["hi"]])
    back = [[-.21, .57], [.21, .57], [.23, .59], [.23, .78],
            [.21, .80], [-.21, .80], [-.23, .78], [-.23, .59]]
    parts.append({"type": "prism", "polygon": back, "depth": [.22, .26],
                  "plane": "XZ", "tile": 2})
    collision.append([[ -.23, .22, .57], [.23, .26, .8]])
    return _asset("Chair", "Low-back work chair", (.48, .52, .8), parts,
                  "Seat Z=.45, low back Z=.80; clipped-corner seat and back for "
                  "top-down readability. No arms, upholstery seams or interactive feature.",
                  collision)


def assets():
    """Return fresh dictionaries for all furniture, in the concept sheet's order."""
    return [_workbench(), _shelf(), _cabinet(), _cart(), _chair()]
