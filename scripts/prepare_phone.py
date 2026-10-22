"""Bounded original phone controller, records and catalogs; build-local only."""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:]);records=[]
def one(pattern,text):
 matches=re.findall(pattern,text,re.M|re.S)
 if len(matches)!=1:raise ValueError(('ambiguous phone input',pattern,len(matches)))
 return matches[0]
def function(path,name):
 source=(root/path).read_text();body=one(r'^(?:(?:static|inline) )*[A-Za-z_][^\n;{}]*\b'+name+r'\([^;{}]*\) \{\n.*?^\}',source)
 records.append({'source':path,'function':name,'source_sha256':hashlib.sha256(source.encode()).hexdigest(),'slice_sha256':hashlib.sha256(body.encode()).hexdigest()});return body
header=(root/'include/application/pokegear/phone/phone_internal.h').read_text()
types=[one(r'typedef struct '+n+r' \{.*?\} '+n+r';',header) for n in ('PokegearPhoneCallState','PokegearPhoneCallContext')]
types.insert(0,one(r'typedef struct PhoneCallScriptDef \{.*?\} PhoneCallScriptDef;',(root/'include/application/pokegear/pokegear_internal.h').read_text()))
write_text_if_changed(out/'phone_types.inc', '\n'.join(types)+'\n')
source='src/application/pokegear/phone/overlay_101_021F1D74.c'
names=('PhoneCall_Main','GearPhoneCall_Simple','PhoneCall_InitialRing','PhoneCall_HangupTone','PhoneCall_HangupNoTone','PhoneCall_PrintGreeting','PhoneCall_ApplyGenericNPCcallSideEffect')
bodies=[function(source,n) for n in names]+[function('src/application/pokegear/phone/scripts/phone_scripts_prof_elm.c','PhoneCall_GetScriptId_ProfElm')]
elm=(root/'src/application/pokegear/phone/scripts/phone_scripts_prof_elm.c').read_text()
bodies.insert(0,one(r'static const u8 sPhoneCallData_ProfElm_MapScripts\[\]\[2\] = \{.*?^\};',elm))
# Preserve original incoming selectors; unsupported outgoing helper services are
# explicit compatibility boundaries, outside incoming admission.
for caller in ('Mother','ProfOak','BikeShop','Bill','DayCareMan','Baoba'):
 filenames={'Mother':'mother','ProfOak':'prof_oak','BikeShop':'bike_shop','Bill':'bill','DayCareMan':'daycare_man','Baoba':'baoba'}
 bodies.append(function('src/application/pokegear/phone/scripts/phone_scripts_'+filenames[caller]+'.c','PhoneCall_GetScriptId_'+caller))
bodies.append(function('src/application/pokegear/phone/scripts/phone_scripts_baoba.c','GearPhoneCall_Baoba'))
for name in ('GearPhoneCall_Mother','MomCallGetIntroMsgByLocation','MomCallGetSaveMoneyPromptMsg'):
 bodies.append(function('src/application/pokegear/phone/scripts/phone_scripts_mother.c',name))
for path,names in [('src/safari_zone.c',('SafariZone_IncObjectUnlockLevel','SafariZone_SetObjectUnlockLevel','sub_0202F784')),('src/sys_flags.c',('Save_VarsFlags_MomsSavingsFlagAction','Save_VarsFlags_MomsSavingsFlagCheck')),('src/sys_vars.c',('Save_VarsFlags_SetVar4057',)),('src/pokedex.c',('Pokedex_CountNationalDexOwned',))]:
 for name in names:bodies.append(function(path,name))
write_text_if_changed(out/'phone_decls.inc', '\n'.join(b.split(' {\n')[0]+';' for b in bodies[1:])+'\n')
# Type15 is the original Baoba area-report controller; all other admitted calls
# use type0. Missing controller branches are rejected before application launch.
bodies.insert(1,'static BOOL (*const sPhoneCallHandlers[16])(PokegearPhoneCallContext *) = {[0]=GearPhoneCall_Simple,[4]=GearPhoneCall_Mother,[15]=GearPhoneCall_Baoba};')
# Compile the complete original script catalog with original JSON message IDs.
# The same substitution applies to the extracted kernel bodies: a `msg_BBB_ID`
# reference is the member index inside message bank BBB.
ids={}
for path in sorted((root/'files/msgdata/msg').glob('*.json')):
 for i,m in enumerate(json.loads(path.read_text())['messages']):
  if m['id'] in ids:raise ValueError(('duplicate message',m['id']))
  ids[m['id']]=i
def message_index(body):
 return re.sub(r'\bmsg_\d{4}_\d{5}\b',lambda m:str(ids[m[0]]),body)
bodies=[message_index(b) for b in bodies]
write_text_if_changed(out/'phone_kernel.inc', '\n'.join(bodies)+'\n')
catalog=one(r'const PhoneCallScriptDef gPhoneCallScriptDef\[\] = \{.*?^\};',(root/'src/application/pokegear/phone/phone_script_defs.c').read_text())
greetings=one(r'const u8 sGreetingMsgIDs\[\]\[12\] = \{.*?^\};',(root/'src/application/pokegear/phone/phone_generic_headers.c').read_text())
catalog=greetings+'\n'+catalog
catalog=message_index(catalog)
book=json.loads((root/'files/tel/pmtel_book.json').read_text())['pmtel_book']
fields=('id','type','unk2','trainerClass','trainerId','mapId','gift','phoneScriptIfLocal','unkC','rematchWeekday','rematchTimeOfDay','unkF','sortParam')
rows=[]
for row in book:
 if set(row)!=set(fields) or len(row['sortParam'])!=4:raise ValueError('Original phonebook schema changed')
 rows.append('{'+','.join('.'+k+'='+('{'+','.join(map(str,row[k]))+'}' if k=='sortParam' else str(row[k])) for k in fields)+'}')
constants='\n'.join(re.findall(r'^#define (?:RTC_WEEK_|TIMEOFDAY_WILD_).*$',(root/'files/tel/pmtel_book.json.txt').read_text(),re.M))
banks=one(r'static const u16 sPhoneMessageGmm\[\] = \{.*?^\};',(root/'src/phonebook_dat.c').read_text())
banks=re.sub(r'\bmsg_(\d{4})\b',lambda m:str(int(m[1])),banks)
write_text_if_changed(out/'phone_catalog.inc', constants+'\n'+catalog+'\n'+banks+'\nstatic const PhoneBookEntry phone_entries[]={'+','.join(rows)+'};\n')
inputs=('include/application/pokegear/phone/phone_internal.h','include/application/pokegear/pokegear_internal.h','src/application/pokegear/phone/phone_script_defs.c','src/phonebook_dat.c','files/tel/pmtel_book.json','files/tel/pmtel_book.json.txt')
write_text_if_changed(out/'phone_provenance.json', json.dumps({'functions':records,'inputs':{p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in inputs},'limitations':['Incoming trigger calls and scripted Elm; original Pokegear graphics/contact menus and unadmitted outgoing controllers are not executed.']},indent=2)+'\n')
