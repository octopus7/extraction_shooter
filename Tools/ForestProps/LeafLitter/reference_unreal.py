from pathlib import Path
import unreal, json
ROOT=Path(__file__).resolve().parents[3]
OUT=ROOT/'TunaSweeper/SourceArt/Environment/ForestProps/LeafLitter'
mesh=unreal.load_asset('/Game/Nature/Bush/Bush_Combined')
task=unreal.AssetExportTask();task.object=mesh
task.filename=str(OUT/'Previews/Reference_Bush.fbx');task.automated=True;task.prompt=False
task.exporter=unreal.StaticMeshExporterFBX();task.options=unreal.FbxExportOption()
assert unreal.Exporter.run_asset_export_task(task)
tex=unreal.load_asset('/Game/Nature/Bush/Bush_Cluster_Base_Color')
task=unreal.AssetExportTask();task.object=tex
task.filename=str(OUT/'Previews/Reference_Bush.tga');task.automated=True;task.prompt=False
task.exporter=unreal.TextureExporterTGA()
assert unreal.Exporter.run_asset_export_task(task)
unreal.log('LEAFLITTER_REFERENCE_EXPORTED')
