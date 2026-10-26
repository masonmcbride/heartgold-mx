#!/usr/bin/env python3
"""Project unchanged original owner declarations without importing SDK managers.

Pointer targets stay opaque with their verified original struct tags. No field
is omitted, reordered, widened manually or added. This native C declaration must
never be handed to ARM code expecting DS offsets. The legacy owner is excluded.
"""
import hashlib
import json
from pathlib import Path
import re
import sys

from generated_files import write_text_if_changed


def declaration(text, name, category='struct'):
    pattern = r'^(?:typedef )?' + category + r' ' + re.escape(name) + r' \{.*?^\}[^\n]*;'
    matches = list(re.finditer(pattern, text, re.M | re.S))
    if len(matches) != 1:
        raise ValueError(('ambiguous/missing original declaration', name, len(matches)))
    return matches[0][0]


def native_field_initialization(relative, name, body):
    """Exclude DS cartridge instrumentation, never synthesize check results.

    These exact spans only load/check DS protection and deliberately exhaust
    heaps. None supplies a game resource or initialization value. Keep line
    positions, hash both bodies, and reject source drift before compilation.
    """
    if (relative, name) != ('src/overlay_124.c', 'FieldSystem_Init'):
        return body, []
    exclusions = (
        '    u32 key = 2441 * 4073; // these are both prime',
        '    FS_LoadOverlay(MI_PROCESSOR_ARM9, FS_OVERLAY_ID(ds_protect));',
        '    key += 769 * (!DSProt_DetectNotFlashcart(ov124_02260D68)); // 769 is prime',
        '    key += 47 * (!DSProt_DetectNotDummy(ov124_02260D6C)); // 47 is prime',
        '    u32 key2 = 929 * DSProt_DetectEmulator(ov124_02260D58); // 929 is prime',
        '    FS_UnloadOverlay(MI_PROCESSOR_ARM9, FS_OVERLAY_ID(ds_protect));',
        '    // all combinations of the three prime multipliers above are coprime with 2441 and 4073\n'
        '    if ((key + key2) % 2441) {\n        ov124_02260D1C(fieldSystem);\n    }',
        '    if ((key + key2) % 4073) {\n        ov124_02260D1C(fieldSystem);\n    }',
    )
    records = []
    for span in exclusions:
        if body.count(span) != 1:
            raise ValueError(('native protection adaptation source drift', span))
        records.append(dict(reason='DS cartridge/encoded ARM instrumentation and heap-exhaustion response do not apply to native code',
                            removed=span, sha256=hashlib.sha256(span.encode()).hexdigest()))
        body = body.replace(span, ''.join('\n' if c == '\n' else ' ' for c in span))
    if re.search(r'\b(?:key2?|DSProt_\w+|ov124_\w+|ds_protect)\b', body):
        raise ValueError('unreviewed protection dependency remains')
    return body, records


def functions(root, selection, transform=None):
    declarations, bodies, provenance = [], [], []
    for relative, names in selection.items():
        source = (root / relative).read_text()
        for name in names:
            matches = list(re.finditer(r'^(?:static )?[\w* ]+\b' + re.escape(name) + r'\([^;{}]*\) \{.*?^\}', source, re.M | re.S))
            if len(matches) != 1:
                raise ValueError(('ambiguous/missing original body', relative, name, len(matches)))
            match = matches[0]
            emitted, adaptations = transform(relative, name, match[0]) if transform else (match[0], [])
            declarations.append(emitted.split(' {', 1)[0] + ';')
            bodies.append(f'#line {source[:match.start()].count(chr(10)) + 1} "{root / relative}"\n' + emitted)
            record=dict(function=name, source=relative, sha256=hashlib.sha256(match[0].encode()).hexdigest())
            if adaptations:
                record.update(emitted_sha256=hashlib.sha256(emitted.encode()).hexdigest(), adaptations=adaptations)
            provenance.append(record)
    return '\n'.join(declarations + bodies) + '\n', provenance


