"""Hand-authored <1000-triangle variant; no decimation, textures or tiny bevels."""
def build(box, plate):
    plate('Impact guard lower', (0,0,.44), (10.9,6.9,.64), .75, 'Guard', .12)
    plate('Blue chassis and lid', (0,0,1.1), (10.6,6.6,1.42), .65, 'Shell', .11)
    # Thin stacked plates have crisp edges; bevel only silhouette-bearing solids.
    plate('Inset border', (-.55,0,1.82), (7.2,4.65,.07), .6, 'Metal', 0)
    plate('Data panel', (-.55,0,1.86), (6.92,4.37,.07), .53, 'Shell', 0)
    for x in [-4.65,4.65]:
        for y in [-2.65,2.65]:
            plate('Corner bumper', (x,y,1.06), (1.7,1.7,1.96), .43, 'Guard', .1)
            box('Armor inlay', (x,y,2.05), (.72,.76,.041), 'Metal', 0)
    plate('Connector root', (5.18,0,1), (1,3.85,1.16), .2, 'Guard', 0)
    box('Connector top', (5.72,0,1.46), (1.56,3.25,.2), 'Metal', 0)
    box('Connector bottom', (5.72,0,.61), (1.56,3.25,.2), 'Metal', 0)
    for y in [-1.51,1.51]:
        box('Connector side', (5.72,y,1.035), (1.56,.23,.65), 'Metal', 0)
    box('Connector cavity', (5.26,0,1.035), (.12,2.83,.72), 'Guard', 0)
    box('Contact tongue', (5.82,0,.89), (1.14,2.6,.21), 'Guard', 0)
    for y in [-.8,0,.8]:
        box('Contact', (5.99,y,1.007), (.85,.32,.04), 'Metal', 0)
    plate('Indicator bezel', (3.49,0,1.831), (.85,2.63,.11), .2, 'Guard', 0)
    plate('Status lens', (3.49,0,1.907), (.4,1.92,.065), .13, 'Status', 0)
    for i in range(3):
        box('Data stack glyph', (-.76+i*.44,0,1.917), (.18,1.2,.05), 'Metal', 0)
    plate('Underside plate', (0,0,.09), (7.7,4.7,.11), .45, 'Shell', 0)
    for y in [-1.52,1.52]:
        box('Underside skid', (0,y,.025), (5.7,.39,.05), 'Guard', 0)
