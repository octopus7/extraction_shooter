"""One-off camera captures; removed with the map generator."""
_screens=['UE_BasePlayCamera','UE_ExpansionPlayCamera','UE_WindowServices','UE_DoorFrame','UE_Annex']
_capture=None;_capture_ticks=0;_capture_busy=False
def capture_tick(dt):
    global _capture,_capture_ticks,_capture_busy
    if _capture_busy:return
    _capture_ticks+=1
    _capture_busy=True
    try:
        assert _capture_ticks<1800,'Screenshot timeout'
        if _capture and not _capture.is_task_done():return
        if _screens:
            name=_screens.pop(0)
            unreal.log('EXPANSION_CAPTURE_REQUEST '+name)
            _capture=unreal.AutomationLibrary.take_high_res_screenshot(1400,1000,str(OUT/'Previews'/f'{name}.png'),camera=bylabel[name],delay=5.0)
            assert _capture,'No screenshot task returned'
        else:
            assert all((OUT/'Previews'/f'{name}.png').is_file() for name in ['UE_BasePlayCamera','UE_ExpansionPlayCamera','UE_WindowServices','UE_DoorFrame','UE_Annex'])
            unreal.unregister_slate_post_tick_callback(_capture_handle)
            (OUT/'unreal_map_validation.json').write_text(json.dumps(report,indent=2));unreal.SystemLibrary.quit_editor()
    except Exception:
        import traceback
        report['passed']=False;report['error']=traceback.format_exc()
        unreal.unregister_slate_post_tick_callback(_capture_handle)
        (OUT/'unreal_map_validation.json').write_text(json.dumps(report,indent=2));unreal.SystemLibrary.quit_editor()
    finally:_capture_busy=False
_capture_handle=unreal.register_slate_post_tick_callback(capture_tick)
