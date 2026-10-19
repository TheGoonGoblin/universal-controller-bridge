# SPDX-License-Identifier: MIT
"""Package only the validated native title, installer, notices and source."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import zipfile

root=Path(__file__).resolve().parents[1]
app=root/'build-title/PPSA73001'
required=['eboot.bin','sce_module/libc.prx','sce_sys/param.json','sce_sys/icon0.png']
for p in required:assert (app/p).is_file(),'Missing '+p
assert (app/'eboot.bin').read_bytes()[:4]==bytes.fromhex('4f153d1d'),'Not the native SELF'
assert 'integrity: valid' in (root/'build-title/eboot-inspection.txt').read_text()
param=json.loads((app/'sce_sys/param.json').read_text())
assert param['titleId']=='PPSA73001' and len(param['contentId'].split('-')[-1])==16
(app/'ucb-install.json').write_text(json.dumps({'owner':'TheGoonGoblin/universal-controller-bridge','titleId':'PPSA73001'})+'\n')
shutil.copyfile(root/'README.md',app/'README.md')
source_links=(root/'downloads/SOURCE_LINKS.txt').read_text()+'\nNative title tool source, vendored in source/third_party/native-title:\nhttps://github.com/mihawk-99/PS5_Vulkan/tree/f410e67a0241b3e8edcdbcd2a9b285dfc4a412ed/tooling/native\n'
(app/'SOURCE_LINKS.txt').write_text(source_links)
for name in ['pic0.png','pic1.png']:
    if (root/'assets/brand'/name).is_file():shutil.copyfile(root/'assets/brand'/name,app/'sce_sys'/name)
# Regenerate after all title files have been staged. The manifest excludes itself.
manifest={p.relative_to(app).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(app.rglob('*')) if p.is_file() and p.name!='MANIFEST.json'}
(app/'MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
out=root/'downloads/UCB_PS5_HomeScreen_Preview.zip'
paths=subprocess.check_output(['git','ls-files','--cached','--others','--exclude-standard'],cwd=root,text=True).splitlines()
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for p in sorted(app.rglob('*')):
        if p.is_file():z.write(p,'PPSA73001/'+p.relative_to(app).as_posix())
    for name in ['Install_UCB.command','install_ucb.py']:
        info=zipfile.ZipInfo(name);info.external_attr=(0o100755<<16);info.compress_type=zipfile.ZIP_DEFLATED
        z.writestr(info,(root/'tools'/name).read_bytes())
    z.write(root/'docs/home-screen-install.md','START_HERE.md')
    z.writestr('SOURCE_LINKS.txt',source_links)
    for name in paths:
        if name.startswith('downloads/') or '__pycache__' in name:continue
        z.write(root/name,'source/'+name)
with zipfile.ZipFile(out) as z:assert z.testzip() is None
digests=[]
for p in sorted((root/'downloads').glob('*.zip')):digests.append(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name)
(root/'downloads/SHA256SUMS.txt').write_text('\n'.join(digests)+'\n')
print(out.name,out.stat().st_size,'bytes; title files',len(manifest))
