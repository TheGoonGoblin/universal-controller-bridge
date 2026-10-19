# SPDX-License-Identifier: MIT
"""Make an install ZIP with exactly one top-level PPSA folder."""
from pathlib import Path
import hashlib
import json
import shutil
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
source_links='''Complete corresponding source for this native binary:
UCB application, build scripts and vendored native-title tooling:
https://github.com/TheGoonGoblin/universal-controller-bridge/archive/3e817304dc8ec4e00b1f841bdb05c71ffe8ec7fe.zip

PS5 payload SDK v0.44 runtime source:
https://github.com/ps5-payload-dev/sdk/archive/refs/tags/v0.44.zip

LLVM 18.1.8 C++ runtime source:
https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/llvm-project-18.1.8.src.tar.xz

The install ZIP contains only the title folder. Source remains available at
the copying facilities above. Original files retain their licenses; the
combined binary is GPLv3-or-later. License texts are in the licenses folder.
'''
(app/'SOURCE_LINKS.txt').write_text(source_links)
for name in ['pic0.png','pic1.png']:
    if (root/'assets/brand'/name).is_file():shutil.copyfile(root/'assets/brand'/name,app/'sce_sys'/name)
# Regenerate after all title files have been staged. The manifest excludes itself.
manifest={p.relative_to(app).as_posix():hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(app.rglob('*')) if p.is_file() and p.name!='MANIFEST.json'}
(app/'MANIFEST.json').write_text(json.dumps(manifest,indent=2)+'\n')
out=root/'downloads/UCB_PS5_HomeScreen_Preview.zip'
with zipfile.ZipFile(out,'w',zipfile.ZIP_DEFLATED,compresslevel=9) as z:
    for p in sorted(app.rglob('*')):
        if p.is_file():z.write(p,'PPSA73001/'+p.relative_to(app).as_posix())
with zipfile.ZipFile(out) as z:
    assert z.testzip() is None
    assert {name.split('/')[0] for name in z.namelist()}=={'PPSA73001'}
digests=[]
for p in sorted((root/'downloads').glob('*.zip')):digests.append(hashlib.sha256(p.read_bytes()).hexdigest()+'  '+p.name)
(root/'downloads/SHA256SUMS.txt').write_text('\n'.join(digests)+'\n')
print(out.name,out.stat().st_size,'bytes; title files',len(manifest))
