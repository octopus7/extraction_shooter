"""Paint UVs for the door kit. Coordinates in the bake contract are centimetres."""
from collections import defaultdict
import math


SIZE = 2048
GUTTER = 8
# Signed orthogonal U, V, depth axes; U cross V points out of the surface.
BASES = {
    'front': (1, 3, -2), 'back': (-1, 3, 2),
    'right': (2, 3, 1), 'left': (-2, 3, -1),
    'top': (1, 2, 3), 'bottom': (1, -2, -3),
}


def component(vector, axis):
    return vector[abs(axis) - 1] * (1 if axis > 0 else -1)


def page_for(asset, kind):
    if 'Temporary' in asset or kind in ('LED', 'Signal'):
        return None
    return 'Frame' if kind == 'Concrete' else 'Metal'


def make_charts(asset, obj):
    """Split at real part seams and changes of dominant plane, including pocket returns."""
    mesh = obj.data
    mesh.calc_loop_triangles()
    world = obj.matrix_world
    coords = [tuple(100.0 * a for a in (world @ v.co)) for v in mesh.vertices]
    pmin = [min(v[i] for v in coords) for i in range(3)]
    pmax = [max(v[i] for v in coords) for i in range(3)]
    axes = {}
    edge_faces = defaultdict(list)
    for face in mesh.polygons:
        normal = world.to_3x3() @ face.normal
        axes[face.index] = max(BASES, key=lambda key: component(normal, BASES[key][2]))
        for edge in face.edge_keys:
            edge_faces[edge].append(face.index)
    adjacent = defaultdict(list)
    for edge in mesh.edges:
        neighbours = edge_faces[edge.key]
        if len(neighbours) == 2 and axes[neighbours[0]] == axes[neighbours[1]]:
            a, b = neighbours
            adjacent[a].append(b)
            adjacent[b].append(a)
        else:
            edge.use_seam = True
    uv = mesh.uv_layers.active or mesh.uv_layers.new(name='PaintUV')
    uv.name = 'PaintUV'
    unseen = set(axes)
    charts = []
    while unseen:
        first = min(unseen)
        axis = axes[first]
        pending = [first]
        unseen.remove(first)
        faces = []
        while pending:
            face = pending.pop()
            faces.append(face)
            for neighbour in adjacent[face]:
                if neighbour in unseen:
                    unseen.remove(neighbour)
                    pending.append(neighbour)
        u, v, d = BASES[axis]
        loops = {loop: coords[mesh.loops[loop].vertex_index]
                 for face in faces for loop in mesh.polygons[face].loop_indices}
        points = {i: (component(p, u), component(p, v)) for i, p in loops.items()}
        lo = [min(p[i] for p in points.values()) for i in range(2)]
        hi = [max(p[i] for p in points.values()) for i in range(2)]
        chart = dict(asset=asset, part=obj['garage_part'], kind=obj['garage_surface'],
                     axis=axis, basis=BASES[axis], lo=lo, hi=hi, pmin=pmin, pmax=pmax,
                     depth=sum(component(p, d) for p in loops.values()) / len(loops),
                     page=page_for(asset, obj['garage_surface']))
        charts.append((chart, uv, points, frozenset(faces), mesh))
    return charts


def density(c):
    axis, part = c['axis'], c['part']
    if c['page'] == 'Frame':
        return {'front': 7.35, 'top': 6.25, 'back': 3.65, 'bottom': 3.0}.get(axis, 5.5)
    if part == 'Inset_plate':
        return 1.05 if axis == 'back' else 3.6
    if part == 'Panel_structural_skin':
        return 4.4 if axis == 'back' else 3.0
    if 'Bolt' in part or 'gasket' in part.lower():
        return 3.5
    return 4.8 if axis == 'front' else 3.2


def rect_size(c, scale):
    return tuple(math.ceil((hi - lo) * scale) + 2 * GUTTER
                 for lo, hi in zip(c['lo'], c['hi']))


def panel_index(name):
    return {'UpperPanel': 0, 'UpperPanel02': 1, 'UpperPanel03': 2, 'UpperPanel04': 3}[name]


