import unreal, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/GrassSparse/Reference'
report=[]
for folder in ['Bush','GrassLow','Flower','SimpleTree','Wood','RockBasic']:
    for path in unreal.EditorAssetLibrary.list_assets('/Game/Nature/'+folder):
        asset=unreal.load_asset(path)
        entry={'path':path,'class':asset.get_class().get_name()}
        if isinstance(asset,unreal.StaticMesh):
            b=asset.get_bounds();entry['size_cm']=[b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2]
            entry['materials']=[s.material_interface.get_path_name() if s.material_interface else None for s in asset.static_materials]
        report.append(entry)
        if folder=='Bush' and isinstance(asset,unreal.Texture2D):
            t=unreal.AssetExportTask();t.object=asset;t.filename=str(OUT/'Bush_BaseColor.tga');t.automated=True;t.prompt=False;t.exporter=unreal.TextureExporterTGA();unreal.Exporter.run_asset_export_task(t)
        if folder=='Bush' and isinstance(asset,unreal.StaticMesh):
            t=unreal.AssetExportTask();t.object=asset;t.filename=str(OUT/'Bush_reference.fbx');t.automated=True;t.prompt=False;t.exporter=unreal.StaticMeshExporterFBX();unreal.Exporter.run_asset_export_task(t)
(OUT/'ue_existing_assets.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
