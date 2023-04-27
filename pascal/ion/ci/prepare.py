
import os
import sys
import subprocess
import re
import json
import platform
import hashlib
import requests

print(f"File downloaded and saved to {local_file_path}")

src_dir   = os.environ.get('CMAKE_SOURCE_DIR')
build_dir = os.environ.get('CMAKE_BINARY_DIR')
cfg       = os.environ.get('CMAKE_BUILD_TYPE')
js_path   = os.environ.get('JSON_IMPORT_INDEX')

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
def   build(type): return cmake('--build', 'build')
def     gen(type, prefix_path, extra=None):
    args = ['-S', '.',
            '-B', 'build', 
           f'-DCMAKE_PREFIX_PATH="{prefix_path}"'
            '-DCMAKE_VERBOSE_MAKEFILE:BOOL=ON',
           f'-DCMAKE_BUILD_TYPE={type[0].upper() + type[1:].lower()}']
    if extra:
        args.extend(extra)
    return cmake(*args)

os.chdir(build_dir)
if not os.path.exists('extern'): os.mkdir('extern');

# prepare an { object } of external repo
# this is not done for peers
def prepare_build(src_dir, fields):
    vname, name, version, res, sha256, url, commit, diff = parse(fields);
    
    ## if there is resource, download it and verify sha256 (required field)
    if res:
        # cmake handles this before prepare is launched
        res_repo = f'{build_dir}/{io_repo}/res/{res}'
        digest = sha256_file(res_repo)
        assert(digest == sha256)
        return res_repo, False
    else:
        os.chdir(build_dir + '/extern')
        if not os.path.exists(vname): git('clone', '--recursive', url, vname)
        os.chdir(vname)
        git('fetch')
        if diff: git('reset', '--hard')
        git('checkout', commit)
        if diff: git('apply', f'{src_dir}/diff/{diff}')
        return f'{build_dir}/extern/{vname}', True
    
everything = []

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
                prepare_project(rel_peer)

        # now prep gen and build
        prefix_path = ''
        for fields in import_list:
            if not isinstance(fields, str):
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
                        continue
                remote_path, is_git = prepare_build(src_dir, fields)

                # only building the src git repos; the resources are system specific builds
                if is_git:
                    gen(cfg, prefix_path, fields.get('cmake'))
                    build(cfg)
                
                # add prefix path if there is one (so the next build sees all of the prior resources)
                remote_build_path = f'{remote_path}/build'
                if os.path.isdir(remote_build_path):
                    if prefix_path:
                        prefix_path  += f'{prefix_path}:{remote_build_path}'
                    else:
                        prefix_path   = remote_build_path

# prepare project recursively
prepare_project(src_dir)

# output everything discovered in original order
with open(js_path, "w") as out:
    json.dump(everything, out)
