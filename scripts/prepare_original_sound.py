"""Original ARM sound policy and work layout, with explicit SDK imports.

This compiles Nintendo's owners; it does not admit an unimplemented sound device,
initialize a fake SDK heap, or substitute the host sequence player for SDK APIs.
"""
import hashlib
from pathlib import Path
import re
import shlex
import subprocess
from generated_files import write_text_if_changed
from prepare_original_2d import sdk_paths, prepare_headers, compiler_flags
from prepare_original_field import functions


def generate_owners(sdk, nns, out, cc, include):
    """Original ARM32 owners. Unprovided driver commands remain imports."""
    import json
    assembly, records, dependencies = [], [], []
    units = ('snd/capture', 'snd/fader', 'snd/heap', 'snd/player',
             'snd/resource_mgr', 'snd/sndarc', 'snd/sndarc_loader', 'snd/sndarc_player',
             'snd/bank', 'snd/command_fence', 'snd/interface', 'snd/work',
             'arm7/command', 'arm7/seq', 'arm7/exchannel', 'arm7/work',
             'arm7/util', 'arm7/data', 'arm7/bank', 'arm7/main', 'arm7/message',
             'fnd/list', 'fnd/frameheap', 'fnd/heapcommon')
    for unit in units:
        label = unit.replace('/', '_')
        path = (
            sdk / 'libraries/snd/src/snd_command.c'
            if unit == 'snd/command_fence' else
            sdk / 'libraries/snd/src/snd_bank.c'
            if unit == 'snd/bank' else
            sdk / 'libraries/snd/src/snd_interface.c'
            if unit == 'snd/interface' else
            nns / ('libraries/' + unit.split('/')[0] + '/src/' + unit.split('/')[1] + '.c')
        )
        arm7 = unit.startswith('arm7/')
        if arm7:
            name = unit.split('/')[1]
            path = sdk / ('libraries/os/src/os_message.c' if name == 'message' else
                          'libraries/snd/src/ARM7/snd_' + name + '.c' if name in ('seq', 'exchannel') else
                          'libraries/snd/src/snd_' + name + '.c')
        if unit == 'snd/work':
            path = sdk / 'libraries/snd/src/snd_work.c'
        original = path.read_text()
        emitted = original
        if unit == 'snd/interface':
            names = ('SND_PrepareSeq', 'SND_StartPreparedSeq', 'SND_SetTrackAllocatableChannel',
                         'SND_SetPlayerChannelPriority', 'SNDi_SetPlayerParam', 'PushCommand_impl')
            prefix = original[:original.index('void SND_StartSeq')]
            bodies = []
            for name in names:
                pattern = r'^(?:    )?(?:static )?[\w* \t]+\b' + name + r' \([^;{}]*\)\n(?P<indent>    )?\{.*?^(?(indent)    )\}'
                matches = list(re.finditer(pattern, original, re.M | re.S))
                if len(matches) != 1: raise ValueError(('sound command definition drift', name))
                bodies.append(matches[0][0])
            emitted = prefix + '\n'.join(bodies) + '\n'
        if unit == 'snd/command_fence':
            names = tuple(re.findall(r'^\w[\w* ]+\b(SND_\w+) \(', original, re.M))
        if unit == 'snd/heap':
            emitted = emitted.replace('void NNS_SndHeapLoadState (', 'void hg_sdk_SndHeapLoadState (')
            emitted += '''
extern void hg_sound_invalid_heap_level(void);
void NNS_SndHeapLoadState(NNSSndHeapHandle heap,int level) {
    if(level < 0 || level > NNS_SndHeapGetCurrentLevel(heap)) { hg_sound_invalid_heap_level(); return; }
    hg_sdk_SndHeapLoadState(heap,level);
}
'''
        if unit == 'snd/player':
            # This SDK weak fallback only prints a warning. Keep its caller,
            # but require the actual device command instead of false success.
            emitted, count = re.subn(r'SDK_WEAK_SYMBOL void SND_SetTrackMuteEx \(.*?^\}', '', emitted, flags=re.M | re.S)
            if count != 1: raise ValueError('SDK weak mute fallback drift')
        source, target, dep = (out / ('original_sound_' + label + '.' + ext) for ext in ('c', 's', 'd'))
        checks = ''
        if unit == 'snd/player':
            checks = '\n_Static_assert(sizeof(NNSSndHandle)==4 && sizeof(NNSSndSeqPlayer)==68 && sizeof(NNSSndPlayer)==36,"ARM32 sound player ABI");\n'
        if unit == 'arm7/seq':
            checks = '\n_Static_assert(sizeof(SNDPlayer)==36 && sizeof(SNDTrack)==64 && __builtin_offsetof(SNDWork,player)==1344 && __builtin_offsetof(SNDWork,track)==1920 && __builtin_offsetof(SNDTrack,cur)==40,"ARM7 sequence ABI");\n'
        if unit == 'arm7/message':
            checks = '\n_Static_assert(sizeof(OSMessageQueue)==32 && __builtin_offsetof(OSMessageQueue,msgCount)==20 && __builtin_offsetof(OSMessageQueue,usedCount)==28,"ARM7 message ABI");\n'
        if unit == 'snd/command_fence':
            checks = '\n_Static_assert(sizeof(SNDCommand)==24 && __builtin_offsetof(SNDCommand,arg)==8 && SND_COMMAND_NUM==256 && SND_COMMAND_READ_DRIVER_INFO==33,"ARM32 command ABI");\n'
        if unit == 'snd/capture':
            checks = '\n_Static_assert(sizeof(CaptureParam)==84 && __builtin_offsetof(CaptureParam,fader)==60,"ARM32 capture ABI");\n'
        write_text_if_changed(source, 'extern int abs(int);\n' + emitted + checks)
        flags = compiler_flags(include, sdk, nns)
        if arm7:
            flags = [f.replace('-DSDK_ARM9', '-DSDK_ARM7').replace('-mcpu=arm946e-s', '-mcpu=arm7tdmi') for f in flags]
        subprocess.run([str(cc), *flags,
                        '-I' + str(nns / 'libraries/fnd/include'),
                        '-MMD', '-MF', str(dep), '-S', str(source), '-o', str(target)], check=True)
        text = target.read_text()
        # HG_DOTTED_SDK_STATIC_FIX
        # Clang gives this function-local SDK static a dotted assembly
        # symbol. Normalize its identity before local-symbol namespacing.
        if unit == 'snd/sndarc_loader':
            original_symbol = 'LoadWaveArcTable.waveArcHeader'
            normalized_symbol = 'LoadWaveArcTable_waveArcHeader'
            if original_symbol not in text:
                raise ValueError('Expected SDK static symbol not found')
            if normalized_symbol in text:
                raise ValueError('SDK static symbol collision')
            text = text.replace(original_symbol, normalized_symbol)
        # Normalize Clang's dotted function-local static symbols before
        # applying the existing per-translation-unit symbol renaming.
        # These remain real SDK statics and must enter the state inventory.
        dotted = set(re.findall(
            r'^\s*\.type\s+([A-Za-z_]\w*(?:\.[A-Za-z_]\w*)+)\s*,\s*%object\b',
            text, re.M))
        for symbol in sorted(dotted, key=len, reverse=True):
            normalized = symbol.replace('.', '_')
            if normalized in text:
                raise ValueError(("sound static name collision", symbol))
            text = text.replace(symbol, normalized)
        if arm7:
            # Distinct CPU software identities, including unimplemented imports;
            # an ARM7 call must never resolve to an ARM9 command producer.
            text = re.sub(r'\b(SNDi?_[A-Za-z0-9_]+)\b', r'arm7_\1', text)
        # Static functions and records from separate original translation units
        # must remain separate in the combined guest assembler input.
        globals_ = set(re.findall(r'^\s*\.globl\s+(\w+)', text, re.M))
        locals_ = set(re.findall(r'^\s*\.type\s+(\w+),', text, re.M)) - globals_
        for name in sorted(locals_, key=len, reverse=True):
            text = re.sub(r'\b' + re.escape(name) + r'\b', 'hg_sound_' + label + '_' + name, text)
        text = re.sub(r'\.L([\w.$]+)', '.Lsound_' + label + r'_\1', text)
        assembly.append(text)
        dependencies += [path] + [Path(p) for p in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]) if Path(p) != source]
        bodies = re.findall(r'^[\w* \t]+\b(\w+)\s*\([^;{}]*\)\s*\{.*?^\}', emitted, re.M | re.S)
        records.append(dict(target_cpu='ARM7TDMI' if arm7 else 'ARM946E-S',
                            source=str(path), sha256=hashlib.sha256(original.encode()).hexdigest(), functions=bodies,
                            adaptation='ARM32 original unit; private symbols retain translation-unit identity; weak unsupported mute fallback excluded' if unit == 'snd/player' else 'ARM32 original unit; private symbols retain translation-unit identity'))
        if unit in ('snd/command_fence','snd/interface'):
            records[-1]['functions'] = list(names)
            records[-1]['admission'] = 'original ARM9 queue/protocol; native PXI delivery to original ARM7 software; active device imports remain explicit'


    write_text_if_changed(out / 'original_sound_owners_provenance.json', json.dumps(records, indent=2) + '\n')
    return '\n'.join(assembly), dependencies


