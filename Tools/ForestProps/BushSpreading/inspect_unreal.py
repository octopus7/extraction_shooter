from pathlib import Path
import unreal,json
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/ForestProps/BushSpreading'
paths=unreal.EditorAssetLibrary.list_assets('/Game/Nature',recursive=True)
report=[]
for path in paths:
    asset=unreal.load_asset(path)
    if isinstance(asset,unreal.StaticMesh):
        b=asset.get_bounds()
        report.append({'path':path,'size_cm':[b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2],'materials':[str(s.material_interface.get_path_name()) if s.material_interface else None for s in asset.static_materials]})
        if '/Bush/' in path:
            task=unreal.AssetExportTask();task.object=asset;task.filename=str(OUT/'Previews/reference_Bush.fbx');task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.StaticMeshExporterFBX();task.options=unreal.FbxExportOption()
            unreal.Exporter.run_asset_export_task(task)
    if isinstance(asset,unreal.Texture2D) and '/Bush/' in path:
        task=unreal.AssetExportTask();task.object=asset;task.filename=str(OUT/'Previews/reference_Bush.tga');task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.TextureExporterTGA()
        unreal.Exporter.run_asset_export_task(task)
    if isinstance(asset,unreal.Texture2D) and '/SimpleTree/' in path:
        task=unreal.AssetExportTask();task.object=asset;task.filename=str(OUT/'Previews'/('reference_'+asset.get_name()+'.tga'));task.automated=True;task.prompt=False;task.replace_identical=True;task.exporter=unreal.TextureExporterTGA()
        unreal.Exporter.run_asset_export_task(task)
(OUT/'reference_unreal_inventory.json').write_text(json.dumps(report,indent=2))
