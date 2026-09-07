"""Read/export six existing Nature examples in a disposable UE audit project."""
import unreal, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushSparse/References'
OUT.mkdir(parents=True,exist_ok=True)
examples=[('Bush','Bush_Combined','Bush_Cluster_Base_Color'),('GrassLow','SM_GrassLow','M_GrassLowBlade'),('Flower','SM_Flower','T_Flower'),('SimpleTree','SM_SimpleTree','T_SimpleTree'),('Wood','SM_LogA','T_WoodCommon'),('RockBasic','RockS_1','T_Rock_Basic')]
ed=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
report=[]
for folder,name,texture in examples:
    mesh=unreal.load_asset('/Game/Nature/'+folder+'/'+name)
    tex=unreal.load_asset('/Game/Nature/'+folder+'/'+texture)
    if not isinstance(tex,unreal.Texture2D):
        mat=tex
        node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
        tex=node.get_editor_property('texture') if isinstance(node,unreal.MaterialExpressionTextureSample) else None
    for obj,ext,exporter in [(mesh,'.fbx',unreal.StaticMeshExporterFBX()),(tex,'.tga',unreal.TextureExporterTGA())]:
        if obj:
            task=unreal.AssetExportTask();task.object=obj;task.filename=str(OUT/(folder+ext));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=exporter
            if ext=='.fbx':
                task.options=unreal.FbxExportOption();task.options.set_editor_property('collision',False);task.options.set_editor_property('level_of_detail',False)
            assert unreal.Exporter.run_asset_export_task(task),task.filename
    b=mesh.get_bounds()
    report.append({'category':folder,'mesh':mesh.get_path_name(),'texture':tex.get_path_name() if tex else None,'dimensions_cm':[2*b.box_extent.x,2*b.box_extent.y,2*b.box_extent.z],'vertices':ed.get_number_verts(mesh,0),'uv_channels':ed.get_num_uv_channels(mesh,0),'materials':[{'path':s.material_interface.get_path_name(),'two_sided':s.material_interface.get_editor_property('two_sided'),'blend_mode':str(s.material_interface.get_editor_property('blend_mode'))} for s in mesh.static_materials]})
(OUT/'existing_asset_audit.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('BUSH_SPARSE_REFERENCE_EXPORT_PASSED')
