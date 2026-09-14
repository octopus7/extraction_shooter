import unreal
root='/Game/Effects/FlameProjectiles/'
for name in ['M_Flame_Core','M_Flame_1_Tail','M_Flame_2_Tail','M_Flame_3_Tail']:
    mat=unreal.load_asset(root+name)
    custom=next(n for i in range(30) if (n:=unreal.find_object(mat,'MaterialExpressionCustom_'+str(i))))
    code=custom.get_editor_property('code')
    if name=='M_Flame_Core':
        code='float hot=pow(saturate(1-UV.x),0.7); float pulse=0.9+0.1*sin(T*9-UV.x*12);float3 c=lerp(float3(1,0.055,0.003),float3(1,0.78,0.2),hot);return float4(c*2.5*pulse,0.78);'
    else:
        code=code.replace('return float4(col*3,a*(0.7+0.3*bands));','return float4(col*2.7,a*smoothstep(0,0.10,u)*(0.7+0.3*bands));')
    custom.set_editor_property('code',code)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
for actor in actors.get_all_level_actors():
    if isinstance(actor,unreal.PostProcessVolume):
        settings=actor.get_editor_property('settings')
        settings.set_editor_property('bloom_intensity',0.5)
        actor.set_editor_property('settings',settings)
unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('MATERIAL_REFINEMENT_SAVED')
