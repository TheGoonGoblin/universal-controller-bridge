# SPDX-License-Identifier: MIT
"""Verify a native SELF round trip, allowing stripped non-loadable note bytes."""
import struct
import sys
from pathlib import Path

original=Path(sys.argv[1]).read_bytes()
extracted=Path(sys.argv[2]).read_bytes()
assert original[:4]==b'\x7fELF' and len(original)==len(extracted)
assert struct.unpack_from('<H',original,16)[0]==0xfe10,'Not a native SCE executable'
phoff=struct.unpack_from('<Q',original,32)[0]
size,count=struct.unpack_from('<HH',original,54)
assert size==56 and original[:phoff+size*count]==extracted[:phoff+size*count]
notes=[];loads=[]
for i in range(count):
 kind,flags,offset,vaddr,paddr,filesize,memsize,align=struct.unpack_from('<IIQQQQQQ',original,phoff+i*size)
 assert offset+filesize<=len(original)
 if kind==1:
  assert original[offset:offset+filesize]==extracted[offset:offset+filesize],'Loadable bytes changed'
  loads.append((vaddr,vaddr+memsize))
 if kind==4:notes.append((offset,offset+filesize))
for i,(a,b) in enumerate(zip(original,extracted)):
 if a!=b:assert any(lo<=i<hi for lo,hi in notes),'Unexpected non-note difference'
entry=struct.unpack_from('<Q',original,24)[0]
assert any(lo<=entry<hi for lo,hi in loads),'Entry point outside loaded ranges'
print('Native ELF/container roundtrip verified; loaded bytes identical')