def prepare(root, out):
    field = (root / 'include/field_system.h').read_text()
    menu = (root / 'include/menu_input_state.h').read_text()
    owner = declaration(field, 'FieldSystem')
    names = set(re.findall(r'^    (\w+)\s*\*', owner, re.M)) - {'void'}
    names.update(('LocalMapObject', 'OverlayManager'))
    headers = {p: p.read_text() for p in sorted((root / 'include').rglob('*.h'))}
    inputs = {root / 'include/field_system.h', root / 'include/menu_input_state.h', root / 'src/field_system.c'}
    forwards = []
    for name in sorted(names):
        tags = set()
        for path, text in headers.items():
            found = set(re.findall(r'typedef\s+struct\s+(\w+)\s*' + re.escape(name) + r'\s*;', text))
            if re.search(r'typedef\s+struct\s+' + re.escape(name) + r'\s*\{', text):
                found.add(name)
            if found:
                inputs.add(path)
                tags.update(found)
        if tags != {name}:
            raise ValueError(('unverified pointer tag', name, tags))
        forwards.append(f'typedef struct {name} {name};')
    parts = ['#pragma once',
             '#ifdef POKEHEARTGOLD_FIELD_SYSTEM_H',
             '#error "Original field owner cannot share a translation unit with another field definition"',
             '#endif', '#define POKEHEARTGOLD_FIELD_SYSTEM_H',
             '#define HG_ORIGINAL_FIELD_OWNER 1', '#include "compat/overlays.h"',
             'typedef s32 fx32; /* Exact Q12 scalar, as in the existing graphics backend. */',
             *forwards,
             declaration(menu, 'MenuInputState', 'enum'),
             declaration(menu, 'MenuInputStateMgr'),
             declaration(field, 'FollowMon'),
             declaration(field, 'FieldProcessManager'), owner,
             declaration(field, 'FieldInput')]
    for name in ('sub_0203DF7C', 'sub_0203DF8C', 'sub_0203DFA4',
                 'FieldSystem_LoadFieldOverlayInternal', 'FieldSystem_LaunchApplication', 'FieldMap_Init', 'FieldMap_Main', 'FieldMap_Exit'):
        matches = re.findall(r'^\w+[ *]+' + name + r'\([^;{}]*\);', field, re.M)
        if len(matches) != 1:
            raise ValueError(('original prototype', name, matches))
        parts.extend(matches)
    write_text_if_changed(out / 'original_field_owner.h', '\n'.join(parts) + '\n')
    # Original process mechanisms only. Full constructor, field control and map
    # lifecycle remain excluded until their resource/ABI imports are implemented.
    names = ('sub_0203DF34', 'sub_0203DF7C', 'sub_0203DF8C', 'sub_0203DFA4',
             'FieldSystem_LaunchApplication', 'ppOverlayManager_RunFrame_DeleteIfFinished',
             'FieldSystem_Main')
    source = (root / 'src/field_system.c').read_text()
    bodies, provenance = [], []
    for name in names:
        matches = list(re.finditer(r'^(?:static )?[\w* ]+\b' + name + r'\([^;{}]*\) \{.*?^\}', source, re.M | re.S))
        if len(matches) != 1:
            raise ValueError(('ambiguous/missing process body', name, len(matches)))
        match = matches[0]
        bodies.append(f'#line {source[:match.start()].count(chr(10)) + 1} "{root / "src/field_system.c"}"\n' + match[0])
        provenance.append(dict(function=name, source='src/field_system.c', sha256=hashlib.sha256(match[0].encode()).hexdigest()))
    task_source=(root/'src/task.c').read_text()
    task_names=re.findall(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{',task_source,re.M)
    canonical_names=set(names)|set(task_names)|{'FieldSystem_LoadFieldOverlayInternal','FieldSystem_Control','ov01_021F6830'}
    write_text_if_changed(out/'original_field_task_names.h','#pragma once\n'+''.join(f'#define {n} hg_original_task_{n}\n' for n in sorted(canonical_names)))
    inputs.add(root/'src/task.c')
    write_text_if_changed(out / 'original_field_process.inc', '\n'.join(bodies) + '\n')
    write_text_if_changed(out / 'original_field_provenance.json', json.dumps(provenance, indent=2) + '\n')
    constructor_sources = {
        'src/field_system.c': ('FieldSystem_New', 'FieldSystem_Delete'),
        'src/overlay_124.c': ('FieldSystem_Init',),
        'src/map_events.c': ('Field_AllocateMapEvents', 'Field_FreeMapEvents'),
        'src/map_matrix.c': ('MapMatrix_New', 'MapMatrix_Free'),
        'src/unk_02092BB8.c': ('UnkStruct_02092BB8_New', 'UnkStruct_02092BB8_Set', 'UnkStruct_02092BB8_Free'),
        'src/follow_mon.c': ('FieldSystem_UnkSub108_Alloc',),
        'src/unk_02092BE8.c': ('GearPhoneRingManager_New', 'GearPhoneRingManager_Delete', 'gearRingingManagerReset', 'GearPhone_ToggleRinging', 'SysTask_RingGearPhone'),
        'src/save_local_field_data.c': ('Save_LocalFieldData_Get', 'LocalFieldData_GetCurrentPosition'),
        'src/save_pokegear.c': ('SaveData_Pokegear_Get', 'SaveData_GetPhoneCallPersistentState', 'SavePokegear_GetPhoneCallPersistentState'),
    }
    code, records = functions(root, constructor_sources, transform=native_field_initialization)
    names = [name for selected in constructor_sources.values() for name in selected]
    write_text_if_changed(out / 'original_field_constructor_names.h',
                          '/* Link isolation only: never bind legacy FieldSystem consumers to these bodies. */\n#pragma once\n' +
                          ''.join(f'#define {name} hg_original_{name}\n' for name in names))
    for record in records:
        record['native_symbol'] = 'hg_original_' + record['function']
    write_text_if_changed(out / 'original_field_constructor.inc', code)
    write_text_if_changed(out / 'original_field_constructor_provenance.json', json.dumps(records, indent=2) + '\n')
    types = []
    for relative, name in (
        ('src/save_local_field_data.c', 'LocalFieldData'),
        ('include/field_system.h', 'FieldSystemUnkSub108'),
        ('include/field_system.h', 'GearPhoneRingManager'),
    ):
        types.append(declaration((root / relative).read_text(), name))
        inputs.add(root / relative)
    inputs.update(root / relative for relative in constructor_sources)
    write_text_if_changed(out / 'original_field_constructor_types.inc', '\n'.join(types) + '\n')
    # Required original new-game preparation: synchronous map-load script owner.
    # All unimplemented command slots reject; source table identities are kept.
    script_sources = {
        'src/script_manager.c': ('RunInitScript', 'StartMapLoadScript', 'CreateScriptContext',
            'DestroyScriptContext', 'SetUpScriptContextForMap', 'LoadScriptsAndMessagesByMapId',
            'LoadScriptsAndMessagesParameterized', 'LoadScriptsAndMessagesForCurrentMap',
            'LoadScriptsForCurrentMap', 'GetCurrentMapMessageBank', 'ScriptRunByIndex', 'FieldSystem_FlagSet'),
        'src/scrcmd_c.c': ('ScrCmd_SetFlag', 'ScrCmd_End'),
        'src/scrcmd_lottery.c': ('ScrCmd_LotoIDSet',),
    }
    script_code,script_records=functions(root,script_sources)
    vm=(root/'src/script.c').read_text()
    vm_names=re.findall(r'^(?:[\w*]+[ \t]+)+\**([A-Za-z_]\w*)\s*\([^;{}]*\)\s*\{',vm,re.M)
    aliases=[name for selected in script_sources.values() for name in selected]+vm_names
    aliases += ['gScriptCmdTable','sNumScriptCmds','sScriptBankMapping']
    write_text_if_changed(out/'original_field_scripts_names.h',
        '#pragma once\n'+''.join(f'#define {name} hg_original_{name}\n' for name in aliases))
    script_manager=(root/'src/script_manager.c').read_text()
    mapping=re.findall(r'^const struct ScriptBankMapping sScriptBankMapping\[30\] = \{.*?^\};',script_manager,re.M|re.S)
    if len(mapping)!=1:raise ValueError('original script bank mapping')
    commands=re.findall(r'^    (\w+),$',(root/'src/data/fieldmap/script_cmd_table.h').read_text(),re.M)
    supported={'ScrCmd_SetFlag','ScrCmd_End','ScrCmd_LotoIDSet'}
    table='static const ScrCmdFunc gScriptCmdTable[] = {\n'+''.join(
        '    '+(name if name in supported else 'unsupported_original_command')+',\n' for name in commands)+'};\n'
    # Prototypes precede the table; function bodies follow its declaration.
    at=script_code.index('#line')
    write_text_if_changed(out/'original_field_scripts.inc',script_code[:at]+declaration(script_manager,'ScriptBankMapping')+'\n'+mapping[0]+'\n'+table+
        'static const u32 sNumScriptCmds=NELEMS(gScriptCmdTable);\n'+script_code[at:])
    write_text_if_changed(out/'original_field_scripts_provenance.json',json.dumps(dict(
        functions=script_records,vm_source='src/script.c',vm_sha256=hashlib.sha256(vm.encode()).hexdigest(),
        command_slots=len(commands),implemented_handlers=sorted(supported),
        boundary='original synchronous init script only; unsupported commands fail visibly'),indent=2)+'\n')
    inputs.update(root/relative for relative in script_sources)
    inputs.update((root/'src/script.c',root/'src/data/fieldmap/script_cmd_table.h'))
    # Native-safe field entry resource closure: no guest object pointers.
    entry_sources = {
        'src/field_warp_tasks.c': ('sub_02052F94',),
        'src/map_events.c': ('Field_InitMapEvents', 'MapEvents_ReadFromNarc',
            'MapEvents_ComputeRamHeader', 'WildEncounters_ReadFromNarc',
            'MapScriptHeader_ReadFromNarc', 'Field_GetWarpEventI'),
        'src/map_matrix.c': ('MapMatrix_Load', 'MapMatrix_MapMatrixData_Load'),
        'src/save_local_field_data.c': ('LocalFieldData_GetPreviousPosition',
            'LocalFieldData_GetDynamicWarp', 'LocalFieldData_GetEntrancePosition'),
        'src/location_backup.c': ('Save_SetPositionToPlayerRoom', 'Location_SetToPlayerRoom'),
    }
    entry_code, entry_records = functions(root, entry_sources)
    aliases=[name for selected in entry_sources.values() for name in selected]
    write_text_if_changed(out/'original_field_entry_names.h', '#pragma once\n'+
        ''.join(f'#define {name} hg_original_{name}\n' for name in aliases))
    location=(root/'src/location_backup.c').read_text()
    location_table=re.findall(r'^static const Location sLocation_PlayerRoom = \{.*?^\};',location,re.M|re.S)
    if len(location_table)!=1: raise ValueError('original starting location')
    write_text_if_changed(out/'original_field_entry.inc',location_table[0]+'\n'+entry_code)
    write_text_if_changed(out/'original_field_entry_provenance.json',json.dumps(dict(functions=entry_records,starting_location_sha256=hashlib.sha256(location_table[0].encode()).hexdigest(),boundary='native original event/location and matrix loading; terrain dispatch/object callbacks remain open'),indent=2)+'\n')
    inputs.update(root/relative for relative in entry_sources)
    # Coherent original map-mode/terrain entry owner; included separately so its
    # pointer-bearing owner can never leak into the legacy viewer translation unit.
    terrain_sources = {
        'src/field_warp_tasks.c': ('sub_02052F30', 'sub_02053284', 'sub_02053324'),
        'src/map_matrix.c': ('RemoveMahoganyTownAntennaTree', 'SetLakeOfRageWaterLevel', 'PlaceSafariZoneAreas'),
        'src/sys_vars.c': ('sub_02066C00', 'sub_02066C74', 'GetScriptVar'),
        'src/terrain_attributes.c': ('TerrainAttributes_New', 'TerrainAttributes_Free',
            'TerrainAttributes_Get', 'TerrainAttributes_Load', 'TerrainAttributes_GetOrPushLandDataID'),
    }
    terrain_code, terrain_records = functions(root, terrain_sources)
    terrain_names = [name for selected in terrain_sources.values() for name in selected]
    write_text_if_changed(out/'original_field_terrain_names.h', '#pragma once\n'+
        ''.join(f'#define {name} hg_original_{name}\n' for name in terrain_names))
    warp = (root/'src/field_warp_tasks.c').read_text()
    modes = re.findall(r'^static const struct MapLoadMode sMapLoadModes\[\] = \{.*?^\};', warp, re.M|re.S)
    sysvars = (root/'src/sys_vars.c').read_text()
    flag_table = re.findall(r'^static const u16 _020FE4A8\[\] = \{.*?^\};', sysvars, re.M|re.S)
    if len(modes) != 1 or len(flag_table) != 1:
        raise ValueError('original map-mode/terrain flag table ambiguous')
    terrain_header = (root/'include/terrain_attributes.h').read_text()
    defines = re.findall(r'^#define (?:MAP_TILES_COUNT_[XZ]|TERRAIN_ATTRIBUTES_\w+)[^\n]*', terrain_header, re.M)
    safari = (root/'include/safari_zone.h').read_text()
    terrain_types = [declaration((root/'include/script.h').read_text(), 'MapLoadMode'),
        *defines, declaration(terrain_header, 'TerrainAttributes')]
    # Safari's matrix mutation consumes these pointer-free original records.
    terrain_types.append('#ifndef POKEHEARTGOLD_SAFARI_ZONE_H')
    terrain_types.extend(declaration(safari, name) for name in
        ('SAFARIZONE_OBJECT', 'SAFARIZONE_AREA', 'SafariZoneAreaSet', 'SAFARIZONE_LINKLEADER', 'SafariZone'))
    terrain_types.append('#endif')
    terrain_prefix = '\n'.join([
        '#include "constants/field/map_load.h"', '#include "constants/safari.h"',
        '#include "files/fielddata/mapmatrix/map_matrix.naix"',
        '#include "constants/vars.h"', *terrain_types,
        'SafariZone *Save_SafariZone_Get(SaveData *);',
        'SafariZoneAreaSet *SafariZone_GetAreaSet(SafariZone *, s32);',
        'void sub_0205489C(void **, BOOL);',
        'void NARC_ReadFromMember(NARC *, u32, u32, u32, void *);',
        modes[0], flag_table[0]])
    write_text_if_changed(out/'original_field_terrain.inc', terrain_prefix+'\n'+terrain_code)
    write_text_if_changed(out/'original_field_terrain_provenance.json', json.dumps(dict(
        functions=terrain_records,
        map_load_modes_sha256=hashlib.sha256(modes[0].encode()).hexdigest(),
        flag_table_sha256=hashlib.sha256(flag_table[0].encode()).hexdigest(),
        boundary='native original map mode/matrix/terrain owner; terrain callback installer is an explicit external ABI bridge'), indent=2)+'\n')
    # Canonical transition owner and pointer-free save mutations. Assembly
    # callees retain a separate, checked guest projection in the test harness.
    transition_sources = {
        'src/field_warp_tasks.c': ('sub_02053038',),
        'include/math_util.h': ('LCRandRange',),
        'src/field_bgm.c': ('FieldBGM_ClearOverride',),
        'src/script_manager.c': ('ClearTempFieldEventData', 'TryStartMapScriptByType', 'GetMapLoadScriptId'),
        'src/map_events.c': ('MapEvents_GetScriptHeader',),
        'src/map_matrix.c': ('ShouldUseAlternateLakeOfRage', 'MapAndDayCheck'),
        'src/field_system_rtc_weather.c': ('FieldSystem_GetWeather_HandleDiamondDust',),
        'src/unk_02055418.c': ('FieldSystem_HasPenalty',),
        'src/save_local_field_data.c': ('LocalFieldData_GetMusicIdAddr', 'LocalFieldData_GetWeatherType', 'LocalFieldData_SetWeatherType', 'LocalFieldData_GetCameraType', 'LocalFieldData_SetCameraType', 'LocalFieldData_SetBlackoutSpawn', 'LocalFieldData_GetPlayer'),
        'src/sys_flags.c': ('ClearFlag972', 'StrengthFlagAction', 'FlagAction', 'SysFlagFlashClear', 'SysFlagDefogClear', 'SysFlagFlashCheck', 'SysFlagDefogCheck', 'ClearFlag99A', 'Save_VarsFlags_FlypointFlagAction', 'Save_VarsFlags_CheckSafariSysFlag'),
        'src/sys_vars.c': ('sub_02066C4C', 'sub_02066C1C'),
        'src/save_follow_mon.c': ('Save_FollowMon_Get', 'Save_FollowMon_SetInhibitFlagState'),
    }
    # Traverse both original roamer branches to save/math leaves in one pass.
    roamer_defs = {}
    for relative in ('src/roamer.c', 'src/field_roamer.c'):
        for m in re.finditer(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}', (root/relative).read_text(), re.M|re.S):
            roamer_defs[m[1]] = (relative, m[0])
    selected=set(); pending=['RoamerSave_SetFlute','UpdatePlayerLocationHistoryIfAnyRoamersActive','Save_UpdateRoamersLocation']
    while pending:
        name=pending.pop()
        if name in selected or name not in roamer_defs: continue
        selected.add(name)
        pending.extend(set(re.findall(r'\b(\w+)\s*\(', roamer_defs[name][1]))-selected)
    for relative in ('src/roamer.c', 'src/field_roamer.c'):
        transition_sources[relative] = tuple(sorted(n for n in selected if roamer_defs[n][0]==relative))
    transition_code, transition_records = functions(root, transition_sources)
    transition_names = [n for ns in transition_sources.values() for n in ns]
    write_text_if_changed(out/'original_field_transition_names.h', '#pragma once\n'+''.join(f'#define {n} hg_original_{n}\n' for n in transition_names))
    weather=(root/'src/field_system_rtc_weather.c').read_text()
    dust=re.search(r'^const MonthDay DiamondDustDates\[\] = \{.*?^\};',weather,re.M|re.S)[0]
    roamer_source=(root/'src/field_roamer.c').read_text()
    roamer_tables='\n'.join(re.findall(r'^static const .*?^\};', roamer_source,re.M|re.S))
    roamer_types=re.search(r'^struct RoamerAdjacency \{.*?^\};',roamer_source,re.M|re.S)[0]
    write_text_if_changed(out/'original_field_transition.inc', '#include \"constants/roamer.h\"\n#define MAX_ROAM_NEIGHBOR 6\n'+roamer_types+'\n'+roamer_tables+'\n'+declaration((root/'include/sav_system_info.h').read_text(), 'SysInfo_RTC')+'\nSysInfo_RTC *Save_SysInfo_RTC_Get(SaveData *);\nBOOL Save_SysInfo_RTC_HasPenalty(SysInfo_RTC *);\n'+dust+'\n'+transition_code)
    write_text_if_changed(out/'original_field_transition_provenance.json', json.dumps(transition_records,indent=2)+'\n')
    inputs.update(root/relative for relative in transition_sources)
    from guest_assembly import assemble
    import shutil
    transition_asm=(root/'asm/unk_02067A60.s').read_text()
    raw=(root/'asm/unk_0203BA5C.s').read_text()+'\n.text\n'
    for name in ('sub_02067A88','sub_02067AE4'):
        matches=re.findall(r'\tthumb_func_start '+name+r'\n.*?\tthumb_func_end '+name,transition_asm,re.S)
        if len(matches)!=1: raise ValueError(('transition assembly drift',name))
        raw+=matches[0]+'\n'
    held=(root/'asm/unk_02062108.s').read_text()
    raw+=re.search(r'\tthumb_func_start MapObject_ClearHeldMovement\n.*?\tthumb_func_end MapObject_ClearHeldMovement',held,re.S)[0]+'\n'
    inputs.add(root/'asm/unk_02062108.s')
    reset=(root/'asm/unk_0205FD20.s').read_text()
    raw+=reset+'\n'
    inputs.add(root/'asm/unk_0205FD20.s')
    raw+='\n.text\n'
    types=(root/'asm/unk_020632B0.s').read_text()
    for name in ('sub_02063A14','sub_02063A40','sub_02063AFC','sub_02063A1C','sub_02063A5C','sub_02063A78','sub_02063B00','sub_02063B04'):
        raw+=re.search(r'\tthumb_func_start '+name+r'\n.*?\tthumb_func_end '+name,types,re.S)[0]+'\n'
    for table in ('_020FE104','_020FE134','_020FE164'):
        raw+='\n.data\n'+re.search(r'^'+table+r':\n(?:\t.word [^\n]+\n){12}',types,re.M)[0]+'\n'
    inputs.add(root/'asm/unk_020632B0.s')
    follower_asm_path=root/'asm/unk_020658D4.s'
    raw+='\n.text\n'+follower_asm_path.read_text()+'\n'
    inputs.add(follower_asm_path)
    restore_path=root/'asm/unk_02055244.s'
    raw+='\n.text\n'+restore_path.read_text()+'\n'
    inputs.add(restore_path)
    assemble(raw,root,out,Path(shutil.which('clang')),'original_transition_guest','HG_TRANSITION')
    inputs.update((root/'asm/unk_02067A60.s',root/'asm/unk_0203BA5C.s'))
    guest_sources=('asm/unk_0205FD20.s','asm/unk_020658D4.s','asm/unk_02055244.s','asm/unk_020632B0.s','asm/unk_02062108.s')
    write_text_if_changed(out/'original_field_object_guest_provenance.json',json.dumps(dict(
        sources={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in guest_sources},
        whole_units=guest_sources[:3],
        selected_type_functions=('sub_02063A14','sub_02063A40','sub_02063AFC','sub_02063A1C','sub_02063A5C','sub_02063A78','sub_02063B00','sub_02063B04'),
        selected_held_function='MapObject_ClearHeldMovement',
        open_boundary='unprovided imports and pointer-bearing object graphs remain named rejecting imports'),indent=2)+'\n')


    # Begin the actual original object/avatar owner, with callback admission
    # retained as an explicit next boundary rather than a successful surrogate.
    object_sources={'src/field_warp_tasks.c': ('sub_0205316C',),
        'src/map_events.c': ('Field_InitMapObjectsFromZoneEventData',),
        'src/script_manager.c': ('FieldSystem_FlagCheck',),
        'src/map_object.c': ('sub_0205F1A0','MapObjectManager_SetMapModelNarc','MapObjectManager_GetMapModelNarc','MapObjectManager_SetFlagsBits','sub_0205F5D4','MapObjectManager_Init','MapObjectManager_New','MapObjectManager_SetFieldSystem','MapObjectManager_SetObjectCount','MapObjectManager_SetPriority','MapObjectManager_SetObjects','MapObjectManager_GetObjects','MapObjectManager_Delete')}
    avatar_source=(root/'src/player_avatar.c').read_text()
    avatar_defs={m[1]:m[0] for m in re.finditer(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}',avatar_source,re.M|re.S)}
    selected=set();pending=['PlayerAvatar_InitMapFeatures','PlayerAvatar_CreateWithParams','PlayerAvatar_DeleteFromMap','MapObjectManager_GetFirstActiveObjectWithMovement1','PlayerAvatar_GetXCoord','PlayerAvatar_GetZCoord']
    while pending:
        name=pending.pop()
        if name in selected or name not in avatar_defs:continue
        selected.add(name);pending.extend(set(re.findall(r'\b(\w+)\s*\(',avatar_defs[name]))-selected)
    object_sources['src/player_avatar.c']=tuple(sorted(selected))
    follow_source=(root/'src/follow_mon.c').read_text()
    follow_defs={m[1]:m[0] for m in re.finditer(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}',follow_source,re.M|re.S)}
    follow_selected=set();pending=['FollowMon_InitMapObject']
    while pending:
        name=pending.pop()
        if name in follow_selected or name not in follow_defs: continue
        follow_selected.add(name);pending.extend(set(re.findall(r'\b(\w+)\s*\(',follow_defs[name]))-follow_selected)
    follow_code,follow_records=functions(root,{'src/follow_mon.c':tuple(sorted(follow_selected)),
        'src/save_follow_mon.c':('Save_FollowMon_SetUnused2bitField','Save_FollowMon_GetInhibitFlagState'),
        'src/party.c':('SaveArray_Party_Get',)})
    tables='\n'.join(re.findall(r'^static const u16 \w+\[\] = \{.*?^\};',follow_source,re.M|re.S))
    declarations=[]
    for header,names in {'pokemon.h':('GetMonData','SetMonData','MonIsShiny','AllocMonZeroed','CreateMon'),
                         'script_pokemon_util.h':('CountAlivePokemon','GetFirstNonEggInParty','GetFirstAliveMonInParty_CrashIfNone')}.items():
        header_source=(root/'include'/header).read_text()
        for name in names:
            matches=re.findall(r'^[^\n;{}]+\b'+name+r'\([^;{}]*\);',header_source,re.M)
            if len(matches)!=1: raise ValueError(('follower external prototype',name))
            declarations+=matches
        inputs.add(root/'include'/header)
    write_text_if_changed(out/'original_field_follow.inc','\n'.join(declarations)+'\n'+tables+'\n'+follow_code)
    follow_names=sorted(follow_selected|{'Save_FollowMon_SetUnused2bitField','Save_FollowMon_GetInhibitFlagState','SaveArray_Party_Get'})
    write_text_if_changed(out/'original_field_follow_names.h','#pragma once\n'+''.join(f'#define {n} hg_original_{n}\n' for n in follow_names))
    write_text_if_changed(out/'original_field_follow_provenance.json',json.dumps(dict(functions=follow_records,tables_sha256=hashlib.sha256(tables.encode()).hexdigest()),indent=2)+'\n')
    inputs.update((root/'src/follow_mon.c',root/'src/save_follow_mon.c',root/'src/party.c'))
    object_source=(root/'src/map_object.c').read_text()
    object_defs={m[1]:m[0] for m in re.finditer(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}',object_source,re.M|re.S)}
    object_cuts={'sub_0205FB00','sub_0205FC94','FieldSystem_VarGetObjectEventGraphicsId'}
    # Scalar native accessors used by the original ARM autonomous owner. Pointer
    # arguments/results remain explicit typed bridges, never implicit casts.
    object_imports=[]; scalar_roots=[]
    assembly_calls=set(re.findall(r'\bbl\s+(\w+)',raw))
    assembly_calls.update(('MapObject_GetPriorityPlusValue','MapObject_GetID','MapObject_GetMapID'))
    graphics_assembly='\n'.join(p.read_text() for p in (root/'asm').glob('overlay_01_*.s'))
    assembly_calls.update(re.findall(r'\bbl\s+(\w+)',graphics_assembly))
    scalar_types={'u32','s32','int','BOOL','u8','u16','s16','s8','void','MapObjectFlagBits'}
    for name in sorted(assembly_calls&object_defs.keys()):
        if name in object_cuts:continue
        signature=object_defs[name].split(' {',1)[0]
        m=re.fullmatch(r'(?:static )?(\w+) '+name+r'\((?:LocalMapObject \*\w+)(?:, (\w+) \w+)?\)',signature)
        if not m or m[1] not in scalar_types or (m[2] and m[2] not in scalar_types):continue
        call=name+'(object'+(',cpu->r[1]' if m[2] else '')+')'
        object_imports.append('if(!strcmp(name,"'+name+'")){'+('cpu->r[0]=' if m[1]!='void' else '')+call+';return 1;}')
        scalar_roots.append(name)
    vector_imports=[]
    for name,body in sorted(object_defs.items()):
        if not re.fullmatch(r'(?:static )?void '+name+r'\(LocalMapObject \*\w+, (?:const )?VecFx32 \*\w+\)',body.split(' {',1)[0]):continue
        vector_imports.append('if(!strcmp(name,"'+name+'")){VecFx32 *v=ds_translate(&cpu->memory,cpu->r[1],sizeof(*v));if(!v)return -1;'+name+'(object,v);return 1;}')
        scalar_roots.append(name)
    write_text_if_changed(out/'original_field_object_vectors.inc','\n'.join(vector_imports)+'\n')
    write_text_if_changed(out/'original_field_object_imports.inc','\n'.join(object_imports)+'\n')
    object_selected=set();pending=list(object_sources['src/map_object.c'])+['MapObject_Create','sub_0205FCB4','sub_0205FCB8','sub_0205FCBC','sub_0205FCC0','sub_0205F41C','sub_0205F340','sub_0205F12C','MapObject_GetFieldSystem','MapObject_Remove','sub_0205F370','sub_0205F394']
    pending+=scalar_roots
    zone_source=(root/'src/map_events.c').read_text()
    zone_body=re.search(r'^void Field_InitMapObjectsFromZoneEventData\([^;{}]*\) \{.*?^\}',zone_source,re.M|re.S)[0]
    pending+=list(set(re.findall(r'\b(\w+)\s*\(',follow_code+zone_body))&object_defs.keys())
    pending+=['sub_0205F568','sub_0205E520','sub_0205E580','sub_0205ED18','sub_0205F370','sub_0205F398','sub_0205F3C0','sub_0205F3E8']
    while pending:
        name=pending.pop()
        if name in object_selected or name in object_cuts or name not in object_defs:continue
        object_selected.add(name);pending.extend(set(re.findall(r'\b(\w+)\s*\(',object_defs[name]))-object_selected)
    object_sources['src/map_object.c']=tuple(sorted(object_selected))

    object_code,object_records=functions(root,object_sources)
    names=[n for ns in object_sources.values() for n in ns]
    write_text_if_changed(out/'original_field_objects_names.h','#pragma once\n'+''.join(f'#define {n} hg_original_{n}\n' for n in names))
    movement_data=(root/'asm/unk_data_020FCBD8.s').read_text()
    movement_table=re.search(r'^_020FD1F4:\n((?:\t.word [^\n]+\n){57})',movement_data,re.M)
    if not movement_table: raise ValueError('movement table drift')
    targets=re.findall(r'\t.word (\w+)',movement_table[1])
    descriptor=re.search(r'^'+targets[1]+r':\n((?:\t.word [^\n]+\n){5})',movement_data,re.M)
    if not descriptor: raise ValueError('player movement descriptor drift')
    values=re.findall(r'\t.word (\w+)',descriptor[1])
    if values!=['0x00000001','sub_0205FCB4','sub_0205FCB8','sub_0205FCBC','sub_0205FCC0']: raise ValueError('player callbacks drift')
    object_code+='\nstatic UnkLMOCallbackStruct canonical_player_movement = {'+','.join(values)+'};\n'
    descriptor48=re.search(r'^'+targets[48]+r':\n((?:\t.word [^\n]+\n){5})',movement_data,re.M)
    if not descriptor48:raise ValueError('follower movement descriptor drift')
    follower_values=re.findall(r'\t.word (\w+)',descriptor48[1])
    if follower_values!=['0x00000003','sub_020658D4','sub_02065900','sub_02065998','sub_0205FCC0']:raise ValueError('follower callbacks drift')
    native_values=[follower_values[0]]
    for target in follower_values[1:]:
        if target in object_selected:native_values.append(target)
        else:
            wrapper='canonical_guest_'+target
            object_code+='static void '+wrapper+'(LocalMapObject *object){canonical_object_arm(object,HG_TRANSITION_'+target+');}\n'
            native_values.append(wrapper)
    object_code+='static UnkLMOCallbackStruct canonical_follower_movement = {'+','.join(native_values)+'};\n'
    object_records.append(dict(boundary='movement 48 native trampolines invoke original ARM callbacks; work storage stays on the original native object',source='asm/unk_data_020FCBD8.s',descriptor=follower_values))

    object_records.append(dict(boundary='movement 1 descriptor projected into native callback pointers; other movements explicitly reject',source='asm/unk_data_020FCBD8.s',table=targets,descriptor=values))
    inputs.add(root/'asm/unk_data_020FCBD8.s')
    write_text_if_changed(out/'original_field_objects.inc',object_code)
    write_text_if_changed(out/'original_field_objects_provenance.json',json.dumps(object_records,indent=2)+'\n')
    inputs.update(root/relative for relative in object_sources)
    parent_code, parent_records = functions(root, {'src/field_warp_tasks.c': ('FieldTask_NewGame', 'CallFieldTask_NewGame')})
    write_text_if_changed(out/'original_field_newgame.inc', parent_code)
    write_text_if_changed(out/'original_field_newgame_provenance.json', json.dumps(parent_records,indent=2)+'\n')
    inputs.update(root/relative for relative in terrain_sources)
    inputs.update(root/relative for relative in ('include/script.h', 'include/terrain_attributes.h',
        'include/safari_zone.h', 'include/constants/safari.h', 'include/constants/field/map_load.h', 'include/constants/vars.h'))
    from prepare_original_parent import prepare_parent
    inputs.update(prepare_parent(root,out,functions,declaration))
    modules = re.findall(r'^Overlay\s+(\w+)', (root / 'main.lsf').read_text(), re.M)
    if len(modules) != len(set(modules)):
        raise ValueError('ambiguous module identities')
    write_text_if_changed(out / 'original_field_modules.h',
                          '#pragma once\n#define FS_OVERLAY_ID(name) HG_ORIGINAL_MODULE_##name\n' +
                          '\n'.join(f'#define HG_ORIGINAL_MODULE_{name} {modules.index(name)}'
                                    for name in ('field', 'ds_protect', 'OVY_124','OVY_2','OVY_3','OVY_4')) + '\n')
    inputs.add(root / 'main.lsf')
    write_text_if_changed(out / 'original_field_inputs.cmake',
                          'set(HG_ORIGINAL_FIELD_INPUTS\n' +
                          ''.join(f'  "{p.resolve()}"\n' for p in sorted(inputs)) + ')\n')


if __name__ == '__main__':
    prepare(*map(Path, sys.argv[1:]))
