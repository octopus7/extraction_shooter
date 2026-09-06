"""UE 5.7 import/reload. Saves ONLY /Game/Environment/ModularInteriorExpansion."""
from pathlib import Path
import unreal,json,os,hashlib
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ModularInteriorExpansion'
DEST='/Game/Environment/ModularInteriorExpansion'
m=json.loads((OUT/'model_manifest.json').read_text())
verify=False
BASE='/Game/Environment/ModularInteriorPreview'
assets=unreal.AssetToolsHelpers.get_asset_tools();lib=unreal.MaterialEditingLibrary
editor=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)

def save(a):
    assert a.get_path_name().startswith(DEST+'/'),a.get_path_name()
    assert unreal.EditorAssetLibrary.save_loaded_asset(a,only_if_is_dirty=False)

def texture(name,srgb):
    if not verify:
        task=unreal.AssetImportTask();task.filename=str(OUT/'Textures'/f'{name}.png')
        task.destination_path=DEST+'/Textures';task.destination_name=name
        task.automated=True;task.replace_existing=True;task.save=True
        assets.import_asset_tasks([task])
    t=unreal.load_asset(f'{DEST}/Textures/{name}');assert t
    if not verify:
        t.set_editor_property('srgb',srgb)
        t.set_editor_property('compression_settings',unreal.TextureCompressionSettings.TC_DEFAULT if srgb else unreal.TextureCompressionSettings.TC_GRAYSCALE)
        t.set_editor_property('lod_group',unreal.TextureGroup.TEXTUREGROUP_WORLD)
        t.set_editor_property('never_stream',False);save(t)
    assert t.get_editor_property('srgb')==srgb
    return t

atlas=texture('T_MIE_ServiceAtlas',True)
mask=unreal.load_asset(BASE+'/Textures/T_MI_DirtMask');assert mask
materials={}
for name,s in {'M_MI_ExpansionSurface':{'tint':[.9,.95,1.],'roughness':.52,'metallic':.25}}.items():
    mat=unreal.load_asset(f'{DEST}/Materials/{name}')
    if not verify:
        if not mat:mat=assets.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
        lib.delete_all_material_expressions(mat)
        mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_OPAQUE)
        mat.set_editor_property('two_sided',False)
        def node(cls,**props):
            n=lib.create_material_expression(mat,getattr(unreal,'MaterialExpression'+cls),0,0)
            for k,v in props.items():n.set_editor_property(k,v)
            return n
        def link(a,out,b,inp):
            names=list(lib.get_material_expression_input_names(b))
            if inp not in names and len(names)==1:inp=names[0]
            assert lib.connect_material_expressions(a,out,b,inp),(b.get_class().get_name(),inp,names)
        def prop(a,out,p):assert lib.connect_material_property(a,out,getattr(unreal.MaterialProperty,'MP_'+p))
        def scalar(n,v,cpd=None):
            p=node('ScalarParameter',parameter_name=n,default_value=v)
            if cpd is not None:
                p.set_editor_property('use_custom_primitive_data',True);p.set_editor_property('primitive_data_index',cpd)
            return p
        def color(n,rgb):return node('VectorParameter',parameter_name=n,default_value=unreal.LinearColor(*rgb,1))
        def mul(a,ao,b,bo):
            n=node('Multiply');link(a,ao,n,'A');link(b,bo,n,'B');return n
        def lerp(a,ao,b,bo,t):
            n=node('LinearInterpolate');link(a,ao,n,'A');link(b,bo,n,'B');link(t,'',n,'Alpha');return n
        if name.endswith('_LED'):
            c=color('LEDColor',s['tint']);strength=scalar('Emission',5)
            prop(c,'RGB','BASE_COLOR');prop(mul(c,'RGB',strength,''),'','EMISSIVE_COLOR')
            prop(scalar('Roughness',.35),'','ROUGHNESS')
        else:
            tex=node('TextureSample',texture=atlas)
            tint=color('Tint',s['tint']);base=mul(tex,'RGB',tint,'RGB')
            if name.endswith('_Concrete'):
                pos=node('WorldPosition')
                z=node('ComponentMask',r=False,g=False,b=True,a=False);link(pos,'',z,'Input')
                # Step gives 1 when Y >= X. Band: z >=105 and 130 >= z (cm).
                low=node('Step');link(scalar('BandBottomCm',105),'',low,'X');link(z,'',low,'Y')
                high=node('Step');link(z,'',high,'X');link(scalar('BandTopCm',130),'',high,'Y')
                band=mul(low,'',high,'');bandcolor=color('BandColor',[1,1,1])
                banded=mul(base,'',bandcolor,'RGB');base=lerp(base,'',banded,'',band)
            uv=node('TextureCoordinate',coordinate_index=1)
            size=scalar('DirtScale',1,1);scaled=mul(uv,'',size,'')
            u=scalar('DirtOffsetU',0,2);v=scalar('DirtOffsetV',0,3)
            offset=node('AppendVector');link(u,'',offset,'A');link(v,'',offset,'B')
            coords=node('Add');link(scaled,'',coords,'A');link(offset,'',coords,'B')
            dirt=node('TextureSample',texture=mask,sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_GRAYSCALE)
            link(coords,'',dirt,'UVs')
            strength=scalar('DirtStrength',.65,0)
            cover=mul(dirt,'R',strength,'');clamp=node('Saturate');link(cover,'',clamp,'Input')
            dust=color('DirtColor',[.12,.105,.075])
            prop(lerp(base,'',dust,'RGB',clamp),'','BASE_COLOR')
            prop(lerp(scalar('Roughness',s['roughness']),'',scalar('DustRoughness',.94),'',clamp),'','ROUGHNESS')
            prop(lerp(scalar('Metallic',s['metallic']),'',scalar('DustMetallic',0),'',clamp),'','METALLIC')
        lib.layout_material_expressions(mat);lib.recompile_material(mat);save(mat)
    assert mat and mat.get_editor_property('blend_mode')==unreal.BlendMode.BLEND_OPAQUE
    assert not mat.get_editor_property('two_sided')
    assert lib.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
    materials[name]=mat