def write_state_inventory(out, assembly):
    """Exact compiled static storage, used for incomplete scene admission rollback."""
    all_objects = set(re.findall(r'^\s*\.type\s+([\w.$]+),%object', assembly, re.M))
    # Only compiler literals and switch tables may be excluded. In particular,
    # a new function-local static (foo.initialized) must not disappear silently.
    unknown = {n for n in all_objects if not re.fullmatch(r'\w+', n)
               and not (n.startswith('.Lsound_') and ('.str' in n or 'switch.table.' in n))}
    if unknown: raise ValueError(('unsupported sound static identity', sorted(unknown)))
    objects = set(re.findall(r'^\s*\.type\s+(\w+),%object', assembly, re.M))
    sizes = dict((name, int(size)) for name, size in re.findall(
        r'^\s*\.(?:comm|size)\s+(\w+),\s*(\d+)\b', assembly, re.M) if name in objects)
    if objects - sizes.keys(): raise ValueError(('sound storage size missing', sorted(objects - sizes.keys())))
    write_text_if_changed(out / 'original_sound_state.inc',
        '#define HG_SOUND_STATIC_BYTES ' + str(sum(sizes.values())) + '\n' +
        'static const struct {uint32_t address;unsigned size;} sound_static_regions[]={\n' +
        ''.join('{HG_LOADER_' + name + ',' + str(size) + '},\n' for name, size in sorted(sizes.items())) + '};\n')


