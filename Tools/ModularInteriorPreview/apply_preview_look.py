"""In the preview-map editor Python console: set MI_LOOK, MI_DIRT and MI_DIRT_SCALE then exec this file.
Does not save. Undoable review material changes only; no new meshes.
"""
import unreal,os
DEST='/Game/Environment/ModularInteriorPreview'
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
assert world.get_path_name().startswith(DEST+'/Maps/'),'Open only the dedicated MI preview map'
look=os.environ.get('MI_LOOK','Light');assert look in ('Light','Dark','Managed')
strength=float(os.environ.get('MI_DIRT','.65'));size=float(os.environ.get('MI_DIRT_SCALE','1'))
material=unreal.load_asset(f'{DEST}/Materials/MI_MI_Concrete_{look}')
with unreal.ScopedEditorTransaction('Modular interior review look'):
    for a in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if not isinstance(a,unreal.StaticMeshActor):continue
        c=a.static_mesh_component
        if not c.static_mesh or not c.static_mesh.get_path_name().startswith(DEST+'/Meshes/'):continue
        c.modify()
        for i in range(c.get_num_materials()):
            mat=c.get_material(i)
            if mat and 'Concrete' in mat.get_name():c.set_material(i,material)
        c.set_default_custom_primitive_data_float(0,strength)
        c.set_default_custom_primitive_data_float(1,size)
