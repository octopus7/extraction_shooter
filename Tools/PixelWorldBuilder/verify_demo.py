"""Read-only reload, placement/collision checks, and real GPU captures of the sample map.

Run in a dedicated rendered UE 5.7 editor with -ExecutePythonScript after following
Docs/unreal_editor_coordination.md. Never run in an editor containing unsaved work.
"""
from pathlib import Path
import hashlib
import json
import struct
import time
import traceback
import zlib
import unreal as u

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'TunaSweeper/SourceArt/PixelWorldDemo'
OUTPUT = SOURCE / 'Validation'
MAP = '/Game/Prototype/PixelWorldDemo/Maps/L_PixelWorldDemo'
PROFILE = '/Game/Prototype/PixelWorldDemo/DA_PixelWorldDemo'
SHOTS = [('Overview', 'PW_Overview'), ('PathView', 'PW_PathView')]


def package_hashes():
    return {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in (ROOT / 'TunaSweeper/Content/Prototype/PixelWorldDemo').rglob('*') if p.is_file()}


def source_pixels():
    """The supplied fixture uses exact, unfiltered RGB scanlines; no color management."""
    raw = (SOURCE / 'PW_Demo_RGB.png').read_bytes()
    assert raw[:8] == b'\x89PNG\r\n\x1a\n'
    w, h, depth, color = struct.unpack('>IIBB', raw[16:26])
    assert (w, h, depth, color) == (64, 64, 8, 2)
    data, offset = b'', 8
    while offset < len(raw):
        size = struct.unpack('>I', raw[offset:offset+4])[0]
        if raw[offset+4:offset+8] == b'IDAT': data += raw[offset+8:offset+8+size]
        offset += size + 12
    data = zlib.decompress(data)
    pixels = []
    for y in range(h):
        row = data[y*(w*3+1):(y+1)*(w*3+1)]
        assert row[0] == 0
        pixels.extend(tuple(row[1+x*3:4+x*3]) for x in range(w))
    return pixels


