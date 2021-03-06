#!/usr/bin/env python3

import os
import subprocess


def get_gcc_plugin_name():
    return 'libgcc_feature_extract'


def is_command_on_path(cmd):
    for path in os.environ['PATH'].split(os.pathsep):
        path = path.strip('"')
        exec_file = os.path.join(path, cmd)
        if os.path.isfile(exec_file) and os.access(exec_file, os.X_OK):
            return True
    return False


class preserve_cwd():
    def __enter__(self):
        self.cwd = os.getcwd()
    def __exit__(self, type, value, traceback):
        os.chdir(self.cwd)


def build(src_dir, build_dir, install_dir, build_system, cc, cxx, fort, flags):
    assert(os.path.exists(src_dir) and os.path.isabs(src_dir))
    assert(os.path.exists(build_dir) and os.path.isabs(build_dir))
    assert(os.path.exists(install_dir) and os.path.isabs(install_dir))
    assert(is_command_on_path(cc))
    assert(is_command_on_path(cxx))
    assert(is_command_on_path(fort))

    print ('-- Building source    \'' + src_dir + '\'')
    print ('   Build directory:   \'' + build_dir + '\'')
    print ('   Install directory: \'' + install_dir + '\'')

    # If no build system was specified, try and detect either a CMake or
    # configure script based build system
    if build_system is None or build_system == '':
        print ('-- No build system specified')
        print ('-- Searching for CMakeLists.txt in \'' + src_dir + '\'')
        if os.path.exists(os.path.join(src_dir, 'CMakeLists.txt')):
            print ('-- Found, configuring using CMake')
            build_system = 'cmake'
        else:
            print ('-- Searching for configure script in \'' + src_dir + '\'')
            if os.path.exists(os.path.join(src_dir, 'configure')):
                print ('-- Found, configuring using configure script')
                build_system = 'configure'
            else:
                print ('-- No supported build system found')
                return False
    elif build_system == 'cmake':
        print ('-- Configuring using cmake')
    elif build_system == 'configure':
        print ('-- Configuring using configure script')
    else:
        print ('-- Configuring using user-specified build script')

    base_dir = os.getcwd()
    with preserve_cwd():
        if build_system == 'cmake' or build_system == 'configure':
            os.chdir(build_dir)
            if build_system == 'cmake':
                if not is_command_on_path('cmake'):
                    print ('-- cmake command not on path')
                    raise Exception

                cmd = ['cmake', src_dir, '-G', 'Unix Makefiles']
                cmd.append('-DCMAKE_C_COMPILER=' + cc)
                cmd.append('-DCMAKE_C_FLAGS=' + flags)
                cmd.append('-DCMAKE_INSTALL_PREFIX=' + install_dir)
            elif build_system == 'configure':
                cmd = [os.path.join(src_dir, 'configure')]
                cmd.append('CC=' + cc)
                cmd.append('CFLAGS=' + flags)
                cmd.append('--prefix=' + install_dir)
    
            # configure the build
            ret = subprocess.call(cmd)
            print ('-- Configuring with: \'' + ' '.join(cmd) + '\'')
            if ret != 0:
                print ('-- Failed to configure \'' + src_dir + '\' using CMake')
                return False

            # Both cmake and configure will generate Makefiles
            if not is_command_on_path('make'):
                print ('-- make command not on path')
                return False

            cmd = ['make']
            ret = subprocess.call(cmd)
            if ret != 0:
                print ('-- Failed to build source \'' + src_dir + '\' using CMake')
                return False

            cmd = ['make', 'install']
            ret = subprocess.call(cmd)
            if ret != 0:
                print ('-- Failed to install build \'' + build_dir + '\' to \'' + install_dir)
                return False
        else:
            # If a custom build script is used, then the benchmark may be
            # build in tree. In order to handle this we use lndir to create a
            # symlink to the source directory in the build directory, and then
            # build in there instead.
            orig_src_dir = str(src_dir)
            src_dir = os.path.join(build_dir, 'src')
            os.makedirs(src_dir)
            os.chdir(src_dir)
            ret = subprocess.call(['lndir', orig_src_dir])
            if ret != 0:
                print ('-- Failed to use lndir to create symlink to source')
                return False

            # If the command refers to a script in the current working directory
            # then make the path absolute so that we can execute it. Otherwise
            # assume that it's a command on the path
            cmd_path = build_system
            if not os.path.isabs(cmd_path):
                cmd_path = os.path.join(base_dir, build_system)

            if os.path.exists(cmd_path):
                cmd_name = cmd_path
            elif is_command_on_path(build_system):
                cmd_name = build_system
            else:
                print ('-- Build system \'' + build_system + '\' is not a known keyword or executable script')
                return False

            cmd = [cmd_name,
                   '--src-dir', src_dir,
                   '--build-dir', build_dir,
                   '--install-dir', install_dir,
                   '--cc', cc,
                   '--cxx', cxx,
                   '--fort', fort,
                   '--flags', flags]
            ret = subprocess.call(cmd)
            if ret != 0:
                print ('-- Failed to build using custom build script \'' + build_system + '\'')
                return False
    return True


def feature_extract(src_dir, build_dir, install_dir, build_system, cc, cxx,
                    fort, flags, database_path, gcc_plugin_path, debug,
                    out_path):
    assert(os.path.exists(src_dir) and os.path.isabs(src_dir))
    assert(os.path.exists(build_dir) and os.path.isabs(build_dir))
    assert(os.path.exists(install_dir) and os.path.isabs(install_dir))
    assert(is_command_on_path(cc))
    assert(is_command_on_path(cxx))
    assert(is_command_on_path(fort))
    assert(os.path.exists(database_path))
    assert(os.path.exists(gcc_plugin_path))
    
    gcc_plugin_name = get_gcc_plugin_name()
    plugin_flags = '-fplugin=' + gcc_plugin_path
    if debug:
        plugin_flags += ' -fplugin-arg-' + gcc_plugin_name + '-debug'
    plugin_flags += ' -fplugin-arg-' + gcc_plugin_name + '-database=' + database_path
    plugin_flags += ' -fplugin-arg-' + gcc_plugin_name + '-out=' + out_path

    new_flags = plugin_flags + ' ' + flags

    print ('-- Performing feature extraction')
    return build(src_dir=src_dir,
                 build_dir=build_dir,
                 install_dir=install_dir,
                 build_system=build_system,
                 cc=cc,
                 cxx=cxx,
                 fort=fort,
                 flags=new_flags)

