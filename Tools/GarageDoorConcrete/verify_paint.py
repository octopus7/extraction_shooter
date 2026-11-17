"""Read UV0 from the saved Unreal source meshes and check both shared texture pages."""
from collections import defaultdict
import math
import unreal

from surface_uv import SIZE, interiors_overlap


def audit(destination, manifest):
    pages = {page: {'triangles': [], 'bins': defaultdict(list), 'area': 0.0}
             for page in manifest['surface_materials']}
    names = {name: page for page, name in manifest['surface_materials'].items()}
    queries = unreal.GeometryScript_MeshQueries
    lod = unreal.GeometryScriptMeshReadLOD(lod_type=unreal.GeometryScriptLODType.SOURCE_MODEL,
                                            lod_index=0)
    opts = unreal.GeometryScriptCopyMeshFromAssetOptions()
    opts.set_editor_property('apply_build_settings', False)
    for entry in manifest['assets']:
        if not any(name in names for name in entry['materials']):
            continue
        mesh = unreal.load_asset(f"{destination}/Meshes/{entry['name']}")
        copies = unreal.GeometryScript_AssetUtils.copy_mesh_from_static_mesh(
            mesh, unreal.DynamicMesh(), opts, lod, None)
        geometry = next(v for v in copies if isinstance(v, unreal.DynamicMesh))
        for index in range(queries.get_num_triangle_i_ds(geometry)):
            if not queries.is_valid_triangle_id(geometry, index):
                continue
            material_id, valid = unreal.GeometryScript_Materials.get_triangle_material_id(geometry, index)
            assert valid
            page_name = names.get(mesh.get_material(material_id).get_name())
            if not page_name:  # The LED diffuser and shared red/green lenses have procedural emission.
                continue
            page = pages[page_name]
            uv = queries.get_triangle_u_vs(geometry, 0, index)
            tri = tuple((p.x*SIZE, p.y*SIZE) for p in uv if isinstance(p, unreal.Vector2D))
            assert len(tri) == 3 and all(0 < n < SIZE for p in tri for n in p)
            a, b, c = tri
            area = abs((b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]))/2
            assert area > 1e-10, ('Collapsed saved UV', entry['name'], index)
            xs, ys = zip(*tri)
            box = (min(xs), min(ys), max(xs), max(ys))
            cells = [(x, y) for x in range(math.floor(box[0]/32), math.floor(box[2]/32)+1)
                     for y in range(math.floor(box[1]/32), math.floor(box[3]/32)+1)]
            candidates = {i for cell in cells for i in page['bins'][cell]}
            for other in candidates:
                previous, bounds = page['triangles'][other]
                if box[0] >= bounds[2] or bounds[0] >= box[2] or box[1] >= bounds[3] or bounds[1] >= box[3]:
                    continue
                assert not interiors_overlap(tri, previous), ('Overlapping saved UV', entry['name'], index, tri, previous)
            for cell in cells:
                page['bins'][cell].append(len(page['triangles']))
            page['triangles'].append((tri, box))
            page['area'] += area
    result = {}
    for name, page in pages.items():
        coverage = round(100*page['area']/SIZE**2, 2)
        assert abs(coverage-manifest['paint_uv'][name]['painted_triangle_coverage_pct']) < .1
        result[name] = {'tested_triangles': len(page['triangles']), 'coverage_pct': coverage,
                        'degenerate_triangles': 0, 'overlaps': 0}
    return result
