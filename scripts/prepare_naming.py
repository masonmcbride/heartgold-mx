"""Original naming application lifecycle and pointer-free misc save record."""
from generated_files import write_text_if_changed
import pathlib,re,json,hashlib,sys
root,out=map(pathlib.Path,sys.argv[1:]);records=[]
def one(pattern,text):
 matches=re.findall(pattern,text,re.M|re.S)
 if len(matches)!=1:raise ValueError(('Ambiguous naming input',pattern,len(matches)))
 return matches[0]
def function(path,name):
 source=(root/path).read_text();body=one(r'^(?:static )?[^\n;{}]*\b'+name+r'\([^;{}]*\) \{\n.*?^\}',source)
 records.append({'source':path,'function':name,'sha256':hashlib.sha256(body.encode()).hexdigest()});return body
misc=(root/'include/save_misc_data.h').read_text();rtc=(root/'include/gf_rtc.h').read_text()
types=[one(r'struct GF_RTC_DateTime \{.*?\};',rtc),one(r'typedef struct BerryPot \{.*?\} BerryPot;',misc),'#define MAX_BERRY_POT 4',one(r'typedef struct SAVE_MISC_DATA \{.*?\} SAVE_MISC_DATA;',misc)]
write_text_if_changed(out/'misc_records.inc', '\n'.join(types)+'\n')
functions=[function('src/player_data.c','Save_Profile_PlayerName_Set'),function('src/string_util.c','StringLength'),function('src/string_util.c','CopyU16StringArray')]+[function('src/launch_application.c',n) for n in ('Task_NamingScreen','SetName','CallTask_NamingScreen')]+[function('src/naming_screen.c',n) for n in ('NamingScreen_CreateArgs','NamingScreen_DeleteArgs')]+[function('src/save_misc.c',n) for n in ('Save_Misc_RivalName_Const_Get','Save_Misc_RivalName_Set')]
write_text_if_changed(out/'naming_kernel.inc', '\n'.join(functions)+'\n')
write_text_if_changed(out/'naming_decls.inc', '\n'.join(b.split(' {\n')[0]+';' for b in functions)+'\n')
record=one(r'typedef struct NamingScreenData \{.*?\} NamingScreenData;',(root/'include/launch_application_internal.h').read_text())
write_text_if_changed(out/'naming_records.inc', record+'\n')
write_text_if_changed(out/'naming_provenance.json', json.dumps({'functions':records},indent=2)+'\n')