for suffix in ('Concrete','Steel','Floor','Door','LED'):
    materials['M_MI_'+suffix]=unreal.load_asset(BASE+'/Materials/M_MI_'+suffix)
materials['MI_MI_Concrete_Light']=unreal.load_asset(BASE+'/Materials/MI_MI_Concrete_Light')
for suffix,rgb,opacity,emission in [('Glass',[.15,.32,.36],.22,0),('Amber',[1,.19,.025],1,3)]:
    name='M_MI_Expansion'+suffix
    mat=unreal.load_asset(DEST+'/Materials/'+name) or assets.create_asset(name,DEST+'/Materials',unreal.Material,unreal.MaterialFactoryNew())
    lib.delete_all_material_expressions(mat)
    mat.set_editor_property('blend_mode',unreal.BlendMode.BLEND_TRANSLUCENT if suffix=='Glass' else unreal.BlendMode.BLEND_OPAQUE)
    mat.set_editor_property('two_sided',False)
    c=color('Color',rgb);prop(c,'RGB','BASE_COLOR')
    prop(scalar('Roughness',.22 if suffix=='Glass' else .35),'','ROUGHNESS')
    if suffix=='Glass':prop(scalar('Opacity',opacity),'','OPACITY')
    else:prop(mul(c,'RGB',scalar('Emission',emission),''),'','EMISSIVE_COLOR')
    lib.layout_material_expressions(mat);lib.recompile_material(mat);save(mat);materials[name]=mat
if not verify:
    for e in m['assets']:
        options=unreal.FbxImportUI()
        for k,v in {'import_mesh':True,'import_as_skeletal':False,'import_animations':False,'import_materials':False,
                    'import_textures':False,'automated_import_should_detect_type':False,'mesh_type_to_import':unreal.FBXImportType.FBXIT_STATIC_MESH}.items():options.set_editor_property(k,v)
        data=options.static_mesh_import_data
        for k,v in {'combine_meshes':True,'auto_generate_collision':False,'one_convex_hull_per_ucx':True,
                    'generate_lightmap_u_vs':True,'convert_scene':True,'convert_scene_unit':True,'force_front_x_axis':False,
                    'transform_vertex_to_absolute':True,'build_nanite':False,'remove_degenerates':True,'import_uniform_scale':1.}.items():data.set_editor_property(k,v)
        data.set_editor_property('normal_import_method',unreal.FBXNormalImportMethod.FBXNIM_IMPORT_NORMALS)
        task=unreal.AssetImportTask();task.filename=str(OUT/'Models'/f"{e['name']}.fbx")
        task.destination_path=DEST+'/Meshes';task.destination_name=e['name']
        task.automated=True;task.replace_existing=True;task.replace_existing_settings=True;task.save=True
        task.options=options;task.factory=unreal.FbxFactory();assets.import_asset_tasks([task])
        mesh=unreal.load_asset(f"{DEST}/Meshes/{e['name']}");assert mesh
        for i,slot in enumerate(mesh.get_editor_property('static_materials')):
            n=str(slot.get_editor_property('material_slot_name'))
            if n not in materials:n=str(slot.get_editor_property('imported_material_slot_name'))
            assert n in materials,n
            mesh.set_material(i,materials['MI_MI_Concrete_Light'] if n=='M_MI_Concrete' else materials[n])
        build=editor.get_lod_build_settings(mesh,0)
        # UV0 atlas, UV1 dirt, generated UV2 lightmap. Never overwrite dirt coordinates.
        for k,v in {'recompute_normals':False,'recompute_tangents':True,'generate_lightmap_u_vs':True,
                    'src_lightmap_index':0,'dst_lightmap_index':2}.items():build.set_editor_property(k,v)
        editor.set_lod_build_settings(mesh,0,build);mesh.set_editor_property('light_map_coordinate_index',2)
        # These authored hulls are all boxes. Store analytic box primitives in UE,
        # avoiding convex cook data and its commandlet/editor initialization dependency.
        body=mesh.get_editor_property('body_setup');agg=body.get_editor_property('agg_geom')
        boxes=[]
        for lo,hi in e['collision_boxes']:
            box=unreal.KBoxElem()
            box.set_editor_property('center',unreal.Vector(*[(a+b)*50 for a,b in zip(lo,hi)]))
            box.set_editor_property('rotation',unreal.Rotator())
            for axis,index in [('x',0),('y',1),('z',2)]:box.set_editor_property(axis,(hi[index]-lo[index])*100)
            boxes.append(box)
        agg.set_editor_property('convex_elems',[]);agg.set_editor_property('box_elems',boxes)
        body.set_editor_property('agg_geom',agg)
        body.set_editor_property('collision_trace_flag',unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
        save(mesh)

exec(compile((Path(__file__).with_name('verify_unreal_assets.py')).read_text(),str(Path(__file__).with_name('verify_unreal_assets.py')),'exec'))
report['mode']='import'
(OUT/'unreal_import_validation.json').write_text(json.dumps(report,indent=2))
