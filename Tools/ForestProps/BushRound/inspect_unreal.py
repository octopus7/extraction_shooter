import unreal, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/BushRound'
report={}
ed=unreal.get_default_object(unreal.StaticMeshEditorSubsystem)
for group in ['Bush','GrassLow','Flower','SimpleTree','Wood','RockBasic']:
    rows=[]
    for path in unreal.EditorAssetLibrary.list_assets('/Game/Nature/'+group,recursive=True):
        obj=unreal.load_asset(path)
        row={'path':path,'class':obj.get_class().get_name()}
        if isinstance(obj,unreal.StaticMesh):
            b=obj.get_bounds()
            row.update(size_cm=[b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2],vertices=ed.get_number_verts(obj,0),materials=[str(x.material_interface) for x in obj.static_materials])
            if group=='Bush':
                task=unreal.AssetExportTask();task.object=obj;task.filename=str(OUT/'Bush_Reference.fbx');task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.StaticMeshExporterFBX()
                assert unreal.Exporter.run_asset_export_task(task)
        if isinstance(obj,unreal.Texture2D) and group=='Bush':
            task=unreal.AssetExportTask();task.object=obj;task.filename=str(OUT/'Bush_Reference.tga');task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.TextureExporterTGA()
            assert unreal.Exporter.run_asset_export_task(task)
        rows.append(row)
    report[group]=rows
(OUT/'existing_assets.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
