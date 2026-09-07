import unreal, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassLongCurved/References'
TEMP=ROOT/'TunaSweeper/Saved/GrassLongCurved'
TEMP.mkdir(parents=True,exist_ok=True); OUT.mkdir(parents=True,exist_ok=True)
ed=unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem) or unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
report=[]
for family in ['Bush','GrassLow','Flower','SimpleTree','Wood','RockBasic']:
    for path in unreal.EditorAssetLibrary.list_assets('/Game/Nature/'+family,recursive=True,include_folder=False):
        obj=unreal.load_asset(path)
        if isinstance(obj,unreal.StaticMesh):
            b=obj.get_bounds()
            report.append({'path':path,'size_cm':[2*b.box_extent.x,2*b.box_extent.y,2*b.box_extent.z],'vertices':ed.get_number_verts(obj,0),'uv_channels':ed.get_num_uv_channels(obj,0),'materials':[s.material_interface.get_path_name() if s.material_interface else None for s in obj.static_materials]})
            if family=='Bush':
                t=unreal.AssetExportTask();t.object=obj;t.filename=str(TEMP/'Bush_Combined.fbx');t.automated=True;t.prompt=False;t.replace_identical=True;t.exporter=unreal.StaticMeshExporterFBX()
                assert unreal.Exporter.run_asset_export_task(t)
        if family=='Bush' and isinstance(obj,unreal.Texture2D):
            t=unreal.AssetExportTask();t.object=obj;t.filename=str(TEMP/'Bush_BaseColor.tga');t.automated=True;t.prompt=False;t.replace_identical=True;t.exporter=unreal.TextureExporterTGA()
            assert unreal.Exporter.run_asset_export_task(t)
(OUT/'unreal_reference_inventory.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
unreal.log('GRASS_REFERENCES_DONE')
