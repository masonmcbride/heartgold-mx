"""Reviewed unsigned bit packing at executed original save-init boundaries."""


def adapt_native_bits(source, name, body):
    edits = {
        ('src/bg_window.c', 'FillWindowPixelBuffer'): (
            ('fillValue << 24', '(u32)fillValue << 24'),
        ),
        ('src/pokedex.c', 'SetDex4Flag'): (
            ('15 << (24 + 4 * idx)', '15u << (24 + 4 * idx)'),
            ('form << (24 + 4 * idx)', '(u32)form << (24 + 4 * idx)'),
        ),
        ('src/pokemon.c', 'CreateBoxMon'): (
            ('fixedPersonality = (LCRandom() | (LCRandom() << 16));',
             'fixedPersonality = (LCRandom() | ((u32)LCRandom() << 16));'),
            ('fixedOtId = (LCRandom() | (LCRandom() << 16));',
             'fixedOtId = (LCRandom() | ((u32)LCRandom() << 16));'),
        ),
        ('src/pokemon.c', 'CreateMonWithNature'): (
            ('LCRandom() << 16', '(u32)LCRandom() << 16'),
        ),
        ('src/pokemon.c', 'CreateMonWithGenderNatureLetter'): (
            ('LCRandom() << 16', '(u32)LCRandom() << 16'),
        ),
        ('src/sys_vars.c', 'Save_VarsFlags_RollLotoId'): (
            ('hi << 16 | lo', '(u32)hi << 16 | lo'),
        ),
    }.get((source, name), ())
    for old, new in edits:
        if body.count(old) != 1:
            raise ValueError(('native bit-packing source drift', source, name, old))
        body = body.replace(old, new)
    return body, [dict(original=old, native=new, reason='unsigned record bit packing must not overflow promoted signed int') for old,new in edits]
