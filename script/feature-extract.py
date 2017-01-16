#!/bin/env python3

import argparse
import os
import sys
import mageec


def main():
    parser = argparse.ArgumentParser(
        description='Perform feature extraction')

    # required arguments
    parser.add_argument('--src-dir', nargs=1, required=True,
        help='Directory containing the source to build')
    parser.add_argument('--build-dir', nargs=1, required=True,
        help='Build directory')
    parser.add_argument('--install-dir', nargs=1, required=True,
        help='Install directory')
    parser.add_argument('--cc', nargs=1, required=True,
        help='Command to use to compile the source')
    parser.add_argument('--database', nargs=1, required=True,
        help='mageec database to store extracted features into')
    parser.add_argument('--mageec-library-path', nargs=1, required=True,
        help='Path to the directory holding the mageec libraries')
    parser.add_argument('--out', nargs=1, required=True,
        help='File to output the extracted features to')

    # optional arguments
    parser.add_argument('--debug', required=False,
        help='Enable debug when doing feature extraction')
    parser.add_argument('--build-system', nargs=1, required=False,
        help='Build system to be used to build the source. May be \'cmake\', '
             '\'configure\', or a script to be used to build the source')
    parser.add_argument('--cflags', nargs=1, required=False,
        help='Common arguments to be used when building')

    args = parser.parse_args(sys.argv[1:])
    src_dir         = os.path.abspath(args.src_dir[0])
    build_dir       = os.path.abspath(args.build_dir[0])
    install_dir     = os.path.abspath(args.install_dir[0])
    cc              = args.cc[0]
    database_path   = os.path.abspath(args.database[0])
    mageec_lib_path = os.path.abspath(args.mageec_library_path[0])
    out_path        = os.path.abspath(args.out[0])

    build_system = None
    if args.build_system:
        build_system = args.build_system[0]
    cflags = ''
    if args.cflags:
        cflags = args.cflags[0]
    debug = args.debug

    # TODO: Extension should depend on platform
    gcc_plugin_name = mageec.get_gcc_plugin_name() + '.so'

    if not os.path.exists(src_dir):
        print ('-- Source directory ' + src_dir + ' does not exist')
        return -1
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    if not os.path.exists(install_dir):
        os.makedirs(install_dir)
    if not os.path.exists(database_path):
        print ('-- Database ' + database_path + ' does not exist')
        return -1

    if not mageec.is_command_on_path(cc):
        print ('-- Compiler ' + cc + ' is not on the path')
        return -1

    print ('-- Checking for mageec plugin ' + gcc_plugin_name)
    gcc_plugin_path = os.path.join(mageec_lib_path, gcc_plugin_name)
    if not os.path.exists(gcc_plugin_path):
        print ('-- Could not find gcc plugin')
        return -1

    build_dir = os.path.join(build_dir, 'feature_extract')
    install_dir = os.path.join(install_dir, 'feature_extract')
    if not os.path.exists(build_dir):
        os.makedirs(build_dir)
    if not os.path.exists(install_dir):
        os.makedirs(install_dir)

    res = mageec.feature_extract(src_dir=src_dir,
                                 build_dir=build_dir,
                                 install_dir=install_dir,
                                 build_system=build_system,
                                 cc=cc,
                                 cflags=cflags,
                                 database_path=database_path,
                                 gcc_plugin_path=gcc_plugin_path,
                                 debug=debug,
                                 out_path=out_path)
    if not res:
        return -1
    return 0


if __name__ == '__main__':
    main()
