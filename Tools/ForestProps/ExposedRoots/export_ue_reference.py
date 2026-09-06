import unreal, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/ExposedRoots'
paths=['Bush/Bush_Combined','GrassLow/SM_GrassLow','Flower/SM_Flower','SimpleTree/SM_SimpleTree','Wood/SM_StumpA','RockBasic/RockM_1']
sub=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
report=[]
for path in paths:
    mesh=unreal.load_asset('/Game/Nature/'+path)
    assert mesh,path
    b=mesh.get_bounds()
    report.append({'asset':mesh.get_path_name(),'extent_cm':[b.box_extent.x,b.box_extent.y,b.box_extent.z],'vertices':sub.get_number_verts(mesh,0),'materials':[str(m.material_interface.get_path_name()) for m in mesh.static_materials]})
    if path.startswith('SimpleTree/'):
        texture=unreal.load_asset('/Game/Nature/SimpleTree/T_SimpleTree')
        task=unreal.AssetExportTask();task.object=texture;task.filename=str(OUT/'Reference_Tree.tga');task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.TextureExporterTGA()
        assert unreal.Exporter.run_asset_export_task(task)
    if path.startswith('Bush/'):
        task=unreal.AssetExportTask();task.object=mesh;task.filename=str(OUT/'Reference_Bush.fbx');task.automated=True;task.prompt=False;task.replace_identical=True
        task.exporter=unreal.StaticMeshExporterFBX();task.options=unreal.FbxExportOption()
        assert unreal.Exporter.run_asset_export_task(task)
        texture=unreal.load_asset('/Game/Nature/Bush/Bush_Cluster_Base_Color')
        task=unreal.AssetExportTask();task.object=texture;task.filename=str(OUT/'Reference_Bush.tga');task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.TextureExporterTGA()
        assert unreal.Exporter.run_asset_export_task(task)
(OUT/'ue_reference_inventory.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
