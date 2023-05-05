
import os
import sys
import subprocess
import re
import json
import platform
import hashlib
#import requests
import shutil
import zipfile
from datetime import datetime
from datetime import timedelta

src_dir   = os.environ.get('CMAKE_SOURCE_DIR')
build_dir = os.environ.get('CMAKE_BINARY_DIR')
cfg       = os.environ.get('CMAKE_BUILD_TYPE')
js_path   = os.environ.get('JSON_IMPORT_INDEX')
cm_build  = '_build'

def sha256_file(file_path):
    sha256_hash = hashlib.sha256()
    with open(file_path, "rb") as file:
        for chunk in iter(lambda: file.read(4096), b""):
            sha256_hash.update(chunk)
    return sha256_hash.hexdigest()

def replace_env(input_string):
    def repl(match):
        var_name = match.group(1)
        return os.environ.get(var_name, match.group(0))
    return re.sub(r'%([^%]+)%', repl, input_string)

dir = os.path.dirname(os.path.abspath(__file__))

# this only builds with cmake using a url with commit-id, optional diff
def check(a):
    assert(a)
    return a

def      parse(f): return f['name'] + '-' + f['version'], f['name'], f['version'], f.get('res'), f.get('sha256'), f.get('url'), f.get('commit'), f.get('diff')
def    git(*args): return subprocess.run(['git']   + list(args), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
def  cmake(*args): return subprocess.run(['cmake'] + list(args), stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode == 0
def   build(type): return cmake('--build', cm_build)
def     gen(type, cmake_script_root, prefix_path, extra=None):
    args = ['-S', cmake_script_root,
            '-B', cm_build, 
           f'-DCMAKE_PREFIX_PATH="{prefix_path}"'
            '-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON',
           f'-DCMAKE_BUILD_TYPE={type[0].upper() + type[1:].lower()}']
    if extra:
        args.extend(extra)
    return cmake(*args)

os.chdir(build_dir)
if not os.path.exists('extern'): os.mkdir('extern');

def latest_file(root_path, avoid = None):
    t_latest = 0
    n_latest = None
    ##
    for file_name in os.listdir(root_path):
        file_path = os.path.join(root_path, file_name)
        if os.path.isdir(file_path):
            if (file_name != '.git') and (not avoid or file_name != avoid):
                ## recurse into subdirectories
                sub_latest, t_sub_latest = latest_file(file_path)
                if sub_latest is not None:
                    if t_sub_latest and t_sub_latest > t_latest:
                        t_latest = t_sub_latest
                        n_latest = sub_latest
        else:
            ## check file modification time
            mt = os.path.getmtime(file_path)
            if mt and mt > t_latest:
                t_latest = mt
                n_latest = file_path
    ##
    return n_latest, t_latest

# prepare an { object } of external repo
def prepare_build(this_src_dir, fields):
    vname, name, version, res, sha256, url, commit, diff = parse(fields);
    
    dst = f'{build_dir}/extern/{vname}'

    ## if there is resource, download it and verify sha256 (required field)
    if res:
        res_zip = f'{build_dir}/io/res/{res}'
        digest = sha256_file(res_zip)
        if digest != sha256:
            print(f'sha256 checksum failure in project {name}')
            print(f'checksum for {res}: {digest}')
        with zipfile.ZipFile(res_zip, 'r') as z:
            z.extractall(dst)
    else:
        os.chdir(f'{build_dir}/extern')
        if not os.path.exists(vname):
            print(f'checking out {vname}...')
            git('clone', '--recursive', url, vname)

        os.chdir(vname)
        git('fetch') # this will get the commit identifiers sometimes not received yet
        if diff: git('reset', '--hard')
        git('checkout', commit)
        if diff: git('apply', f'{this_src_dir}/diff/{diff}')
        
    # overlay files; not quite as good as diff but its easier to manipulate
    overlay = f'{this_src_dir}/overlays/{name}'
    if os.path.exists(overlay):
        shutil.copytree(overlay, dst, dirs_exist_ok=True)
        
    return vname, dst, not res
    
everything = []

# create sym-link for each remote as f'{name}' as its first include entry, or the repo path if no includes
# all peers are symlinked and imported first
def prepare_project(src_dir):
    with open(src_dir + '/project.json', 'r') as project_contents:
        project_json = json.load(project_contents)
        import_list  = project_json['import']

        # import the peers first, which have their own index (fetch first!)
        # it should build while checking out
        for fields in import_list:
            everything.append(fields)
            if isinstance(fields, str):
                rel_peer = f'{src_dir}/../{fields}'
                sym_peer = f'{build_dir}/extern/{fields}'
                if not os.path.exists(sym_peer): os.symlink(rel_peer, sym_peer, True)
                assert(os.path.islink(sym_peer))
                prepare_project(rel_peer) # recurse into project, pulling its things too

        # now prep gen and build
        prefix_path = ''
        for fields in import_list:
            name = fields['name']
            if not isinstance(fields, str):
                cmake = fields.get('cmake')
                cmake_script_root = '.'
                cmake_args = []
                if cmake:
                    cmpath = cmake.get('path')
                    if cmpath:
                        cmake_script_root = cmpath
                    cmargs = cmake.get('args')
                    if cmargs:
                        cmake_args = cmargs
                platforms = fields.get('platforms')
                if platforms:
                    p = platform.system()
                    keep = False
                    if p == 'Darwin'  and 'mac'   in platforms:
                        keep = True
                    if p == 'Windows' and 'win'   in platforms:
                        keep = True
                    if p == 'Linux'   and 'linux' in platforms:
                        keep = True
                    if not keep:
                        print(f'skipping {name} for this platform...')
                        continue
                vname, remote_path, is_git = prepare_build(src_dir, fields)

                ## only building the src git repos; the resources are system specific builds
                if is_git:
                    ## get latest 
                    extern_build  = f'{remote_path}/{cm_build}'
                    perform_build = not os.path.exists(extern_build)
                    ts = os.path.join(extern_build, '._build_timestamp')

                    if not perform_build:
                        if os.path.exists(ts):
                            _, m_latest = latest_file(remote_path, cm_build)
                            build_time = os.stat(ts).st_mtime
                            perform_build = m_latest > build_time
                            if perform_build:
                                print(f'building {vname}... (updated)')
                            else:
                                print(f'skipping {vname}... (no-change)')
                        else:
                            perform_build = True

                    if perform_build:
                        print(f'building {vname}...')
                        gen(cfg, cmake_script_root, prefix_path, cmake_args)
                        build(cfg)
                        with open(ts, 'w') as f:
                            f.write('')
                
                remote_build_path = f'{remote_path}/_build'
                
                # add prefix path if there is one (so the next build sees all of the prior resources)
                if os.path.isdir(remote_build_path):
                    if prefix_path:
                        prefix_path += f'{prefix_path}:{remote_build_path}'
                    else:
                        prefix_path  = remote_build_path

# prepare project recursively
prepare_project(src_dir)

# output everything discovered in original order
with open(js_path, "w") as out:
    json.dump(everything, out)
