"""Validate PNG chunks and finalize UE exports without changing any pixels."""
from pathlib import Path
import struct
import zlib


def validate_png(path, allow_trailing=False):
    path = Path(path)
    data = path.read_bytes()
    assert data[:8] == b'\x89PNG\r\n\x1a\n', 'Invalid PNG signature: ' + str(path)
    offset = 8
    while offset + 12 <= len(data):
        length = struct.unpack('>I', data[offset:offset + 4])[0]
        kind = data[offset + 4:offset + 8]
        end = offset + 12 + length
        assert end <= len(data), 'Truncated PNG chunk: ' + str(path)
        actual_crc = zlib.crc32(data[offset + 4:offset + 8 + length]) & 0xffffffff
        expected_crc = struct.unpack('>I', data[offset + 8 + length:end])[0]
        assert actual_crc == expected_crc, 'PNG CRC mismatch: ' + str(path)
        offset = end
        if kind == b'IEND':
            assert length == 0, 'Invalid PNG IEND: ' + str(path)
            assert allow_trailing or offset == len(data), 'Trailing bytes after PNG IEND: ' + str(path)
            return offset
    raise AssertionError('Missing PNG IEND: ' + str(path))


def finalize_exported_png(path):
    """Remove exporter allocation padding after IEND; keep every PNG chunk byte."""
    path = Path(path)
    payload_size = validate_png(path, allow_trailing=True)
    padding = path.stat().st_size - payload_size
    if padding:
        with path.open('r+b') as stream:
            stream.truncate(payload_size)
    return padding
