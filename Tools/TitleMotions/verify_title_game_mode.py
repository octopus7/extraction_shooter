"""Read-only IntroMap PIE regression: no gameplay pawn, menu and camera retained.

Run in a separate UnrealEditor process with -ExecutePythonScript=<this file>.
Never saves assets or alters the user's editor session.
"""
import json
import time
import traceback
from pathlib import Path
import unreal

OUT = Path(__file__).resolve().parents[2] / 'TunaSweeper/Saved/Automation/TitleGameMode.json'
OUT.parent.mkdir(parents=True, exist_ok=True)
report = {'status': 'running'}
OUT.write_text(json.dumps(report), encoding='utf-8')
unreal.EditorLoadingAndSavingUtils.load_map('/Game/Maps/IntroMap')
menu_class = unreal.EditorAssetLibrary.load_blueprint_class('/Game/UI/WBP_IntroMenu')
gameplay_class = unreal.EditorAssetLibrary.load_blueprint_class('/Game/Core/BP_TunaSweeperGameMode')
assert menu_class and gameplay_class, 'Required gameplay/menu assets must load before PIE'
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
started = time.monotonic()
frames = 0
busy = False


def tick(delta):
    global frames, busy
    if busy:
        return
    busy = True
    frames += 1
    try:
        assert time.monotonic() - started < 180, 'PIE timed out'
        if frames == 10:
            levels.editor_request_begin_play()
        if frames < 100:
            return
        world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_game_world()
        if not world:
            return
        pc = unreal.GameplayStatics.get_player_controller(world, 0)
        assert pc, 'Title must retain the local player controller'
        pawns = unreal.GameplayStatics.get_all_actors_of_class(world, unreal.Pawn)
        report['pawns'] = [p.get_class().get_name() for p in pawns]
        report['controller'] = pc.get_class().get_name()
        report['view_target'] = pc.get_view_target().get_class().get_name()
        assert not pawns, 'IntroMap must not spawn gameplay or spectator pawns: ' + str(report['pawns'])
        assert pc.get_controlled_pawn() is None, 'Title controller must not possess a pawn'
        assert isinstance(pc.get_view_target(), unreal.TunaSweeperTitlePresentationActor), 'Title camera must remain active'
        menus = unreal.WidgetLibrary.get_all_widgets_of_class(world, menu_class, False)
        assert any(w.is_in_viewport() for w in menus), 'Title menu must be in viewport without a pawn'
        gameplay = unreal.get_default_object(gameplay_class)
        assert gameplay.get_editor_property('default_pawn_class'), 'Gameplay must retain its default pawn'
        report['gameplay_pawn'] = gameplay.get_editor_property('default_pawn_class').get_name()
        report['status'] = 'passed'
    except Exception:
        report['status'] = 'failed'
        report['error'] = traceback.format_exc()
    finally:
        busy = False
        if report['status'] != 'running':
            OUT.write_text(json.dumps(report, indent=2), encoding='utf-8')
            unreal.log('TITLE_GAME_MODE_RESULT ' + json.dumps(report))
            unreal.unregister_slate_post_tick_callback(handle)
            levels.editor_request_end_play()
            unreal.SystemLibrary.quit_editor()


handle = unreal.register_slate_post_tick_callback(tick)
unreal.EditorPythonScripting.set_keep_python_script_alive(True)
