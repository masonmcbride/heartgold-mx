"""Extract the original blackout controller; native display stays at its boundary."""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,re,sys
root,out=map(pathlib.Path,sys.argv[1:]);path=root/'src/blackout.c';source=path.read_text()
parts=re.findall(r'enum \{\n\s+STATE_BLACKOUT_TASK_INIT,.*?\n\};',source,re.S)
bodies=re.findall(r'^BOOL Task_Blackout\(TaskManager \*taskManager\) \{\n.*?^\}',source,re.M|re.S)
if len(parts)!=1 or len(bodies)!=1:raise ValueError('Ambiguous original blackout controller')
write_text_if_changed(out/'blackout_kernel.inc', parts[0]+'\n'+bodies[0]+'\n')
write_text_if_changed(out/'blackout_provenance.json', json.dumps({'source':'src/blackout.c','source_sha256':hashlib.sha256(source.encode()).hexdigest(),'function':'Task_Blackout','sha256':hashlib.sha256(bodies[0].encode()).hexdigest()},indent=2)+'\n')
