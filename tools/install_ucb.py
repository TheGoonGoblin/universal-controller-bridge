# SPDX-License-Identifier: MIT
"""Explicit-target FTP installer; staged uploads and SHA-256 verification."""
import argparse
import ftplib
import getpass
import hashlib
import ipaddress
import json
from pathlib import Path, PurePosixPath
import uuid

TITLE='PPSA73001'
OWNER='TheGoonGoblin/universal-controller-bridge'

def require(ok,message):
    if not ok:raise ValueError(message)

def plan(app):
    app=Path(app)
    marker=json.loads((app/'ucb-install.json').read_text())
    require(marker=={'owner':OWNER,'titleId':TITLE},'Invalid UCB installation marker')
    param=json.loads((app/'sce_sys/param.json').read_text())
    require(param['titleId']==TITLE,'Wrong title ID')
    manifest=json.loads((app/'MANIFEST.json').read_text())
    files=[]
    for name,expected in sorted(manifest.items()):
        path=PurePosixPath(name)
        require(name and not path.is_absolute() and '..' not in path.parts,'Unsafe manifest path')
        local=app.joinpath(*path.parts)
        require(local.is_file() and not local.is_symlink(),'Missing or symlinked input')
        require(hashlib.sha256(local.read_bytes()).hexdigest()==expected,'Local checksum mismatch: '+name)
        files.append((local,name,expected))
    require({'eboot.bin','sce_sys/param.json','sce_sys/icon0.png','sce_module/libc.prx','ucb-install.json'}<=set(manifest),'Incomplete title')
    files.append((app/'MANIFEST.json','MANIFEST.json',hashlib.sha256((app/'MANIFEST.json').read_bytes()).hexdigest()))
    return files

def ensure_dir(ftp,path):
    current=''
    for part in PurePosixPath(path).parts:
        if part=='/':continue
        current+='/'+part
        try:ftp.mkd(current)
        except ftplib.error_perm:
            ftp.cwd(current)  # fail if it is not actually an accessible directory
            ftp.cwd('/')

def exists(ftp,path):
    try:ftp.cwd(path);ftp.cwd('/');return True
    except ftplib.error_perm:return False

def install(ftp,app,report=print):
    files=plan(app)
    remote='/data/homebrew/'+TITLE
    already=exists(ftp,remote)
    if already:
        content=bytearray()
        try:ftp.retrbinary('RETR '+remote+'/ucb-install.json',content.extend)
        except ftplib.all_errors as e:raise RuntimeError('Existing title has no UCB marker; nothing was overwritten') from e
        if json.loads(content)!= {'owner':OWNER,'titleId':TITLE}:
            raise RuntimeError('Existing title belongs to another application; nothing was overwritten')
    token=uuid.uuid4().hex[:12]
    stage='/data/ucb-staging-'+token
    backup='/data/ucb-backup-'+token
    ensure_dir(ftp,stage)
    for local,name,expected in files:
        destination=stage+'/'+name
        ensure_dir(ftp,str(PurePosixPath(destination).parent))
        with local.open('rb') as stream:ftp.storbinary('STOR '+destination,stream)
        digest=hashlib.sha256()
        ftp.retrbinary('RETR '+destination,digest.update)
        if digest.hexdigest()!=expected:raise RuntimeError('Upload verification failed: '+name)
        report('Verified '+name)
    ensure_dir(ftp,'/data/homebrew')
    if already:ftp.rename(remote,backup)
    try:ftp.rename(stage,remote)
    except ftplib.all_errors:
        if already:ftp.rename(backup,remote)
        raise
    if already:report('Previous UCB files retained at '+backup)
    report('Transfer verified: '+remote)
    report('Use your usual ShadowMountPlus registration/refresh, then launch UCB.')
    report('Console launch and Xbox bridging have not been verified by this installer.')
    return remote

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--host')
    parser.add_argument('--port',type=int)
    parser.add_argument('--user',default='anonymous')
    parser.add_argument('--app',type=Path,default=Path(__file__).resolve().parent/'PPSA73001')
    parser.add_argument('--dry-run',action='store_true')
    args=parser.parse_args()
    files=plan(args.app)
    if args.dry_run:
        for local,name,digest in files:print(name,local.stat().st_size,digest)
        return
    host=args.host or input('PS5 IPv4 address: ').strip()
    ipaddress.IPv4Address(host)
    port=args.port or int(input('PS5 FTP port [1337]: ').strip() or '1337')
    if not 1<=port<=65535:raise ValueError('Invalid FTP port')
    password='anonymous@' if args.user=='anonymous' else getpass.getpass('FTP password: ')
    print('Connecting only to',host,'port',port)
    with ftplib.FTP() as ftp:
        ftp.connect(host,port,timeout=15);ftp.login(args.user,password)
        install(ftp,args.app)

if __name__=='__main__':
    try:main()
    except (OSError,ValueError,KeyError,AssertionError,RuntimeError,ftplib.Error) as e:
        print('Installation stopped:',e)
        raise SystemExit(1)
