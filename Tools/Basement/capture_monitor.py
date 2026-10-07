"""Read-only monitor close-up capture in a dedicated unattended rendered editor."""
import sys
from pathlib import Path
import unreal as u
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Tools/Basement'))
from capture_unreal import BasementCapture
from verify_monitor import validate

class MonitorCapture(BasementCapture):
    def __init__(self):
        super().__init__()
        self.labels=['Basement_RangeCamera']
        self.output=ROOT/'TunaSweeper/SourceArt/Environment/Basement/RangeMonitor/Previews'
        self.output.mkdir(parents=True,exist_ok=True)
        self.report['requested_cameras']=self.labels

    def select_camera(self):
        self.report['monitor_validation']=validate()
        # The saved room camera focuses downrange; close-up QA must keep the screen legible.
        u.SystemLibrary.execute_console_command(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),'r.DepthOfFieldQuality 0')
        actors={a.get_actor_label():a for a in self.actors.get_all_level_actors()}
        u.SystemLibrary.execute_console_command(u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world(),'r.Streaming.FullyLoadUsedTextures 1')
        origin=actors['ControlConsole_001'].get_actor_location()
        camera=actors['Basement_RangeCamera']
        camera.set_actor_location(origin+u.Vector(-280,-15,100),False,False)
        camera.set_actor_rotation(u.MathLibrary.find_look_at_rotation(camera.get_actor_location(),origin+u.Vector(0,0,34)),False)
        camera.camera_component.field_of_view=35
        super().select_camera()

command=u.SystemLibrary.get_command_line().lower()
assert u.SystemLibrary.is_unattended() and '-executepythonscript' in command and 'capture_monitor.py' in command and '-nullrhi' not in command
CAPTURE=MonitorCapture()
CAPTURE.start()
