"""Select unchanged original script decisions; never edit or copy into Git.

These are function slices, not compilation of the complete containing units.
"""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:])
groups={
 'src/script_manager.c':('GetVarPointer','FieldSystem_VarGet','FieldSystem_VarSet','FieldSystem_VarGetObjectEventGraphicsId','FieldSystem_FlagCheck','FieldSystem_FlagSet','FieldSystem_FlagClear','ClearTempFieldEventData','GetMapLoadScriptId','GetMapSceneScriptId'),
 'include/scrcmd.h':('ScriptGetVar','ScriptGetVarPointer'),
 'src/scrcmd_c.c':('ScrCmd_Nop','ScrCmd_End','RunPauseTimer','ScrCmd_Wait','Compare','ScrCmd_CompareLocalToLocal','ScrCmd_CompareLocalToValue','ScrCmd_CompareVarToValue','ScrCmd_CompareVarToVar','ScrCmd_GoTo','ScrCmd_Call','ScrCmd_Return','ScrCmd_GoToIf','ScrCmd_CallIf','ScrCmd_SetFlag','ScrCmd_ClearFlag','ScrCmd_CheckFlag','ScrCmd_AddVar','ScrCmd_SubVar','ScrCmd_SetVar','ScrCmd_CopyVar','ScrCmd_SetOrCopyVar','ScrCmd_GetFriendSprite','IsAllMovementFinished','ScrCmd_WaitMovement','ScrCmd_CallStd','ScrNative_WaitStd','ScrCmd_RestartCurrentScript','ScrCmd_SetStarterChoice','ScrCmd_GetStarterChoice','ScrCmd_GetMenuChoice','sub_020477C0','ScrCmd_GetPlayerGender','ScrCmd_RegisterGearNumber','ScrCmd_CheckRegisteredPhoneNumber','ScrCmd_RegisterPokegearCard','ScrCmd_804','ScrCmd_HealParty','ScrCmd_573','ScrCmd_SetPhoneCall','ScrCmd_RunPhoneCall','ScrNative_WaitApplication_DestroyTaskData','ScrCmd_RestoreOverworld'),
 'src/scrcmd_strbuf.c':('ScrCmd_BufferPlayersName','ScrCmd_BufferMonSpeciesName','ScrCmd_BufferItemName','ScrCmd_BufferItemNamePlural','ScrCmd_BufferPocketName','ScrCmd_BufferPartyMonSpeciesNameIndef'),
 'src/sys_vars.c':('SetScriptVar','GetScriptVar','Save_VarsFlags_SetLotoId','Save_VarsFlags_RollLotoId','Save_VarsFlags_SetStarter','Save_VarsFlags_GetStarter','Save_VarsFlags_SetFishingCompetitionLengthRecord','Save_VarsFlags_IsInRocketTakeover'),
 'src/scrcmd_sound.c':('ScrCmd_PlayBGM','ScrCmd_StopBGM','ScrCmd_ResetBGM','ScrCmd_TempBGM','ScrCmd_FadeOutBGM','ScrCmd_FadeInBGM','ScrNative_GetFadeTimer','ScrCmd_PlaySE','ScrCmd_StopSE','ScrCmd_WaitSE','ScrNative_SEPlaying','ScrCmd_PlayFanfare','ScrCmd_WaitFanfare','ScrNative_WaitFanfare'),
 'src/scrcmd_lottery.c':('ScrCmd_LotoIDSet',),
 'src/sys_flags.c':('SetScriptFlag','ClearScriptFlag','CheckScriptFlag','SetFlag960','CheckGameClearFlag','CheckMetBill','Save_VarsFlags_ClearHaveFollowerFlag','CheckFlag09A'),
 'src/scrcmd_items.c':('ScrCmd_HasSpaceForItem','ScrCmd_HasItem','ScrCmd_GiveItem','ScrCmd_GetItemPocket'),
 'src/scrcmd_party.c':('ScrCmd_GetPartyCount','ScrCmd_GetPartyMonSpecies','ScrCmd_GetPartyLeadAlive','ScrCmd_PartyCountNotEgg'),
 'src/scrcmd_17.c':('ScrCmd_CheckBadge','ScrCmd_GivePokedex','ScrCmd_CheckPokedex'),
}
groups['src/scrcmd_sound.c']+=('ScrCmd_PlayCry','ScrCmd_WaitCry','ScrNative_WaitCry')
groups['src/scrcmd_c.c']+=('ScrCmd_NameRival','ScrCmd_CatchingTutorial','ScrCmd_GetTrcardStars','ScrCmd_379','ScrCmd_DebugWatch','ScrCmd_148','ScrCmd_MenuInitStdGmm','sub_02041770','ScrCmd_MenuItemAdd','ScrCmd_MenuExec','sub_020478D0','sub_02047908')
groups['src/field/scrcmd_message.c']=('ScrCmd_GetStdMsgNaix','ScrCmd_MsgBoxExtern')
groups['src/scrcmd_party.c']+=('ScrCmd_PartyHasPokerus',)
groups['src/scrcmd_12.c']=('ScrCmd_MomGiftCheck',)
groups['src/scrcmd_20.c']=('ScrCmd_NopVar490',)
groups['src/scrcmd_strbuf.c']+=('ScrCmd_BufferRivalsName','ScrCmd_BufferPartyMonNick')
groups['src/scrcmd_items.c']+=('ScrCmd_TakeItem',)
groups['src/scrcmd_battle.c']=('ScrCmd_TrainerBattle','ScrCmd_CheckBattleWon')
groups['src/field_bgm.c']=('FieldBGM_GetEyesMeetForTrainer',)
groups['src/scrcmd_battle.c']+=('ScrCmd_EncounterMusic',)
groups['src/scrcmd_battle.c']+=('ScrCmd_GetTrainerNum','ScrCmd_GetTrainerMsgParams','ScrCmd_GetRematchMsgParams','ScrCmd_TrainerIsDoubleBattle')
groups['src/phonebook_dat.c']=('FreePhoneBook','LoadPhoneBookEntryI')
groups['src/scrcmd_c.c']+=('ScrCmd_LoadPhoneDat',)
groups['src/trainer_data.c']=('TrainerData_GetAttr','TrainerData_ReadTrData')
groups['src/script_manager.c']+=('TrainerNumIsDouble','ScriptNoToDoublePartnerNo')
groups['src/scrcmd_c.c']+=('ScrCmd_SetTrainerFlag','ScrCmd_CheckTrainerFlag')
groups['src/script_manager.c']+=('ScriptNumToTrainerNum','TrainerFlagCheck','TrainerFlagSet')
groups['src/battle/battle_setup.c']=('IsBattleResultWin',)
groups['src/sys_flags.c']+=('Save_VarsFlags_CheckHaveFollower',)
groups['src/sys_vars.c']+=('Save_VarsFlags_GetFollowerTrainerNum','Save_VarsFlags_SetFollowerTrainerNum')
groups['src/field/scrcmd_pokemon_misc.c']=('ScrCmd_BugContestAction','ScrCmd_BufferBugContestWinner','ScrCmd_JudgeBugContest','ScrCmd_BufferBugContestMonNick','ScrCmd_BugContestGetTimeLeft','ScrCmd_IsBugContestantRegistered')
groups['src/overlay_bug_contest.c']=('BugContest_BufferContestWinnerNames','BugContest_BufferCaughtMonNick')
scene_groups={'include/scrcmd.h':('ScriptGetVar','ScriptGetVarPointer'),
 'src/scrcmd_c.c':('ScrCmd_307','ScrCmd_308','ScrCmd_309','ScrCmd_310','ScrCmd_311','ScrCmd_GetPlayerCoords','ScrCmd_GetPersonCoords','ScrCmd_GetPlayerFacing','ScrCmd_MovePersonFacing','ScrCmd_PlaceStarterBallsInElmsLab','ScrCmd_609','ScrCmd_ChooseStarter','ScrCmd_ReleaseAll','ScrCmd_Lock','ScrCmd_Release','ScrCmd_ShowPerson','ScrCmd_HidePerson','ScrCmd_605','ScrCmd_608','ScrCmd_ToggleFollowingPokemonMovement','ScrCmd_WaitFollowingPokemonMovement','NativeScript_WaitFollowingPokemonMovement','ScrCmd_FollowingPokemonMovement')}
