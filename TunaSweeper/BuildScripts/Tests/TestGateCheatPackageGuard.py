"""Exercise the package guard on disposable copies; never modifies the built executable."""
import argparse
import pathlib
import shutil
import struct
import subprocess
import tempfile


def exported_offset(data, symbol):
    u16 = lambda pos: struct.unpack_from('<H', data, pos)[0]
    u32 = lambda pos: struct.unpack_from('<I', data, pos)[0]
    pe = u32(60)
    sections = pe + 24 + u16(pe + 20)

    def offset(rva):
        for i in range(u16(pe + 6)):
            section = sections + 40 * i
            start, size, raw = u32(section + 12), u32(section + 16), u32(section + 20)
            if start <= rva < start + size:
                return raw + rva - start
        raise ValueError('Expected file-backed export')

    directory = offset(u32(pe + 136))
    names, ordinals, functions = (offset(u32(directory + n)) for n in (32, 36, 28))
    for i in range(u32(directory + 24)):
        name = offset(u32(names + i * 4))
        if data[name:data.index(0, name)].decode('ascii') == symbol:
            return offset(u32(functions + 4 * u16(ordinals + i * 2)))
    raise ValueError('Missing export ' + symbol)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('executable', type=pathlib.Path)
    args = parser.parse_args()
    data = args.executable.read_bytes()
    gate = exported_offset(data, 'TunaSweeperGateCheatControl')
    movement = exported_offset(data, 'TunaSweeperMovementCheatControl')
    script = pathlib.Path(__file__).resolve().parents[1] / 'ValidateMovementCheatPackage.ps1'
    shell = shutil.which('pwsh') or shutil.which('powershell')
    assert shell, 'PowerShell required'
    with tempfile.TemporaryDirectory(prefix='gate-cheat-package-') as directory:
        root = pathlib.Path(directory)
        target = root / 'Binaries/Win64/TunaSweeper-Win64-Shipping.exe'
        target.parent.mkdir(parents=True)

        def run(label, offset=None, extra_helper=False):
            modified = bytearray(data)
            if offset is not None:
                modified[offset] = 1
            target.write_bytes(modified)
            helper = root / 'TunaBuildHelper.dll'
            if extra_helper:
                helper.write_bytes(b'fixture')
            result = subprocess.run([shell, '-NoProfile', '-File', str(script), '-PackageRoot', str(root)],
                                    capture_output=True)
            expected_success = offset is None and not extra_helper
            assert (result.returncode == 0) == expected_success, (label, result.stdout, result.stderr)
            if helper.exists():
                helper.unlink()
            print('PASS:', label)

        run('clean binary')
        run('active movement token rejected', movement + 16)
        run('old progression protocol rejected', gate + 8)
        run('queued gate reset rejected', gate + 24)
        run('queued progression operation rejected', gate + 56)
        run('stale gate snapshot rejected', gate + 88)
        run('tail row runtime state rejected', gate + 25863)
        run('helper artifact rejected', extra_helper=True)


if __name__ == '__main__':
    main()
