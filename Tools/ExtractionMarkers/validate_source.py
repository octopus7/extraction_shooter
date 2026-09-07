"""Fresh-process, read-only Blender/FBX audit and actual reload proof render."""
import bpy, bmesh, json, math, hashlib, traceback
from collections import defaultdict
from pathlib import Path
from mathutils import Matrix

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/ExtractionMarkers'
REF = ROOT / 'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion/Models'
report = {'passed': False, 'blender_version': bpy.app.version_string, 'assets': [], 'errors': []}


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def rounded(v):
    return tuple(round(float(x), 5) for x in v)


def triangle_signature(obj):
    obj.data.calc_loop_triangles()
    return sorted(tuple(sorted(rounded(obj.data.vertices[i].co) for i in tri.vertices))
                  for tri in obj.data.loop_triangles)


def attribute_signature(obj):
    obj.data.calc_loop_triangles()
    return sorted((tri.material_index, tuple(sorted(
        (rounded(obj.data.vertices[obj.data.loops[li].vertex_index].co),
         tuple(rounded(layer.data[li].uv) for layer in obj.data.uv_layers))
        for li in tri.loops))) for tri in obj.data.loop_triangles)


def audit(obj, entry, label):
    require(obj.location.length < 1e-6, f'{label}: nonzero pivot')
    require(obj.rotation_euler.to_matrix().is_identity, f'{label}: nonidentity rotation')
    require(max(abs(v-1) for v in obj.scale) < 1e-6, f'{label}: nonidentity scale')
    mesh = obj.data
    mesh.calc_loop_triangles()
    require(len(mesh.loop_triangles) == entry['triangles'], f'{label}: triangle count')
    require(len(mesh.materials) == entry['material_slots'], f'{label}: slot count')
    require(all(m for m in mesh.materials), f'{label}: missing material')
    require([m.name.split('.')[0] for m in mesh.materials]==entry['materials'], f'{label}: material slot order/name mismatch')
    require(len(mesh.uv_layers) == 2, f'{label}: expected exactly two authored UV channels')
    bounds = [min(v.co[i] for v in mesh.vertices) for i in range(3)] + [max(v.co[i] for v in mesh.vertices) for i in range(3)]
    bounds_error = max(abs(a-b) for a,b in zip(bounds,entry['bounds_m']))
    require(bounds_error < 1e-5, f'{label}: signed bounds {bounds}')
    require(all(math.isfinite(c) for v in mesh.vertices for c in v.co), f'{label}: nonfinite coordinates')
    min_area = min(t.area for t in mesh.loop_triangles)
    require(min_area > 1e-10, f'{label}: degenerate face')
    for p in mesh.polygons:
        require(abs(p.normal.length-1) < 1e-5, f'{label}: invalid normal')
        require(0 <= p.material_index < len(mesh.materials), f'{label}: invalid material index')
    uv_mins = []
    for layer in mesh.uv_layers:
        require(all(math.isfinite(v) for d in layer.data for v in d.uv), f'{label}: nonfinite UV')
        areas=[]
        for t in mesh.loop_triangles:
            a,b,c = [layer.data[i].uv for i in t.loops]
            areas.append(abs((b.x-a.x)*(c.y-a.y)-(b.y-a.y)*(c.x-a.x))*.5)
        uv_mins.append(min(areas))
        require(min(areas)>1e-12, f'{label}: degenerate UV in {layer.name}: {min(areas)}')
    # Preserve independently closed touching pieces. Only repair import seam splits
    # if topology is open; globally welding closed touching shells creates false
    # nonmanifold interfaces at the deliberately stacked beacon cylinders.
    bm=bmesh.new();bm.from_mesh(mesh)
    if not all(e.is_manifold for e in bm.edges):
        bmesh.ops.remove_doubles(bm,verts=list(bm.verts),dist=1e-7)
    require(all(e.is_manifold for e in bm.edges), f'{label}: open/nonmanifold edges')
    require(all(e.is_contiguous for e in bm.edges), f'{label}: inconsistent winding')
    pending=set(bm.faces);volumes=[]
    while pending:
        start=pending.pop();component={start};todo=[start]
        while todo:
            for edge in todo.pop().edges:
                for f in edge.link_faces:
                    if f in pending: pending.remove(f);component.add(f);todo.append(f)
        volume=0
        for f in component:
            verts=[v.co for v in f.verts]
            for i in range(1,len(verts)-1): volume+=verts[0].dot(verts[i].cross(verts[i+1]))/6
        require(volume>1e-10, f'{label}: inward/zero-volume connected shell {volume}')
        volumes.append(volume)
    bm.free()
    arrow_faces=0
    if entry['name']=='SM_EM_DirectionSign':
        for p in mesh.polygons:
            if p.normal.z>.9 and p.area>.04 and p.center.z>.075:
                arrow_faces+=1
                loops=list(p.loop_indices)
                for i in range(len(loops)):
                    for j in range(i+1,len(loops)):
                        l1,l2=loops[i],loops[j]
                        x1=mesh.vertices[mesh.loops[l1].vertex_index].co.x
                        x2=mesh.vertices[mesh.loops[l2].vertex_index].co.x
                        v1=mesh.uv_layers[0].data[l1].uv.y
                        v2=mesh.uv_layers[0].data[l2].uv.y
                        if abs(x2-x1)>1e-5: require((x2-x1)*(v2-v1)>0, f'{label}: arrow UV +V is not +X')
        require(arrow_faces==2, f'{label}: expected two broad arrow triangles, got {arrow_faces}')
    return {'label':label,'triangles':len(mesh.loop_triangles),'material_slots':len(mesh.materials),
            'bounds_m':bounds,'bounds_error_m':bounds_error,'uv_channels':len(mesh.uv_layers),
            'minimum_triangle_area_m2':min_area,'minimum_uv_triangle_areas':uv_mins,
            'closed_shell_count':len(volumes),'positive_shell_volumes_m3':volumes,
            'normals_and_winding_valid':True,'arrow_faces_pointing_plus_x':arrow_faces}


