"""Lab fixtures for the shared LabSupplyProps builder (dimensions in metres)."""

from math import cos, pi, sin


def box(lo, hi, tile=0, **faces):
    return dict(type="box", lo=list(lo), hi=list(hi), tile=tile, face_tiles=faces)


def cylinder(center, radius, depth, tile=0, axis="Z"):
    return dict(type="cylinder", center=list(center), radius=radius,
                depth=depth, axis=axis, sides=8, tile=tile)


def octagonal_body(x, y, radius, bottom, top):
    return dict(type="prism", plane="XY",
                polygon=[[x + radius * cos(i * pi / 4),
                          y + radius * sin(i * pi / 4)] for i in range(8)],
                depth=[bottom, top], tile=0, face_tiles={"front": 13})


def solid_collision(parts):
    """Only boxes: this avoids convex collision bridging the fixture openings."""
    return [[p["lo"], p["hi"]] for p in parts if p["type"] == "box"]


def assets():
    control = [
        box((-.3, -.18, 0), (.3, 0, .8), 0),
        box((-.285, -.2, .02), (.285, -.18, .78), 0, front=5),
    ]

    # Intake is an actual open recess, not a dark panel over a solid block.
    analyzer = [
        box((-.6, -.4, 0), (.6, .4, .065), 1),
        box((-.585, -.385, .065), (.585, .385, .665), 0),
        box((-.6, -.4, .665), (.6, .4, .735), 2, top=3),
        box((-.585, -.32, .735), (-.38, .4, 1.13), 0),
        box((.38, -.32, .735), (.585, .4, 1.13), 0, right=7),
        box((-.38, .31, .735), (.38, .4, 1.13), 15),
        box((-.38, -.32, .735), (.38, .31, .755), 15),
        # One shallow sample landing tray with a visibly separate lip.
        box((-.29, -.34, .755), (.29, .19, .775), 1),
        box((-.29, -.34, .775), (-.275, .19, .805), 1),
        box((.275, -.34, .775), (.29, .19, .805), 1),
        box((-.275, .175, .775), (.275, .19, .805), 1),
        dict(type="prism", plane="YZ",
             polygon=[[-.32, 1.13], [.4, 1.13], [.4, 1.5], [-.16, 1.5]],
             depth=[-.585, .585], tile=0,
             face_tiles={"front": 6, "right": 7}),
    ]
    analyzer_collision = solid_collision(analyzer)
    # These two simple boxes sit inside the wedge and leave the intake clear.
    analyzer_collision += [
        [[-.585, -.16, 1.13], [.585, .4, 1.5]],
        [[-.585, -.26, 1.13], [.585, -.16, 1.25]],
    ]

    sink = [
        box((-.78, -.33, 0), (.78, .33, .075), 1),
        box((-.775, -.33, .075), (.775, .33, .61), 0, front=4),
        # Separate cabinet front/ends enclose the under-counter basin.
        box((-.775, -.33, .61), (.775, -.30, .86), 0, front=4),
        box((-.775, -.30, .61), (-.75, .33, .86), 0),
        box((.75, -.30, .61), (.775, .33, .86), 0),
        box((-.75, .305, .61), (.75, .33, .86), 0),
        # Four counter strips preserve the rectangular .55 x .42 opening.
        box((-.8, -.35, .86), (-.63, .35, .9), 3),
        box((-.08, -.35, .86), (.8, .35, .9), 3),
        box((-.63, -.35, .86), (-.08, -.22, .9), 3),
        box((-.63, .2, .86), (-.08, .35, .9), 3),
        # Basin floor and walls, open above; floor top is .65 m.
        box((-.63, -.22, .625), (-.08, .2, .65), 15),
        box((-.645, -.235, .65), (-.63, .215, .86), 3),
        box((-.08, -.235, .65), (-.065, .215, .86), 3),
        box((-.63, -.235, .65), (-.08, -.22, .86), 3),
        box((-.63, .2, .65), (-.08, .215, .86), 3),
        # Low-poly square-neck faucet, inside the countertop footprint.
        box((-.38, .235, .9), (-.34, .275, 1.2), 3),
        box((-.38, .045, 1.16), (-.34, .235, 1.2), 3),
        box((-.38, .045, 1.105), (-.34, .085, 1.16), 3),
        cylinder((-.46, .255, .925), .024, .05, 2),
        cylinder((-.26, .255, .925), .024, .05, 2),
    ]

    sample = [
        box((-.225, -.15, 0), (.225, .15, .012), 3),
        box((-.225, -.15, .012), (-.213, .15, .06), 3),
        box((.213, -.15, .012), (.225, .15, .06), 3),
        box((-.213, -.15, .012), (.213, -.138, .06), 3),
        box((-.213, .138, .012), (.213, .15, .06), 3),
    ]
    # Six detached, opaque pale bottles. Facets and stepped necks read at top-down
    # camera distances without glass sorting, refraction or hidden internals.
    bottles = [(-.14, -.073, .035, .12), (0, -.073, .031, .095),
               (.14, -.073, .033, .135), (-.14, .073, .04, .165),
               (0, .073, .04, .185), (.14, .073, .038, .155)]
    for x, y, radius, height in bottles:
        sample.append(octagonal_body(x, y, radius, .012, height))
        sample.append(cylinder((x, y, height + .007), radius * .66, .014, 0))
        sample.append(cylinder((x, y, height + .024), radius * .70, .02, 2))
    sample_collision = [[[ -.225, -.15, 0], [.225, .15, .06]]]
    for x, y, radius, height in bottles:
        sample_collision.append([[x - radius, y - radius, .06],
                                 [x + radius, y + radius, height + .034]])

    return [
        dict(key="ControlBox", label="Wall control enclosure", dimensions=[.6, .2, .8],
             parts=control, collision=solid_collision(control),
             notes="Rear-bottom wall pivot Y=0. Gauge and warning details are front atlas graphics."),
        dict(key="Analyzer", label="Large laboratory analyzer", dimensions=[1.2, .8, 1.5],
             parts=analyzer, collision=analyzer_collision,
             notes="Real .76 m wide intake, .375 m high above landing deck; sloped screen console. No gameplay or internal mechanisms."),
        dict(key="Sink", label="Laboratory sink counter", dimensions=[1.6, .7, 1.2],
             parts=sink, collision=solid_collision(sink),
             notes="Counter Z=.90 matches Workbench; total height includes faucet. Basin opening .55 x .42 m; basin depth .25 m. No full collision hull across basin."),
        dict(key="SampleTray", label="Six sample bottles and tray", dimensions=[.45, .3, .219],
             parts=sample, collision=sample_collision,
             notes="Detached .45 x .30 tray for shelves/counters. Six opaque pale bottles with teal caps and front atlas labels save transparency passes; no liquid/interior geometry."),
    ]
