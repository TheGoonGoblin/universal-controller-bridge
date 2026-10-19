# SPDX-License-Identifier: MIT
"""Real local FTP transfer tests. No PS5 or external host is contacted."""
import ftplib
import hashlib
import importlib.util
import json
import logging
from pathlib import Path
import tempfile
import threading
from pyftpdlib.authorizers import DummyAuthorizer
from pyftpdlib.handlers import FTPHandler
from pyftpdlib.servers import FTPServer

spec=importlib.util.spec_from_file_location('installer',Path(__file__).resolve().parents[1]/'tools/install_ucb.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
logging.getLogger('pyftpdlib').setLevel(logging.ERROR)

with tempfile.TemporaryDirectory() as temp:
    root=Path(temp);local=root/'local';remote=root/'remote';local.mkdir();remote.mkdir()
    source={
      'eboot.bin':b'TEST FIXTURE - NOT A PS5 EXECUTABLE',
      'sce_module/libc.prx':b'TEST FIXTURE',
      'sce_sys/icon0.png':b'TEST FIXTURE',
      'sce_sys/param.json':json.dumps({'titleId':m.TITLE}).encode(),
      'ucb-install.json':json.dumps({'owner':m.OWNER,'titleId':m.TITLE}).encode()}
    for name,data in source.items():
        p=local/name;p.parent.mkdir(parents=True,exist_ok=True);p.write_bytes(data)
    def manifest():
        (local/'MANIFEST.json').write_text(json.dumps({n:hashlib.sha256((local/n).read_bytes()).hexdigest() for n in source}))
    manifest()
    auth=DummyAuthorizer();auth.add_user('test','local-only',str(remote),perm='elradfmwMT')
    class Handler(FTPHandler):pass
    Handler.authorizer=auth
    server=FTPServer(('127.0.0.1',0),Handler)
    port=server.socket.getsockname()[1]
    thread=threading.Thread(target=server.serve_forever,kwargs={'timeout':0.05},daemon=True);thread.start()
    try:
        with ftplib.FTP() as ftp:
            ftp.connect('127.0.0.1',port,timeout=5);ftp.login('test','local-only')
            m.install(ftp,local,lambda s:None)
            destination=remote/'data/homebrew'/m.TITLE
            assert (destination/'eboot.bin').read_bytes()==source['eboot.bin']
            (local/'eboot.bin').write_bytes(b'UPDATED TEST FIXTURE');manifest()
            m.install(ftp,local,lambda s:None)
            assert (destination/'eboot.bin').read_bytes()==b'UPDATED TEST FIXTURE'
            assert list((remote/'data').glob('ucb-backup-*'))
            class FailRename:
                def __getattr__(self,name):return getattr(ftp,name)
                def rename(self,a,b):
                    if 'ucb-staging-' in a:raise ftplib.error_perm('550 simulated rename failure')
                    return ftp.rename(a,b)
            try:m.install(FailRename(),local,lambda s:None)
            except ftplib.error_perm:pass
            else:raise AssertionError('Expected rename failure')
            assert (destination/'eboot.bin').read_bytes()==b'UPDATED TEST FIXTURE'
            (destination/'ucb-install.json').write_text('{"owner":"unrelated"}')
            try:m.install(ftp,local,lambda s:None)
            except RuntimeError:pass
            else:raise AssertionError('Foreign title should be protected')
            assert (destination/'eboot.bin').read_bytes()==b'UPDATED TEST FIXTURE'
            (local/'eboot.bin').write_bytes(b'CORRUPT')
            try:m.plan(local)
            except ValueError:pass
            else:raise AssertionError('Corrupt local file should fail')
    finally:
        server.close_all();thread.join(timeout=2)
print('Installer: local FTP install, checksums, update backup, rollback, collision and corruption checks passed')
