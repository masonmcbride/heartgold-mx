"""Check original indexed art/palette separation, including every species/sex."""
import pathlib, struct, sys
root, archive = map(pathlib.Path, sys.argv[1:])
data = archive.read_bytes()
blocks = {}; at = 16
while at < len(data):
    tag, size = struct.unpack_from('<4sI', data, at)
    assert size >= 8 and at + size <= len(data)
    blocks[tag] = data[at+8:at+size]; at += size
fat, image = blocks[b'BTAF'], blocks[b'GMIF']
assert struct.unpack_from('<H', fat)[0] == 494*8

def chunks(png):
    out = []; at = 8
    while at < len(png):
        size = struct.unpack_from('>I', png, at)[0]
        out.append((png[at+4:at+8], png[at+8:at+8+size])); at += size+12
    return out

checked = 0
for species in range(1, 494):
    folder = root/'files/poketool/pokegra/pokegra'/f'{species:04}'
    for shiny, source in enumerate(('front', 'back')):
        candidates = [(folder/gender/(source+'.png')).read_bytes() for gender in ('female', 'male')]
        original_palette = dict(chunks(next(x for x in candidates if x)))[b'PLTE']
        for gender_index, gender in enumerate(('male', 'female')):
            for facing_index, facing in enumerate(('front', 'back')):
                key = species*8+shiny*4+gender_index*2+facing_index
                start, end = struct.unpack_from('<II', fat, 4+key*8)
                packed = image[start:end]; original = (folder/gender/(facing+'.png')).read_bytes()
                if not original:
                    assert not packed; continue
                actual = chunks(packed)
                assert dict(actual)[b'PLTE'] == original_palette, (species, gender, facing, shiny)
                # Compressed pixel indices, alpha, dimensions and metadata are unchanged.
                assert [(k,v) for k,v in actual if k != b'PLTE'] == [(k,v) for k,v in chunks(original) if k != b'PLTE']
                checked += 1
print(f'{checked} original sprite variants: normal/shiny palettes match original build; pixel indices and transparency preserved')