class Verify:
    def __init__(self):
        self.start = time.monotonic()
        self.levels = u.get_editor_subsystem(u.LevelEditorSubsystem)
        self.actors = u.get_editor_subsystem(u.EditorActorSubsystem)
        self.before = package_hashes()
        self.report = {'passed': False, 'engine_version': u.SystemLibrary.get_engine_version(), 'captures': []}
        self.state, self.frames, self.index, self.busy = 'load', 0, 0, False
        OUTPUT.mkdir(parents=True, exist_ok=True)
        u.EditorPythonScripting.set_keep_python_script_alive(True)
        self.handle = u.register_slate_post_tick_callback(self.tick)

    def transition(self, state):
        self.state, self.frames, self.stage_start = state, 0, time.monotonic()

    def validate(self):
        manifest = json.loads((SOURCE / 'manifest.json').read_text())
        for name, expected in manifest['png_sha256'].items():
            assert hashlib.sha256((SOURCE / name).read_bytes()).hexdigest() == expected
        profile = u.load_asset(PROFILE)
        assert profile and profile.get_class().get_name() == 'PixelWorldProfile'
        expected = {item['mesh']:tuple(item['rgb']) for item in manifest['vegetation']}
        actors = self.actors.get_all_level_actors()
        roots = [a for a in actors if 'PixelWorldBuilder.Root' in [str(t) for t in a.tags]]
        landscapes = [a for a in actors if isinstance(a, u.Landscape)]
        assert len(roots) == len(landscapes) == 1
        landscape = landscapes[0]
        components = landscape.get_components_by_class(u.LandscapeComponent)
        assert len(components) == 4, len(components)
        origin, scale = landscape.get_actor_location(), landscape.get_actor_scale3d()
        assert abs(origin.x+6400) < .01 and abs(origin.y+6400) < .01
        assert abs(scale.x*126-12800) < .1 and abs(scale.y*126-12800) < .1
        assert landscape.get_editor_property('landscape_material').get_path_name().startswith('/Game/Prototype/PixelWorldDemo/')
        pixels = source_pixels()
        world = u.get_editor_subsystem(u.UnrealEditorSubsystem).get_editor_world()
        ignore = [a for a in actors if a != landscape]
        groups, total, max_error = [], 0, 0.0
        for component in roots[0].get_components_by_class(u.HierarchicalInstancedStaticMeshComponent):
            mesh = component.get_editor_property('static_mesh')
            for slot in mesh.get_editor_property('static_materials'):
                material = slot.get_editor_property('material_interface')
                assert material.get_path_name().startswith('/Game/Prototype/PixelWorldDemo/Materials/')
                assert material.get_editor_property('used_with_instanced_static_meshes')
            rgb = expected[mesh.get_path_name()]
            count = component.get_instance_count()
            assert count > 0
            for i in range(count):
                position = component.get_instance_transform(i, world_space=True).translation
                x, y = int((position.y+6400)/200), int((6400-position.x)/200)
                assert 0 <= x < 64 and 0 <= y < 64
                assert pixels[y*64+x] == rgb, (mesh.get_name(), i, x, y)
                hit = u.SystemLibrary.line_trace_single(world, position+u.Vector(0,0,2000), position-u.Vector(0,0,2000),
                    u.TraceTypeQuery.TRACE_TYPE_QUERY1, True, ignore, u.DrawDebugTrace.NONE, True)
                assert hit and hit.to_tuple()[0], (mesh.get_name(), i, 'missing landscape collision')
                error = abs(hit.to_tuple()[4].z-position.z)
                assert error < 2, (mesh.get_name(), i, error)
                max_error = max(max_error, error)
            groups.append({'mesh':mesh.get_path_name(), 'instances':count, 'source_rgb':rgb})
            total += count
        assert len(groups) == 7 and total > 1000, (len(groups), total)
        self.report.update(landscape_components=len(components), world_extent_m=[128,128], instances=total, groups=groups,
                           all_instances_match_source_rgb=True, materials_support_hism=True, max_ground_error_cm=max_error)
        self.by_name = {a.get_name():a for a in actors}

    def tick(self, _delta):
        if self.busy: return
        self.busy = True
        try:
            assert time.monotonic()-self.start < 600, self.state
            self.frames += 1
            if self.state == 'load':
                if self.frames < 5 or not self.levels.get_viewport_config_keys(): return
                assert self.levels.load_level(MAP)
                self.transition('settle')
            elif self.state == 'settle':
                self.levels.editor_invalidate_viewports()
                if self.frames < 90 or time.monotonic()-self.stage_start < 8: return
                u.AutomationLibrary.finish_loading_before_screenshot()
                self.validate()
                self.transition('camera')
            elif self.state == 'camera':
                name, camera_name = SHOTS[self.index]
                self.camera = self.by_name[camera_name]
                self.levels.pilot_level_actor(self.camera)
                self.levels.set_exact_camera_view(True)
                u.AutomationLibrary.set_editor_active_viewport_view_mode(u.ViewModeIndex.VMI_LIT)
                self.pending = OUTPUT / (name+'.png')
                self.transition('warmup')
            elif self.state == 'warmup':
                self.levels.editor_invalidate_viewports()
                if self.frames < 90 or time.monotonic()-self.stage_start < 5: return
                self.task = u.AutomationLibrary.take_high_res_screenshot(1600,900,str(self.pending),camera=self.camera,
                    mask_enabled=False,capture_hdr=False,delay=1.0,force_game_view=True)
                assert self.task and self.task.is_valid_task()
                self.transition('capture')
            elif self.state == 'capture':
                self.levels.editor_invalidate_viewports()
                if not self.task.is_task_done() or not self.pending.is_file(): return
                image = self.pending.read_bytes()
                assert image[:8] == b'\x89PNG\r\n\x1a\n' and struct.unpack('>II',image[16:24]) == (1600,900)
                self.report['captures'].append({'path':str(self.pending.relative_to(ROOT)), 'sha256':hashlib.sha256(image).hexdigest()})
                self.index += 1
                if self.index < len(SHOTS): self.transition('camera')
                else: self.finish()
        except Exception:
            self.finish(traceback.format_exc())
        finally: self.busy = False

    def finish(self, error=None):
        u.unregister_slate_post_tick_callback(self.handle)
        self.report['packages_unchanged'] = self.before == package_hashes()
        self.report['elapsed_seconds'] = round(time.monotonic()-self.start,2)
        if error: self.report['error'] = error
        self.report['passed'] = not error and self.report['packages_unchanged'] and len(self.report['captures']) == 2
        (OUTPUT/'report.json').write_text(json.dumps(self.report,indent=2)+'\n',encoding='utf-8')
        u.log('PIXEL_DEMO_VERIFY '+json.dumps(self.report))
        u.EditorPythonScripting.set_keep_python_script_alive(False)
        u.SystemLibrary.quit_editor()


if __name__ == '__main__':
    assert '-nullrhi' not in u.SystemLibrary.get_command_line().lower()
    PIXEL_DEMO_VERIFY = Verify()
