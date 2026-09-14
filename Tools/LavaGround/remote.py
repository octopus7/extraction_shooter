"""One-off local editor transport; only targets this project's open editor."""
import pathlib
import sys
import time
sys.path.insert(0, 'C:/Program Files/Epic Games/UE_5.7/Engine/Plugins/Experimental/PythonScriptPlugin/Content/Python')
import remote_execution

remote = remote_execution.RemoteExecution()
remote.start()
try:
    time.sleep(2)
    nodes = [n for n in remote.remote_nodes if n.get('project_root', '').replace('\\', '/').rstrip('/').lower() == 'd:/github/extraction_shooter/tunasweeper']
    if len(nodes) != 1:
        raise RuntimeError(f'Expected one TunaSweeper editor, found {len(nodes)}')
    remote.open_command_connection(nodes[0]['node_id'])
    source = pathlib.Path(sys.argv[1]).read_text(encoding='utf-8') if len(sys.argv) > 1 else sys.stdin.read()
    result = remote.run_command(source)
    for entry in result.get('output', []):
        print(entry.get('output', ''))
    if not result['success']:
        raise RuntimeError(result['result'])
finally:
    remote.stop()