def generate_channels(sdk,nns,out):
    """Native private allocator records, never an ARM32 SDK layout cast."""
    header=sdk/'include/nitro/snd/common/exchannel.h'
    path=sdk/'libraries/snd/src/ARM7/snd_exchannel.c'
    h=header.read_text();source=path.read_text()
    names=('SND_AllocExChannel','SND_FreeExChannel','SND_LockChannel','SND_UnlockChannel',
           'SND_GetLockedChannel','InitAllocExChannel','CompareExChannelVolume','CalcRelease',
           'SND_SetExChannelAttack','SND_SetExChannelDecay','SND_SetExChannelSustain',
           'SND_SetExChannelRelease','SND_InitLfoParam')
    bodies=[];records=[]
    for name in names:
        found=re.findall(r'^(?:static )?[\w* ]+\b'+name+r'\([^;{}]*\)\n\{.*?^\}',source,re.M|re.S)
        if len(found)!=1:raise ValueError(('sound channel source drift',name))
        bodies.append(found[0].removeprefix('static '));records.append(dict(function=name,source=str(path),sha256=hashlib.sha256(found[0].encode()).hexdigest()))
    types=[]
    for name in ('SNDExChannelCallbackStatus','SNDLfoTarget','SNDWaveParam','SNDLfoParam','SNDLfo','SNDExChannel'):
        matches=re.findall(r'^[ \t]*typedef (?:struct|enum) '+name+r' \{.*?^[ \t]*\} '+name+r';',h,re.M|re.S)
        if not matches:raise ValueError(('sound channel type drift',name))
        types.append(matches[0]) # first WaveParam is the original DS layout branch
        if name=='SNDLfoTarget':types.append('struct SNDExChannel;typedef void (*SNDExChannelCallback)(struct SNDExChannel *,SNDExChannelCallbackStatus,void *);')
    constants='\n'.join(re.findall(r'^#define SND_(?:EX_CHANNEL|LOCK_)[^\n]+',h+'\n'+source,re.M))
    player=nns/'libraries/snd/src/player.c'
    player_body=re.search(r'^void NNS_SndPlayerSetAllocatableChannel \(.*?^\}',player.read_text(),re.M|re.S)[0]
    records.append(dict(function='NNS_SndPlayerSetAllocatableChannel',source=str(player),sha256=hashlib.sha256(player_body.encode()).hexdigest()))
    emitted = """// Generated from pinned SDK. Private native types; no ARM consumer.
using u8=uint8_t;using s8=int8_t;using u16=uint16_t;using s16=int16_t;
using u32=uint32_t;using s32=int32_t;using BOOL=int;using SNDDuty=int;
#define TRUE 1
#define FALSE 0
#define SDK_MINMAX_ASSERT(x,lo,hi) assert((x)>=(lo)&&(x)<=(hi))
#define SDK_NULL_ASSERT(x) assert(x)
"""+constants+'\n'+'\n'.join(types)+"""
struct SoundChannels {
    SNDExChannel channel[16]{};
    u32 sLockChannel=0,sWeakLockChannel=0;
    // Native policy projection: only this original NNSSndPlayer member is
    // admitted; this record is never exported as an SDK binary layout.
    struct {u32 allocChBitFlag=0;} sPlayer[32];
    void *host=nullptr;void (*stop)(void *,unsigned)=nullptr;
    void SND_StopChannel(unsigned id,BOOL hold){assert(!hold);if(stop)stop(host,id);}
#define NNS_SND_PLAYER_NO_MAX 31
#define NNS_MINMAX_ASSERT(x,lo,hi) assert((x)>=(lo)&&(x)<=(hi))
#define SND_CHANNEL_NUM 16
#define SNDi_Work (*this)
"""+player_body+'\n'+'\n'.join(bodies)+"""
#undef SNDi_Work
#undef NNS_MINMAX_ASSERT
};
#undef SDK_MINMAX_ASSERT
#undef SDK_NULL_ASSERT
#undef TRUE
#undef FALSE
"""
    write_text_if_changed(out/'original_sound_channels.inc',emitted)
    import json
    write_text_if_changed(out/'original_sound_channels_provenance.json',json.dumps(dict(functions=records,
        representation='private native SDK records projected from existing DSP channels before allocation; widened pointers never exposed to guest',
        unproved='DSP envelope/volume and command timing refinement; native sampling is not an ARM7 execution proof'),indent=2)+'\n')
    return [header,path,player]

