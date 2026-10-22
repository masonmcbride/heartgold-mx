"""Shared cached ARM data assembler and ELF section reader."""
from generated_files import write_text_if_changed
import hashlib,json,pathlib,struct,subprocess,shlex
def sections(data,allow_relocations=False):
 assert data[:6]==b'\x7fELF\x01\x01'
 offset=struct.unpack_from('<I',data,32)[0];size,count,names=struct.unpack_from('<HHH',data,46)
 table=[struct.unpack_from('<10I',data,offset+i*size) for i in range(count)]
 def content(s):return data[s[4]:s[4]+s[5]]
 strings=content(table[names])
 def name(i):return strings[i:strings.index(b'\0',i)].decode()
 assert allow_relocations or not any(s[1] in (4,9) and s[5] for s in table), 'unresolved data relocation'
 return table,content,name

def assemble(root,out,cc,source,target):
 command=[str(cc),'-target','arm-none-eabi','-DPM_ASM','-x','assembler-with-cpp',
  '-I'+str(root/'include'),'-I'+str(root/'asm'),'-I'+str(root/'files'),'-I'+str(out),'-c',str(source),'-o',str(target)]
 stamp=target.with_suffix('.deps.json');depfile=target.with_suffix('.d')
 def digest(path):return hashlib.sha256(pathlib.Path(path).read_bytes()).hexdigest()
 compiler=cc.resolve().stat();identity=[command,compiler.st_size,compiler.st_mtime_ns]
 if stamp.is_file() and target.is_file():
  cached=json.loads(stamp.read_text())
  if cached['compiler']==identity and all(pathlib.Path(p).is_file() and digest(p)==value for p,value in cached['inputs'].items()):return target.read_bytes()
 result=subprocess.run(command+['-MMD','-MF',str(depfile)],capture_output=True,text=True)
 if result.returncode:raise ValueError(result.stderr)
 dependencies=shlex.split(depfile.read_text().replace('\\\n','').split(':',1)[1])
 write_text_if_changed(stamp, json.dumps({'compiler':identity,'inputs':{p:digest(p) for p in dependencies}})+'\n')
 return target.read_bytes()

