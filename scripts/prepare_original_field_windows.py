"""Project original field Window owners without the menu preview Window ABI.

The native records deliberately retain native pointers and never enter ARM32
memory. Encoded strings reuse the existing original String owner. Lazy message ownership and decryption use original encrypted banks, separately
from the preview message service. Drawing and active map-name tasks remain open.
"""
import hashlib
import json
from pathlib import Path
import re

from generated_files import write_text_if_changed
from prepare_original_field import declaration, functions


ALIASES = {
    'Window': 'HGFieldWindow',
    'FillWindowPixelBuffer':'hg_field_window_fill',
    'MI_CpuFillFast':'MI_CpuFill32',
    'CopyU16ArrayToStringN':'hg_field_string_copy_n',
    'String_SetEmpty':'hg_field_string_empty',
    'ReadMsgData_ExistingNarc_ExistingString':'hg_field_message_read',
    'NARC_New':'hg_field_message_narc_new',
    'LoadSingleElementFromNarc':'hg_field_message_direct',
    'FreeMsgDataRawData':'hg_field_message_raw_free',
    'String': 'HGMonString',
    'String_New': 'hg_mon_string_new',
    'String_Delete': 'hg_mon_string_delete',
    'MsgData': 'HGFieldMsgData',
    'NewMsgDataFromNarc': 'canonical_field_message_lease_new',
    'DestroyMsgData': 'canonical_field_message_lease_free',
    'AddWindowParameterized': 'hg_field_window_add',
    'RemoveWindow': 'hg_field_window_remove',
    'FieldDrawMapName_SetupWindow': 'hg_field_map_name_setup_window',
    'FieldDrawMapName_ResetState': 'hg_field_map_name_reset_state',
    'FieldDrawMapName_Init': 'hg_field_map_name_init',
    'FieldDrawMapNameInfo_Create': 'hg_field_map_name_new',
    'FieldDrawMapNameInfo_Destroy': 'hg_field_map_name_delete',
    'Signpost_Init': 'hg_field_signpost_new',
    'Signpost_Free': 'hg_field_signpost_delete',
}


def renamed(text):
    return re.sub(r'\b(' + '|'.join(map(re.escape, ALIASES)) + r')\b',
                  lambda match: ALIASES[match[0]], text)


