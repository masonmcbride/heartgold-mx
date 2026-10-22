"""Build-local NARC writer shared by resource preparation tools."""
from generated_files import write_bytes_if_changed
import struct

def pack(path,members):
    image=bytearray();fat=bytearray()
    for member in members:
        start=len(image);image.extend(member);fat.extend(struct.pack('<II',start,len(image)))
        image.extend(b'\xff'*((-len(image))%4))
    def block(tag,data): return tag+struct.pack('<I',8+len(data))+data
    body=block(b'BTAF',struct.pack('<HH',len(members),0)+fat)+block(b'BTNF',struct.pack('<IHH',4,0,1))+block(b'GMIF',image)
    write_bytes_if_changed(path, struct.pack('<4sHHIHH',b'NARC',0xfffe,0x100,16+len(body),16,3)+body)
