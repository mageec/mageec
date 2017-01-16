#!/bin/env python3

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


def build(src_dir, build_dir, install_dir, build_system, cc, cflags):
    base_dir = os.getcwd()

    assert(os.path.exists(src_dir) and os.path.isabs(src_dir))
    assert(os.path.exists(build_dir) and os.path.isabs(build_dir))
    assert(os.path.exists(install_dir) and os.path.isabs(install_dir))
    assert(is_command_on_path(cc))

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

    os.chdir(build_dir)
    try:
        if build_system == 'cmake' or build_system == 'configure':
            if build_system == 'cmake':
                if not is_command_on_path('cmake'):
                    print ('-- cmake command not on path')
                    raise Exception

                cmd = ['cmake', src_dir, '-G', 'Unix Makefiles']
                cmd.append('-DCMAKE_C_COMPILER=' + cc)
                cmd.append('-DCMAKE_C_FLAGS=' + cflags)
                cmd.append('-DCMAKE_INSTALL_PREFIX=' + install_dir)
            elif build_system == 'configure':
                cmd = [os.path.join(src_dir, 'configure')]
                cmd.append('CC=' + cc)
                cmd.append('CFLAGS=' + cflags)
                cmd.append('--prefix=' + install_dir)
    
            # configure the build
            ret = subprocess.call(cmd)
            print ('-- Configuring with: \'' + ' '.join(cmd) + '\'')
            if ret != 0:
                print ('-- Failed to configure \'' + src_dir + '\' using CMake')
                raise Exception

            # Both cmake and configure will generate Makefiles
            if not is_command_on_path('make'):
                print ('-- make command not on path')
                raise Exception

            cmd = ['make']
            ret = subprocess.call(cmd)
            if ret != 0:
                print ('-- Failed to build source \'' + src_dir + '\' using CMake')
                raise Exception

            cmd = ['make', 'install']
            ret = subprocess.call(cmd)
            if ret != 0:
                print ('-- Failed to install build \'' + build_dir + '\' to \'' + install_dir)
                raise Exception
        else:
            assert False, 'custom build script not supported yet'
    except:
        os.chdir(base_dir)
        return False
    return True


def feature_extract(src_dir, build_dir, install_dir, build_system, cc, cflags,
                    database_path, gcc_plugin_path, debug, out_path):
    assert(os.path.exists(src_dir) and os.path.isabs(src_dir))
    assert(os.path.exists(build_dir) and os.path.isabs(build_dir))
    assert(os.path.exists(install_dir) and os.path.isabs(install_dir))
    assert(is_command_on_path(cc))
    assert(os.path.exists(database_path))
    assert(os.path.exists(gcc_plugin_path))
    
    gcc_plugin_name = get_gcc_plugin_name()
    plugin_flags = '-fplugin=' + gcc_plugin_path
    if debug:
        plugin_flags += ' -fplugin-arg-' + gcc_plugin_name + '-debug'
    plugin_flags += ' -fplugin-arg-' + gcc_plugin_name + '-database=' + database_path
    plugin_flags += ' -fplugin-arg-' + gcc_plugin_name + '-out=' + out_path

    new_cflags = plugin_flags + ' ' + cflags

    print ('-- Performing feature extraction')
    return build(src_dir=src_dir,
                 build_dir=build_dir,
                 install_dir=install_dir,
                 build_system=build_system,
                 cc=cc,
                 cflags=new_cflags)