def pack(charts, page, secondary_scale):
    # The twelve principal painted faces read like the closed door in the UV editor.
    remaining = list(range(len(charts)))
    placements = {}
    free = [(0, 0, SIZE, SIZE)]
    if page == 'Metal':
        hero = [i for i, c in enumerate(charts) if c['part'] == 'Inset_plate'
                and c['axis'] == 'front' and c['hi'][0] - c['lo'][0] > 50]
        assert len(hero) == 12
        hero.sort(key=lambda i: (panel_index(charts[i]['asset']), charts[i]['lo'][0]))
        scale = 10.8
        sizes = [rect_size(charts[i], scale) for i in hero]
        width, height = map(max, zip(*sizes))
        free = [(0, height * 4, SIZE, SIZE - height * 4),
                (width * 3, 0, SIZE - width * 3, height * 4)]
        for order, i in enumerate(hero):
            placements[i] = (width * (order % 3), height * (order // 3), scale, False)
            remaining.remove(i)
    dims = {i: rect_size(charts[i], density(charts[i]) * secondary_scale) for i in remaining}
    remaining.sort(key=lambda i: (max(dims[i]), min(dims[i])), reverse=True)
    for i in remaining:
        w, h = dims[i]
        choices = []
        for j, (_, _, fw, fh) in enumerate(free):
            for rot in (False, True):
                rw, rh = (h, w) if rot else (w, h)
                if rw <= fw and rh <= fh:
                    choices.append(((min(fw-rw, fh-rh), fw*fh-rw*rh, rot), j, rw, rh, rot))
        if not choices:
            return None
        _, j, rw, rh, rot = min(choices)
        x, y, fw, fh = free.pop(j)
        placements[i] = (x, y, density(charts[i]) * secondary_scale, rot)
        # Disjoint guillotine rectangles keep every reserved gutter isolated.
        if fw-rw > fh-rh:
            free.extend(((x+rw, y, fw-rw, fh), (x, y+rh, rw, fh-rh)))
        else:
            free.extend(((x+rw, y, fw-rw, rh), (x, y+rh, fw, fh-rh)))
    return placements


def interiors_overlap(a, b):
    # Separating axes permit triangles to share an edge, never positive area.
    for tri in (a, b):
        for p, q in zip(tri, tri[1:] + tri[:1]):
            nx, ny = p[1]-q[1], q[0]-p[0]
            tol = 1e-6 * math.hypot(nx, ny)
            pa = [nx*x + ny*y for x, y in a]
            pb = [nx*x + ny*y for x, y in b]
            if max(pa) <= min(pb) + tol or max(pb) <= min(pa) + tol:
                return False
    return True


def assign(groups):
    pages = defaultdict(list)
    for name, parts in groups.items():
        asset = name.removeprefix('SM_GarageConcrete_')
        for part in parts:
            page = page_for(asset, part['garage_surface'])
            if page:
                pages[page].extend(make_charts(asset, part))
    bake, stats = [], {}
    for page, items in pages.items():
        charts = [item[0] for item in items]
        for step in range(110, 34, -2):
            placements = pack(charts, page, step / 100)
            if placements:
                break
        assert placements, 'Cannot fit ' + page
        total_area = 0
        front_density = []
        for i, (c, uv, points, faces, mesh) in enumerate(items):
            x, y, scale, rot = placements[i]
            w, h = rect_size(c, scale)
            c.update(x=x, y=y, w=w, h=h, scale=scale, rotated=rot)
            outlines = set()
            pixel = {}
            for loop, (a, b) in points.items():
                px = GUTTER + (a-c['lo'][0])*scale
                py = GUTTER + (c['hi'][1]-b)*scale
                if rot:
                    px, py = h-py, px
                px += x
                py += y
                pixel[loop] = (px, py)
                uv.data[loop].uv = (px/SIZE, 1-py/SIZE)
                assert 0 < px < SIZE and 0 < py < SIZE
            triangles = []
            for tri in mesh.loop_triangles:
                if tri.polygon_index not in faces:
                    continue
                a, b, d = triangle = tuple(pixel[loop] for loop in tri.loops)
                area = abs((b[0]-a[0])*(d[1]-a[1])-(b[1]-a[1])*(d[0]-a[0])) / 2
                assert area > 1e-10, ('Collapsed paint UV', c['asset'], c['part'], c['axis'], area)
                for prev in triangles:
                    assert not interiors_overlap(triangle, prev), ('Folded paint UV', c['asset'], c['part'], c['axis'], triangle, prev, tri.polygon_index)
                triangles.append(triangle)
                total_area += area
                for a, b in zip(triangle, triangle[1:]+triangle[:1]):
                    edge = tuple(sorted((a, b)))
                    if edge in outlines:
                        outlines.remove(edge)
                    else:
                        outlines.add(edge)
            c['outline'] = sorted(outlines)
            if c['axis'] == 'front' and max(c['hi'][j]-c['lo'][j] for j in range(2)) > 50:
                if page == 'Frame' or c['part'] == 'Inset_plate':
                    front_density.append(scale * 100)
            bake.append(c)
        stats[page] = {'resolution': [SIZE, SIZE], 'islands': len(items),
                       'edge_padding_px': GUTTER, 'min_island_spacing_px': 2*GUTTER,
                       'painted_triangle_coverage_pct': round(100*total_area/SIZE**2, 2),
                       'main_front_px_per_m': round(min(front_density)),
                       'overlapping_triangles': 0, 'collapsed_triangles': 0}
    return bake, stats