def generate(root, out, cc, sdk_root=None, nns_root=None):
    sdk, nns = sdk_paths(root, sdk_root, nns_root)
    include, dependencies = prepare_headers(out, sdk)
    dependencies += generate_channels(sdk,nns,out)
    owners, owner_dependencies = generate_owners(sdk,nns,out,cc,include)
    dependencies += owner_dependencies
    path = root / 'src/sound.c'
    text = path.read_text()
    names = tuple(m[1] for m in re.finditer(
        r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\) \{.*?^\}', text, re.M | re.S))
    _, records = functions(root, {'src/sound.c': names})
    checks = '''
_Static_assert(sizeof(FSFile)==0x48 && sizeof(FSFileID)==8 && FS_SEEK_SET==0 && FS_SEEK_CUR==1 && FS_SEEK_END==2, "loader ARM32 filesystem ABI");
_Static_assert(sizeof(SND_WORK)==0xBEC88, "original ARM32 sound work");
_Static_assert(__builtin_offsetof(SND_WORK, currentSeqNo)==0xBEBFC, "sound sequence ABI");
_Static_assert(__builtin_offsetof(SND_WORK, stateSaveDepth)==0xBEC0C, "sound heap state ABI");
'''
    c, s, dep = (out / ('original_sound.' + ext) for ext in ('c', 's', 'd'))
    if text.count('SND_WORK sSoundWork;') != 1: raise ValueError('sound work source drift')
    emitted = text.replace('SND_WORK sSoundWork;', 'extern SND_WORK sSoundWork;')
    write_text_if_changed(c, '#line 1 "' + str(path) + '"\n' + emitted + checks)
    subprocess.run([str(cc), *compiler_flags(include, sdk, nns),
                    '-I' + str(root / 'include'), '-I' + str(out),
                    '-MMD', '-MF', str(dep), '-S', str(c), '-o', str(s)], check=True)
    dependencies += [path, root / 'subprojects/NitroSDK.wrap', root / 'subprojects/NitroSystem.wrap']
    dependencies += [Path(p) for p in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]) if Path(p) != c]
    assembly = re.sub(r'\.L([A-Za-z0-9_.]+)', r'.Lsound_\1', s.read_text())
    for label in ('sound_radio_sys', 'sound_chatot'):
        path = root / ('src/' + label + '.c')
        names = tuple(re.findall(r'^(?:static )?[\w* ]+\b(\w+)\([^;{}]*\)\s*\{',path.read_text(),re.M))
        _, unit_records = functions(root, {'src/' + label + '.c': names})
        records += unit_records
        source, target, dep = (out / ('original_' + label + '.' + ext) for ext in ('c', 's', 'd'))
        write_text_if_changed(source, path.read_text())
        subprocess.run([str(cc), *compiler_flags(include, sdk, nns),
                        '-I' + str(root / 'include'), '-MMD', '-MF', str(dep), '-S', str(source), '-o', str(target)], check=True)
        assembly += '\n' + re.sub(r'\.L([A-Za-z0-9_.]+)', '.L' + label + r'_\1', target.read_text())
        dependencies += [path] + [Path(p) for p in shlex.split(dep.read_text().replace('\\\n', ' ').split(':', 1)[1]) if Path(p) != source]
    write_state_inventory(out, assembly + '\n' + owners)
    provenance = dict(functions=records, source_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                      compiler_target='arm-none-eabi arm946e-s Thumb',
                      adaptation='Original SND_WORK layout retained; global storage is external. Handles and metadata are mapped; archive and heap prefix remain inaccessible.',
                      admission='Original sound policy and SDK software owners. InitSoundData, archive loading, active device commands and PCM transport are not admitted; exports do not establish execution or playback.')
    return '\n.set hg_compiler_syntax, 1\n' + assembly + '\n' + owners + '\n.set hg_compiler_syntax, 0\n', provenance, sorted(set(dependencies))
