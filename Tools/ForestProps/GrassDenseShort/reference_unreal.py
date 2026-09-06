"""Read-only export of existing nature meshes for visual style comparison."""
from pathlib import Path
import json
import unreal

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / 'TunaSweeper/SourceArt/Environment/GrassDenseShort/Reference'
OUT.mkdir(parents=True, exist_ok=True)
paths = {
    'Bush': '/Game/Nature/Bush/Bush_Combined',
    'GrassLow': '/Game/Nature/GrassLow/SM_GrassLow',
    'Flower': '/Game/Nature/Flower/SM_Flower',
    'SimpleTree': '/Game/Nature/SimpleTree/SM_SimpleTree',
    'Wood': '/Game/Nature/Wood/SM_LogA',
    'RockBasic': '/Game/Nature/RockBasic/RockM_1',
}
report = {}
for name, path in paths.items():
    mesh = unreal.load_asset(path)
    assert isinstance(mesh, unreal.StaticMesh), path
    task = unreal.AssetExportTask()
    task.object = mesh
    task.filename = str(OUT / (name + '.fbx'))
    task.automated = True
    task.prompt = False
    task.replace_identical = True
    task.exporter = unreal.StaticMeshExporterFBX()
    assert unreal.Exporter.run_asset_export_task(task), path
    b = mesh.get_bounds()
    report[name] = {'asset': path, 'size_cm': [b.box_extent.x*2,b.box_extent.y*2,b.box_extent.z*2],
                    'materials': [str(s.material_interface.get_path_name()) for s in mesh.static_materials]}
    mat=mesh.static_materials[0].material_interface
    node=unreal.MaterialEditingLibrary.get_material_property_input_node(mat,unreal.MaterialProperty.MP_BASE_COLOR)
    report[name]['base_node']=node.get_class().get_name() if node else None
    if isinstance(node,unreal.MaterialExpressionTextureSample):
        texture=node.get_editor_property('texture')
        report[name]['base_texture']=texture.get_path_name()
        t=unreal.AssetExportTask()
        t.object=texture;t.filename=str(OUT/(name+'_BaseColor.tga'));t.automated=True;t.prompt=False
        t.exporter=unreal.TextureExporterTGA();t.replace_identical=True
        assert unreal.Exporter.run_asset_export_task(t)
tex = unreal.load_asset('/Game/Nature/Bush/Bush_Cluster_Base_Color')
t = unreal.AssetExportTask()
t.object=tex; t.filename=str(OUT/'Bush.tga'); t.automated=True; t.prompt=False
t.exporter=unreal.TextureExporterTGA(); t.replace_identical=True
assert unreal.Exporter.run_asset_export_task(t)
(OUT/'existing_assets.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