def generate(root, out):
    """Write native type/prototype and body includes; return records and inputs.

    Include types before the existing original parent include, then bodies after
    it (the original native BgConfig definition must be complete). Bind parent
    calls through hg_field_map_name_new/delete and hg_field_signpost_new/delete.
    Original lazy message bodies and an explicit raw-archive selector are emitted.
    Do not use the inventory/menu MsgData or Window implementations for them.
    """
    root, out = Path(root), Path(out)
    bg_path = root / 'include/bg_window.h'
    name_path = root / 'include/field/draw_map_name.h'
    sign_path = root / 'include/field/signpost.h'
    types = '\n'.join((
        '#pragma once',
        'typedef struct BgConfig BgConfig;',
        renamed(declaration((root/'include/pm_string.h').read_text(), 'String')),
        declaration((root/'include/msgdata.h').read_text(), '_MAT_ENTRY'),
        declaration((root/'include/msgdata.h').read_text(), '_MAT'),
        renamed(declaration((root/'include/msgdata.h').read_text(), 'MsgData')),
        renamed(declaration(bg_path.read_text(), 'Window')),
        renamed(declaration(name_path.read_text(), 'FieldDrawMapNameInfo')),
        renamed(declaration(sign_path.read_text(), 'Signpost')),
        '_Static_assert(sizeof(HGFieldWindow)==2*sizeof(void *)+8,"native original field Window");',
        '_Static_assert(__builtin_offsetof(HGFieldWindow,pixelBuffer)==sizeof(void *)+8,"native field pixel owner");',
        'HGMonString *hg_mon_string_new(u32,enum HeapID);',
        'void hg_mon_string_delete(HGMonString *);',
        'static NARC *canonical_original_message_archive(NarcId,enum HeapID);',
        'static HGFieldMsgData *canonical_field_message_lease_new(int,NarcId,int,enum HeapID);',
        'static void canonical_field_message_lease_free(HGFieldMsgData *);',
        'static void hg_field_string_copy_n(HGMonString *,const u16 *,u32);',
        'static void hg_field_string_empty(HGMonString *);',
        'static void hg_field_message_read(NARC *,u32,u32,enum HeapID,HGMonString *);',
        'static FieldDrawMapNameInfo *hg_field_map_name_new(BgConfig *);',
        'static void hg_field_map_name_delete(FieldDrawMapNameInfo *);',
        'static Signpost *hg_field_signpost_new(enum HeapID);',
        'static void hg_field_signpost_delete(Signpost *);',
    )) + '\n'
    selection = {
        'src/msgdata.c':('Decrypt1','Decrypt2','FreeMsgDataRawData','NewMsgDataFromNarc','DestroyMsgData','ReadMsgData_ExistingNarc_ExistingString'),
        'src/pm_string.c':('String_SetEmpty','CopyU16ArrayToStringN'),
        'src/bg_window.c': ('AddWindowParameterized', 'RemoveWindow', 'FillWindowPixelBuffer'),
        'src/field/draw_map_name.c': ('FieldDrawMapName_SetupWindow',
            'FieldDrawMapName_ResetState', 'FieldDrawMapName_Init',
            'FieldDrawMapNameInfo_Create', 'FieldDrawMapNameInfo_Destroy'),
        'src/field/signpost.c': ('Signpost_Init', 'Signpost_Free'),
    }

    def adapt(relative, name, body):
        from native_bit_operations import adapt_native_bits
        body,bit_changes=adapt_native_bits(relative,name,body)
        result = renamed(body)
        # Keep helpers local to the canonical native FieldSystem translation unit.
        if not result.startswith(('static ','inline static ')):
            result = 'static ' + result
        return result, bit_changes+[dict(
            reason='native field Window, original encoded String and lazy encrypted message-bank ownership',
            renamed={key: value for key, value in ALIASES.items()
                     if re.search(r'\b' + re.escape(key) + r'\b', body)})]

    bodies, records = functions(root, selection, transform=adapt)
    prelude='''
#include "constants/charcode.h"
void MI_CpuFill32(void *,u32,u32);
#define MSGDATA_LOAD_DIRECT 0
#define MSGDATA_LOAD_LAZY 1
typedef int MsgDataLoadType;
static NARC *hg_field_message_narc_new(NarcId id,enum HeapID heap){
    return canonical_original_message_archive(id,heap);
}
static void *hg_field_message_direct(NarcId id,s32 file,enum HeapID heap){
    (void)id;(void)file;(void)heap;unsupported("direct message-bank loading");return NULL;
}
'''
    string_source=(root/'src/pm_string.c').read_text()
    prelude+=string_source[string_source.index('#define STRING_MAGIC'):string_source.index('String *String_New')]
    includes = '#include "constants/map_sections.h"\n#include "files/msgdata/msg.naix"\n'
    includes+=declaration(bg_path.read_text(),'GFBppMode','enum')+'\n'
    state_defines = '\n'.join(re.findall(r'^#define POSITION_STATE_\w+[^\n]*',
        (root / 'src/field/draw_map_name.c').read_text(), re.M)) + '\n'
    write_text_if_changed(out / 'original_field_windows_types.inc', types)
    write_text_if_changed(out / 'original_field_windows.inc', includes + prelude + state_defines + bodies)
    dependencies = [bg_path, name_path, sign_path, *(root / p for p in selection)]
    provenance = dict(
        functions=records,
        declarations={str(p): hashlib.sha256(p.read_bytes()).hexdigest()
                      for p in (bg_path, name_path, sign_path)},
        boundary='original native constructors/destructors, lazy message decryption and real pixel allocation; no menu Window, map-name drawing or animation admission',
        lifetime='map-name deletion requires no active callback; FieldDrawMapName_Reset belongs to the original caller protocol',
        failure='constructor interruption requires caller acquisition capture rollback, never its complete-object destructor')
    write_text_if_changed(out / 'original_field_windows_provenance.json', json.dumps(provenance, indent=2) + '\n')
    return provenance, dependencies


if __name__ == '__main__':
    import sys
    generate(*map(Path, sys.argv[1:]))
