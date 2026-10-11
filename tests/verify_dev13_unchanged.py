#!/usr/bin/env python3
"""Verify 24 unfixed renderer bodies are byte-identical to archived dev.13.
Call: python3 tests/verify_dev13_unchanged.py <path-to-dev13-archive>
"""
import hashlib
import re
import sys
import zipfile
from pathlib import Path

# There are 20 standard modes, 5 experimental numbered 21..25, and 5 audio.
unchanged_standard=set(range(1,26))-{1,2,9,10,14,16}
unchanged_audio=set(range(1,6))

def bodies(s):
    d={}
    for m in re.finditer(r'static void (mode_(?:gpt\d+|audio\d+))\(\)\s*\{',s):
        depth=0
        for j in range(m.end()-1,len(s)):
            if s[j]=='{': depth+=1
            elif s[j]=='}':
                depth-=1
                if depth==0:
                    d[m.group(1)]=s[m.start():j+1]
                    break
    return d

if len(sys.argv)!=2:
    sys.exit('usage: verify_dev13_unchanged.py <dev13 archive>')
with zipfile.ZipFile(sys.argv[1]) as z:
    src=z.read('wled-usermod-gpt-matrix-fx/gpt_matrix_fx.cpp').decode()
now=Path(__file__).resolve().parent.parent.joinpath('gpt_matrix_fx.cpp').read_text()
a,b=bodies(src),bodies(now)
keep=['mode_gpt%d'%n for n in sorted(unchanged_standard)]+['mode_audio%d'%n for n in sorted(unchanged_audio)]
assert len(keep)==24,(len(keep),keep)
for k in keep:
    assert k in a and k in b,k
    assert a[k]==b[k],(k,hashlib.sha256(a[k].encode()).hexdigest(),hashlib.sha256(b[k].encode()).hexdigest())
assert 'frac(sinf(ix*127.1f+iy*311.7f)*43758.5453f)' in b['mode_gpt10']
print('DEV13_UNCHANGED_PASS unchanged_renderers=%d spatial_hash=verbatim'%len(keep))
