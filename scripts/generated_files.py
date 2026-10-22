"""Keep generated build inputs' timestamps stable when their contents match."""
from pathlib import Path


def write_bytes_if_changed(path, data):
    path = Path(path)
    try:
        if path.read_bytes() == data:
            return len(data)
    except FileNotFoundError:
        pass
    return path.write_bytes(data)


def write_text_if_changed(path, text):
    write_bytes_if_changed(path, text.encode('utf-8'))
    return len(text)