try:
    manifest=json.loads((OUT/'model_manifest.json').read_text())
    bpy.ops.wm.open_mainfile(filepath=str(OUT/'ExtractionMarkers.blend'))
    source={e['name']:bpy.data.objects[e['name']] for e in manifest['assets']}
    signatures={name:triangle_signature(obj) for name,obj in source.items()}
    attributes={name:attribute_signature(obj) for name,obj in source.items()}
    materials={name:list(obj.data.materials) for name,obj in source.items()}
    for e in manifest['assets']:
        report['assets'].append(audit(source[e['name']],e,'source:'+e['name']))
    for e in manifest['assets']:
        name=e['name']
        path=OUT/'Models'/(name+'.fbx')
        if e['reused_ue']:
            expected=REF/(name+'.fbx')
            require(path.read_bytes()==expected.read_bytes(), 'Reused emergency-light FBX differs from original')
            report['original_emergency_light_sha256']=hashlib.sha256(path.read_bytes()).hexdigest()
        before=set(bpy.data.objects)
        bpy.ops.import_scene.fbx(filepath=str(path),use_custom_normals=True)
        added=set(bpy.data.objects)-before
        meshes=[o for o in added if o.type=='MESH' and not o.name.startswith('UCX')]
        require(len(meshes)==1,f'{name}: FBX must contain one render mesh')
        obj=meshes[0]
        bpy.ops.object.select_all(action='DESELECT');obj.select_set(True);bpy.context.view_layer.objects.active=obj
        bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
        obj.data.transform(Matrix.Diagonal((1,-1,1,1)))
        bm=bmesh.new();bm.from_mesh(obj.data);bmesh.ops.reverse_faces(bm,faces=list(bm.faces));bm.to_mesh(obj.data);bm.free();obj.data.update()
        result=audit(obj,e,'fbx:'+name)
        require(triangle_signature(obj)==signatures[name],f'{name}: FBX position topology differs from source')
        require(attribute_signature(obj)==attributes[name],f'{name}: FBX UV/material-index assignment differs from source')
        result['triangle_position_signature_matches_source']=True
        result['triangle_uv_and_material_signature_matches_source']=True
        report['assets'].append(result)
        old_data=source[name].data
        imported_indices=[p.material_index for p in obj.data.polygons]
        obj.data.materials.clear()
        for mat in materials[name]:obj.data.materials.append(mat)
        for p,index in zip(obj.data.polygons,imported_indices):p.material_index=index
        # Keep imported per-face material indices; material objects are the saved
        # source shaders so the proof isolates FBX geometry/UV/slot assignment.
        for existing in list(bpy.data.objects):
            if existing not in added and existing.type=='MESH' and existing.data==old_data:existing.data=obj.data
        for new in added:bpy.data.objects.remove(new,do_unlink=True)
    scene=bpy.context.scene
    scene.camera=bpy.data.objects['TrueTopDown']
    if scene.use_nodes and scene.node_tree.nodes.get('ProjectAxisFlip'):
        scene.node_tree.nodes['ProjectAxisFlip'].mute=False
    scene.render.filepath=str(OUT/'Previews/FBX_Reload.png')
    bpy.ops.render.render(write_still=True)
    report['proof_render']='Previews/FBX_Reload.png'
    report['proof_render_uses_reloaded_fbx_geometry_uvs_material_indices']=True
    report['unique_triangles']=manifest['unique_triangles']
    report['passed']=True
except Exception:
    report['errors'].append(traceback.format_exc())
    raise
finally:
    (OUT/'source_validation.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
print('EM_SOURCE_AND_FBX_VALIDATION_PASSED')