scene_groups['src/scrcmd_c.c']+=('ScrCmd_FadeScreen','ScrCmd_WaitFade','sub_02043458','ScrCmd_SetSpawn')
scene_groups['src/scrcmd_c.c']+=('ScrCmd_729','ScrCmd_GetPlayerState','ScrCmd_SetAvatarBits','ScrCmd_UpdateAvatarState','ScrCmd_446','ScrCmd_PokeCenAnim','ScrCmd_599','ScrCmd_436')
scene_groups['src/scrcmd_17.c']=('ScrCmd_GiveRunningShoes',)
scene_groups['src/map_object.c']=('MapObject_Delete',)
scene_groups['src/scrcmd_c.c']+=('ScrCmd_574','ScrCmd_109','ScrCmd_454')
scene_groups['src/overlay_26_022599D0.c']=('ov26_022599D0','ov26_022599D8','ov26_02259A24')
starter_groups={'src/launch_application.c':('ChooseStarter_LaunchApp',),'src/choose_starter.c':('LaunchStarterChoiceScene','CreateStarter'),'src/update_dex_received.c':('UpdatePokedexWithReceivedSpecies',)}
lines=['/* Original slices with recorded native adaptations; not whole-source coverage. */'];records=[];declarations=[]
scene_lines=[];scene_declarations=[];starter_lines=[];starter_declarations=[]
for relative in dict.fromkeys((*groups,*scene_groups,*starter_groups)):
 names=tuple(dict.fromkeys(groups.get(relative,())+scene_groups.get(relative,())+starter_groups.get(relative,())))
 path=root/relative;source=path.read_text()
 if relative=='src/field/scrcmd_message.c':
  lines.append('#include "files/msgdata/msg.naix"')
  tables=re.findall(r'const u16 ov01_022067C8\[\] = \{.*?^\};',source,re.M|re.S)
  if len(tables)!=1:raise ValueError('Original standard message banks changed')
  lines.append(tables[0])
 if relative=='src/field_bgm.c':
  music_tables=re.findall(r'^const static u16 sTrainerEncounterMusicParam\[\]\[3\] = \{.*?^\};',source,re.M|re.S)
  if len(music_tables)!=1:raise ValueError('Original trainer encounter music table changed')
  lines.append(music_tables[0])
 if relative=='src/scrcmd_c.c':
  match=re.search(r'^static const u8 sConditionTable\[6\]\[3\] = \{.*?^\};',source,re.M|re.S)
  if not match:raise ValueError('Original script condition table changed')
  lines.append(match[0])
 for name in names:
  matches=list(re.finditer(r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b'+name+r'\([^;{}]*\) \{\n.*?^\}',source,re.M|re.S))
  if len(matches)!=1:raise ValueError(('Original script definition changed',name,len(matches)))
  match=matches[0];line=source[:match.start()].count('\n')+1
  from native_bit_operations import adapt_native_bits
  body,adaptations=adapt_native_bits(relative,name,match[0])
  if name in groups.get(relative,()):
   declarations.append(match[0].split(' {\n',1)[0]+';')
   lines.extend([f'#line {line} "{path}"',body])
  if name in scene_groups.get(relative,()):
   scene_declarations.append(match[0].split(' {\n',1)[0]+';')
   scene_lines.extend([f'#line {line} "{path}"',match[0]])
  if name in starter_groups.get(relative,()):
   starter_declarations.append(match[0].split(' {\n',1)[0]+';')
   starter_lines.extend([f'#line {line} "{path}"',match[0]])
  records.append({'source':relative,'function':name,'line':line,'source_sha256':hashlib.sha256(path.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(match[0].encode()).hexdigest()})
  if adaptations:records[-1].update(adaptations=adaptations,emitted_sha256=hashlib.sha256(body.encode()).hexdigest())
starter_struct=re.search(r'struct ChooseStarterTaskData \{.*?^\};',(root/'src/choose_starter.c').read_text(),re.M|re.S)[0]
write_text_if_changed(out/'starter_kernel.inc', starter_struct+'\n'+'\n'.join(starter_lines)+'\n')
write_text_if_changed(out/'starter_decls.inc', '\n'.join(starter_declarations)+'\n')
write_text_if_changed(out/'script_scene_kernel.inc', '\n'.join(scene_lines)+'\n')
write_text_if_changed(out/'script_scene_decls.inc', '\n'.join(scene_declarations)+'\n')
write_text_if_changed(out/'script_kernel.inc', '\n'.join(lines)+'\n')
write_text_if_changed(out/'script_kernel_decls.inc', '\n'.join(declarations)+'\n')
script=(root/'include/script.h').read_text()
decls=[re.search(r'typedef enum ScriptEnvField \{.*?^\} ScriptEnvField;',script,re.M|re.S)[0]]
decls.append('#ifndef HG_NAME_SCREEN_TYPE\n#define HG_NAME_SCREEN_TYPE\n'+re.search(r'typedef enum NameScreenType \{.*?\} NameScreenType;', (root/'include/launch_application.h').read_text(),re.S)[0]+'\n#endif')
write_text_if_changed(out/'story_types.inc', '\n'.join(decls)+'\n')
commands=re.findall(r'^    (\w+),$',(root/'src/data/fieldmap/script_cmd_table.h').read_text(),re.M)
write_text_if_changed(out/'trainer_event_opcodes.inc','enum { HG_SCRIPT_TRAINER_MESSAGE='+str(commands.index('ScrCmd_TrainerMessage'))+', HG_SCRIPT_ENCOUNTER_MUSIC='+str(commands.index('ScrCmd_EncounterMusic'))+' };\n')
selected={name for names in groups.values() for name in names if name.startswith('ScrCmd_')}
scene_selected={n for names in scene_groups.values() for n in names if n.startswith('ScrCmd_')}
write_text_if_changed(out/'script_scene_admitted.inc', ''.join(f'case {i}:return 1; /* {name} */\n' for i,name in enumerate(commands) if name in scene_selected))
write_text_if_changed(out/'script_scene_table.inc', ''.join(f'[{i}]={name},\n' for i,name in enumerate(commands) if name in scene_selected))
write_text_if_changed(out/'script_core_table.inc', ''.join(f'[{i}]={name},\n' for i,name in enumerate(commands) if name in selected))
whole=('src/save_vars_flags.c','src/save_pokegear.c','src/apricorn_tree_sys.c')
write_text_if_changed(out/'script_kernel_provenance.json', json.dumps({'functions':records,
 'whole_c_inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in whole},
 'whole_c_boundaries':{'src/save_vars_flags.c':'src/compat/story_records.c','src/save_pokegear.c':'src/compat/pokegear.c','src/apricorn_tree_sys.c':'src/compat/apricorn.c'},
 'declaration_inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in ('include/script.h','include/player_data.h','include/save_misc_data.h','include/apricorn_tree.h','src/data/fieldmap/script_cmd_table.h','include/save_vars_flags.h')}},indent=2)+'\n')

# Derive fixed operand widths from admitted original handlers. Separate from
# semantic validation: this prevents duplicating opcode numbers/read sizes in
# the VM whenever the generated command closure grows.
widths={'ScriptReadByte':1,'ScriptReadHalfword':2,'ScriptReadWord':4,'ScriptGetVar':2,'ScriptGetVarPointer':2}
operand_lines=[]
for i,name in enumerate(commands):
 if name not in selected|scene_selected:continue
 record=next(r for r in records if r['function']==name)
 source=(root/record['source']).read_text()
 match=list(re.finditer(r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b'+name+r'\([^;{}]*\) \{\n.*?^\}',source,re.M|re.S))
 if len(match)!=1:raise ValueError(('operand handler',name))
 body=re.sub(r'/\*.*?\*/|//[^\n]*','',match[0][0],flags=re.S)
 reads=re.findall(r'\b(ScriptRead\w+|ScriptGetVar(?:Pointer)?)\s*\(',body)
 if any(read not in widths for read in reads):raise ValueError(('unknown script reader',name,reads))
 direct_bytes=len(re.findall(r'\*ctx->script_ptr\+\+',body))
 operand_lines.append(f'case {i}:return {sum(widths[read] for read in reads)+direct_bytes}; /* {name} */')
write_text_if_changed(out/'script_original_operands.inc', '\n'.join(operand_lines)+'\n')

# Validate captured sound IDs against the original catalog, rather than a
# caller-specific list of cues. Sound output remains a native muted boundary.
sounds=sorted({int(value) for value in re.findall(r'^#define\s+SEQ_\w+\s+(\d+)\s*$',(root/'include/constants/sndseq.h').read_text(),re.M)})
if not sounds:raise ValueError('Original sound catalog is empty')
write_text_if_changed(out/'script_sound_ids.inc', 'static const unsigned short script_sound_ids[] = {'+','.join(map(str,sounds))+'};\n')
# Pointer-free original Apricorn record, retained without unrelated misc/RTC SDK
# declarations. The complete original tree record implementation compiles.
misc=(root/'include/save_misc_data.h').read_text()
record=re.findall(r'typedef struct APRICORN_TREE \{.*?\} APRICORN_TREE;',misc,re.S)
if len(record)!=1:raise ValueError('Original Apricorn record changed')
limits=[re.search(r'^#define '+name+r'\s+\d+',misc,re.M)[0] for name in ('MAX_APRICORN_TREE','NUM_APRICORN_TREE')]
write_text_if_changed(out/'apricorn_types.inc', record[0]+'\n'+'\n'.join(limits)+'\n')

fade_header=(root/'include/screen_fade.h').read_text()
fade_enums=re.findall(r'enum Fade(?:Mode|Type) \{.*?\};',fade_header,re.S)
if len(fade_enums)!=2:raise ValueError('Original fade modes/types changed')
write_text_if_changed(out/'screen_fade_types.inc', '\n'.join(fade_enums)+'\n')

# Original trainer message lookup is shared by the native UTF-8 presentation
# boundary; preserve the resource-table algorithm without a second lookup.
trainer_message_source=root/'src/trainer_data.c'
trainer_message_matches=re.findall(r'^void GetTrainerMessageByIdPair\([^;{}]*\) \{\n.*?^\}',trainer_message_source.read_text(),re.M|re.S)
if len(trainer_message_matches)!=1:raise ValueError('Original trainer message lookup changed')
write_text_if_changed(out/'trainer_message_kernel.inc',trainer_message_matches[0]+'\n')
write_text_if_changed(out/'trainer_message_provenance.json',json.dumps({'source':'src/trainer_data.c','function':'GetTrainerMessageByIdPair','source_sha256':hashlib.sha256(trainer_message_source.read_bytes()).hexdigest(),'slice_sha256':hashlib.sha256(trainer_message_matches[0].encode()).hexdigest()},indent=2)+'\n')
