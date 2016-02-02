#!/usr/bin/env python3

import argparse
import os
import shutil
import subprocess
import sys


# UUID for the mageec FileML machine learner which is used to set the
# configuration of the compiler.
fileml_uuid = '8998d958-5d9c-4b6b-a90b-679d8722d63a' 


def build_benchmark(gcc_plugin_path, benchmark_dir, config_path, db_path,
                    out_dir):
  # make paths absolute
  gcc_plugin_path = os.path.abspath(gcc_plugin_path)
  benchmark_dir   = os.path.abspath(benchmark_dir)
  config_path     = os.path.abspath(config_path)
  db_path         = os.path.abspath(db_path)
  out_dir         = os.path.abspath(out_dir)

  print(
    '-- Benchmark build parameters:',
    ' |- GCC plugin path:   ' + gcc_plugin_path,
    ' |- Benchmark sources: ' + benchmark_dir,
    ' |- Compiler config:   ' + config_path,
    ' |- Database file:     ' + db_path,
    ' |- Output directory:  ' + out_dir,
    sep='\n'
  )

  build_dir   = os.path.join(out_dir, 'build')
  install_dir = os.path.join(out_dir, 'install')

  # Check that the output directory doesn't already exist. We don't want to
  # clobber a previous build.
  if os.path.exists(out_dir):
    print ('-- Output directory already exists and would be clobbered... '
           'Aborting')
    return False
  os.makedirs(out_dir)
  os.makedirs(build_dir)
  os.makedirs(install_dir)

  # Copy the configuration to the ouput directory, update the config path to
  # point to this copy
  shutil.copy(config_path, out_dir)
  config_file = os.path.basename(config_path)
  config_path = os.path.join(out_dir, config_file)

  # Store the compilation ids and any debug output from the build
  debug_path = os.path.join(out_dir, 'build.log')
  compilation_ids_path = os.path.join(out_dir, 'compilation.ids')

  # Setup cflags and the compiler for building the benchmark using FileML
  # FIXME: This will cause the build system to spit out compilation ids for
  # any tests it runs too.
  cflags = [
    '-g', '-O3', '-fplugin=' + gcc_plugin_path,
    '-fplugin-arg-libmageec_gcc-debug',
    '-fplugin-arg-libmageec_gcc-database=' + db_path,
    '-fplugin-arg-libmageec_gcc-feature-extract',
    '-fplugin-arg-libmageec_gcc-out-file=' + compilation_ids_path,
    '-fplugin-arg-libmageec_gcc-ml=' + fileml_uuid,
    '-fplugin-arg-libmageec_gcc-ml-config=' + config_path,
    '-fplugin-arg-libmageec_gcc-optimize'
  ]
  c_compiler = 'gcc'

  # Build with cmake or autoconf. If neither is detected error
  # FIXME: Target something other than just Makefiles?
  cmake_lists_path = os.path.join(benchmark_dir, 'CMakeLists.txt')
  configure_path   = os.path.join(benchmark_dir, 'configure')

  if os.path.exists(cmake_lists_path):
    print ('-- CMakeLists.txt detected... Building benchmark with cmake')

    orig_cwd = os.getcwd()
    os.chdir(build_dir)

    cmd = [
      'cmake', benchmark_dir,
      '-DCMAKE_C_COMPILER=' + c_compiler,
      '-DCMAKE_C_FLAGS=' + ' '.join(cflags),
      '-DCMAKE_INSTALL_PREFIX=' + install_dir
    ]
    print ('-- Configuring benchmarks with command: ', ' '.join(cmd))
    ret = subprocess.call(cmd, stderr=open(debug_path, 'a'))
    if ret != 0:
      print ('-- Failed to configure benchmarks... Aborting')
      return False

    # build and install
    # FIXME: Makefile specific
    print ('-- Building and installing benchmarks into \'' + out_dir + '\'')
    ret = subprocess.call(['make'], stderr=open(debug_path, 'a'))
    if ret != 0:
      print ('-- Failed to install benchmark in \'' + build_dir + '\'... '
             'Aborting')
      return False

    ret = subprocess.call(['make', 'install'], stderr=open(debug_path, 'a'))
    if ret != 0:
      print ('-- Failed to install benchmark to \'' + install_dir + '\'... '
             'Aborting')
      return False

    # restore to the original working directory
    os.chdir(orig_cwd)

  elif os.path.exists(configure_path):
    print ('-- configure detected... Building benchmark with autoconf')

    orig_cwd = os.getcwd()
    os.chdir(build_dir)

    cmd = [configure_path, '--prefix=' + install_dir]
    print ('-- Configuring benchmarks with command: ', ' '.join(cmd))
    ret = subprocess.call(cmd, stderr=open(debug_path, 'a'))
    if ret != 0:
      print ('-- Failed to configure benchmarks... Aborting')
      return False

    # build and install
    print ('-- Building and installing benchmarks into \'' + out_dir + '\'')
    make_cmd = [
      'make', 
      'CFLAGS=' + ' '.join(cflags),
      'CC=' + c_compiler
    ]
    ret = subprocess.call(make_cmd, stderr=open(debug_path, 'a'))
    if ret != 0:
      print ('-- Failed to install benchmark in \'' + build_dir + '\'... '
             'Aborting')
      return False

    ret = subprocess.call(['make', 'install'], stderr=open(debug_path, 'a'))
    if ret != 0:
      print ('-- Failed to install benchmark to \'' + install_dir + '\'... '
             'Aborting')
      return False

    # restore to the original cwd
    os.chdir(orig_cwd)

  else:
    print ('-- No supported build system detected... Aborting')
    return False

  return True


def main():
  # Parse in the arguments
  parser = argparse.ArgumentParser(
      description='Generate a build of a provided benchmark suite')
  parser.add_argument('--gcc-plugin-path', nargs=1, required=True,
      help='Path of the MAGEEC gcc plugin')
  parser.add_argument('--benchmark', nargs=1, required=True,
      help='Directory of the benchmark to be built')
  parser.add_argument('--database',  nargs=1, required=True,
      help='Path of the database that extracted results will be added to')
  parser.add_argument('--config',    nargs=1, required=True,
      help='FileML configuration to be provided to the compiler')
  parser.add_argument('--out-dir',   nargs=1, required=True,
      help='Directory for the benchmark build and tool output')

  args = parser.parse_args(sys.argv[1:])
  gcc_plugin_path = args.gcc_plugin_path[0]
  benchmark_dir   = args.benchmark[0]
  db_path         = args.database[0]
  config_path     = args.config[0]
  out_dir         = args.out_dir[0]

  ret_code = build_benchmark(gcc_plugin_path, benchmark_dir, config_path,
                             db_path, out_dir)
  if not ret_code:
    return -1
  return 0


if __name__ == '__main__':
  main()
