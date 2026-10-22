"""Bounded original tutorial setup and input callbacks; reject ambiguity."""
from generated_files import write_text_if_changed
import pathlib,re,json,hashlib,sys
root,out=map(pathlib.Path,sys.argv[1:]);records=[]
def one(pattern,s):
 matches=re.findall(pattern,s,re.M|re.S)
 if len(matches)!=1:raise ValueError(('Ambiguous tutorial extraction',pattern,len(matches)))
 return matches[0]
def fun(path,name):
 body=one(r'^(?:static )?[^\n;{}]*\b'+name+r'\([^;{}]*\) \{\n.*?^\}',(root/path).read_text());records.append({'source':path,'function':name,'sha256':hashlib.sha256(body.encode()).hexdigest()});return body
header=(root/'include/battle/battle_setup.h').read_text()
write_text_if_changed(out/'tutorial_setup_record.inc', one(r'struct BattleSetupSub_138 \{.*?^\};',header)+'\n'+one(r'struct BattleSetup \{.*?^\};',header)+'\n')
setup=(root/'src/battle/battle_setup.c').read_text()
funcs=[fun('src/options.c','Options_Copy'),fun('src/player_data.c','PlayerProfile_SetTrainerGender')]+[fun('src/battle/battle_setup.c',n) for n in ('FieldSystem_GetTerrainFromStandingTile','sub_02052504','BattleSetup_SetAllySideBattlersToPlayer','BattleSetup_New_Tutorial')]
write_text_if_changed(out/'tutorial_setup_decls.inc', '\n'.join(f.split(' {\n')[0]+';' for f in funcs)+'\n')
write_text_if_changed(out/'tutorial_setup_kernel.inc', one(r'static const Terrain _020FC4C0\[.*?^\};',setup)+'\n'+'\n'.join(funcs)+'\n')
funcs=[fun('src/battle/battle_input.c',n) for n in ('BattleInput_CatchingTutorialCB_Fight','BattleInput_CatchingTutorialCB_Move','BattleInput_CatchingTutorialCB_Bag','BattleInput_CatchingTutorialMain')]
inputs=(root/'include/battle/battle_input.h').read_text()
write_text_if_changed(out/'tutorial_input_record.inc', one(r'typedef struct BattleInputTutorial \{.*?\} BattleInputTutorial;',inputs)+'\n')
write_text_if_changed(out/'tutorial_input_kernel.inc', '\n'.join(f.split(' {\n')[0]+';' for f in funcs)+'\nstatic int (*const ov12_0226E260[2])(BattleInput *)={BattleInput_CatchingTutorialCB_Fight,BattleInput_CatchingTutorialCB_Move};\n'+'\n'.join(funcs)+'\n')
write_text_if_changed(out/'tutorial_provenance.json', json.dumps({'functions':records},indent=2)+'\n')
