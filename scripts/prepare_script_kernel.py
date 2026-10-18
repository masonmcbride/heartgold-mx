"""Select unchanged original script decisions; never edit or copy into Git.

These are function slices, not compilation of the complete containing units.
"""
import hashlib,json,pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:])
groups={
 'src/script_manager.c':('GetVarPointer','FieldSystem_VarGet','FieldSystem_VarSet','FieldSystem_VarGetObjectEventGraphicsId','FieldSystem_FlagCheck','FieldSystem_FlagSet','FieldSystem_FlagClear','ClearTempFieldEventData','GetMapLoadScriptId','GetMapSceneScriptId'),
 'include/scrcmd.h':('ScriptGetVar','ScriptGetVarPointer'),
 'src/scrcmd_c.c':('ScrCmd_Nop','ScrCmd_End','RunPauseTimer','ScrCmd_Wait','Compare','ScrCmd_CompareLocalToLocal','ScrCmd_CompareLocalToValue','ScrCmd_CompareVarToValue','ScrCmd_CompareVarToVar','ScrCmd_GoTo','ScrCmd_Call','ScrCmd_Return','ScrCmd_GoToIf','ScrCmd_CallIf','ScrCmd_SetFlag','ScrCmd_ClearFlag','ScrCmd_CheckFlag','ScrCmd_AddVar','ScrCmd_SubVar','ScrCmd_SetVar','ScrCmd_CopyVar','ScrCmd_SetOrCopyVar','ScrCmd_GetFriendSprite','IsAllMovementFinished','ScrCmd_WaitMovement'),
 'src/scrcmd_strbuf.c':('ScrCmd_BufferPlayersName',),
 'src/sys_vars.c':('SetScriptVar','Save_VarsFlags_SetLotoId','Save_VarsFlags_RollLotoId'),
 'src/scrcmd_lottery.c':('ScrCmd_LotoIDSet',),
 'src/player_data.c':('PlayerProfile_SetVersion','PlayerProfile_Init','PlayerProfile_GetTrainerGender','PlayerProfile_TestBadgeFlag','PlayerProfile_GetNamePtr'),
 'src/party.c':('Party_GetCount',),
 'src/scrcmd_party.c':('ScrCmd_GetPartyCount',),
 'src/scrcmd_17.c':('ScrCmd_CheckBadge',),
}
lines=['/* Unchanged original function slices; not whole-source coverage. */'];records=[];declarations=[]
for relative,names in groups.items():
 path=root/relative;source=path.read_text()
 if relative=='src/scrcmd_c.c':
  match=re.search(r'^static const u8 sConditionTable\[6\]\[3\] = \{.*?^\};',source,re.M|re.S)
  if not match:raise ValueError('Original script condition table changed')
  lines.append(match[0])
 for name in names:
  matches=list(re.finditer(r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b'+name+r'\([^;{}]*\) \{\n.*?^\}',source,re.M|re.S))
  if len(matches)!=1:raise ValueError(('Original script definition changed',name,len(matches)))
  match=matches[0];line=source[:match.start()].count('\n')+1
  declarations.append(match[0].split(' {\n',1)[0]+';')
  lines.extend([f'#line {line} "{path}"',match[0]])
  records.append({'source':relative,'function':name,'line':line,'source_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(match[0].encode()).hexdigest()})
(out/'script_kernel.inc').write_text('\n'.join(lines)+'\n')
(out/'script_kernel_decls.inc').write_text('\n'.join(declarations)+'\n')
script=(root/'include/script.h').read_text()
decls=[re.search(r'typedef enum ScriptEnvField \{.*?^\} ScriptEnvField;',script,re.M|re.S)[0]]
(out/'story_types.inc').write_text('\n'.join(decls)+'\n')
commands=re.findall(r'^    (\w+),$',(root/'src/data/fieldmap/script_cmd_table.h').read_text(),re.M)
selected={name for names in groups.values() for name in names if name.startswith('ScrCmd_')}
(out/'script_core_table.inc').write_text(''.join(f'[{i}]={name},\n' for i,name in enumerate(commands) if name in selected))
whole=('src/save_vars_flags.c',)
(out/'script_kernel_provenance.json').write_text(json.dumps({'functions':records,
 'whole_c_inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in whole},
 'whole_c_boundaries':{'src/save_vars_flags.c':'src/compat/story_records.c'},
 'declaration_inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in ('include/script.h','include/player_data.h','src/data/fieldmap/script_cmd_table.h','include/save_vars_flags.h')}},indent=2)+'\n')
