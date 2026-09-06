import unreal,json
from pathlib import Path
cls=unreal.EditorAssetLibrary.load_blueprint_class('/Game/Environment/Bunker/GarageDoor/BP_RaidBunkerGarageDoor')
a=unreal.get_default_object(cls)
names=['door_width','door_thickness','shared_upper_panel_height','lower_panel_height','lower_panel_embed_depth','frame_side_width','frame_top_height','frame_depth','led_bar_width','led_bar_height','led_bar_thickness','open_duration','canopy_panel_reveal_ratio','canopy_panel_stack_vertical_step','carrier_final_roll_degrees','override_upper_panel_height','upper_panel_height_override']
r={}
for n in names:
 try:
  v=a.get_editor_property(n);r[n]=list(v) if isinstance(v,unreal.Array) else v
 except Exception as e:r[n]=str(e)
for n in ['frame_top_mesh','frame_left_mesh','frame_right_mesh','led_bar_mesh','metal_material','led_material']:
 v=a.get_editor_property(n);r[n]=v.get_path_name() if v else None
(Path(__file__).resolve().parents[2]/'TunaSweeper/SourceArt/Environment/GarageDoorConcrete/existing_blueprint_layout.json').write_text(json.dumps(r,indent=2),encoding='utf-8')
unreal.log('GARAGE_STRUCTURE_READ_COMPLETE')
