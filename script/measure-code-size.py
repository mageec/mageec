#!/usr/bin/env python3

import argparse
import os
import stat
import shutil
import subprocess
import sys

from elftools.common.py3compat import itervalues
from elftools.elf.elffile import ELFFile
from elftools.elf.elffile import SymbolTableSection

# Measure the code size of executables produced by a benchmark, and try and
# associate those results with compilation ids generated when building that
# benchmark. If code size data can be extracted, store the calculated size
# in the database


class file_compilation_ids:
  module = None
  functions = []

  def __init__(self, module, functions):
    self.module = module
    self.functions = functions

  def __str__(self):
    s = ['module,' + self.module[0] + ',' + self.module[1]]
    for f in self.functions:
      s.append('function' + ',' + f[0] + ',' + f[1])
    return ''.join(s)


def read_compilation_ids(input_file):
  src_files = {}

  # Parameters of the current source file in the input file
  src_path = ''
  module = None
  funcs = []

  for line in open(input_file):
    values = line.split(",")
    entry_type = values[0]

    if entry_type == 'file':
      if src_path:
        # Save previous entry if there is one
        #assert not src_files[src_path], 'Multiple entries for this source file'
        src_files[src_path] = file_compilation_ids(module, funcs)
      else:
        assert not module
        assert not funcs
      # start a new entry
      src_path = values[1].strip().encode()
      module = None
      funcs = []

    elif entry_type == 'module':
      assert src_path
      assert not module
      mod_name = values[1].strip().encode()
      mod_id   = values[2].strip()
      module = (mod_name, mod_id)

    elif entry_type == 'function':
      assert src_path
      func_name = values[1].strip().encode()
      func_id   = values[2].strip()
      funcs.append( (func_name, func_id) )

  # save the final entry
  if src_path:
    #assert not src_files[src_path], 'Multiple entries for this source file'
    src_files[src_path] = file_compilation_ids(module, funcs)

  return src_files


# Get the source files used to produce the given executable file
# Returns a list of the constituent source files
def get_exec_src_files(exec_file):
  exec_srcs = []
  with open(exec_file, 'rb') as elf_file_handle:
    try:
      elf_file = ELFFile(elf_file_handle)
    except:
      print ("Not an elf file")
      return []

    if not elf_file.has_dwarf_info():
      print ('Executable file \'' + exec_file + '\' has no DWARF info')
      return []

    dwarf_info = elf_file.get_dwarf_info()
    for CU in dwarf_info.iter_CUs():
      DIE = CU.get_top_DIE()
      for attr in itervalues(DIE.attributes):
        if attr.name == 'DW_AT_name':
          exec_srcs.append(attr.value)
  return exec_srcs


# Get the size of all of the symbols in the provided executable file
# Return a dictionary mapping the symbol name to its size
def get_exec_sym_sizes(exec_file):
  sym_sizes = {}
  with open(exec_file, 'rb') as elf_file_handle:
    try:
      elf_file = ELFFile(elf_file_handle)
    except:
      print ("Not an elf file")
      return {}

    for section in elf_file.iter_sections():
      if not isinstance(section, SymbolTableSection):
        continue

      for _, symbol in enumerate(section.iter_symbols()):
        sym_sizes[symbol.name] = symbol['st_size']
  return sym_sizes


def is_command_on_path(cmd):
  for path in os.environ['PATH'].split(os.pathsep):
    path = path.strip('"')
    exec_file = os.path.join(path, cmd)
    if os.path.isfile(exec_file) and os.access(exec_file, os.X_OK):
      return True
  return False


def measure_benchmark(benchmark_dir, db_path):
  # Make paths absolute
  benchmark_dir = os.path.abspath(benchmark_dir)
  db_path       = os.path.abspath(db_path)

  print (
    '-- Benchmark measure parameters:',
    ' |- Benchmark directory: ' + benchmark_dir,
    ' |- Database path:       ' + db_path,
    sep='\n'
  )

  # Check that mageec is available on the PATH
  if not is_command_on_path('mageec'):
    print ('-- Cannot find \'mageec\' command. Ensure it is on the path')
    return False

  # Path containing the executables to be measured and the compilation ids
  # generated when building the benchmarks.
  exec_dir = os.path.join(benchmark_dir, 'install')
  if not os.path.exists(exec_dir):
    print ('-- No install directory for benchmarks... Aborting')
    return False

  compilation_ids_path = os.path.join(benchmark_dir, 'compilation.ids')
  if not os.path.exists(compilation_ids_path):
    print ('-- No compilation ids for benchmarks... Aborting')
    return False

  # Path to output any debug to when measuring results
  debug_path = os.path.join(benchmark_dir, 'measure.log')
  if os.path.exists(debug_path):
    print ('-- Debug file already exists and would be clobbered... Aborting')
    return False

  # File to store the results data. This will be used later to insert results
  # into the database.
  results_path = os.path.join(benchmark_dir, 'results.txt')
  if os.path.exists(results_path):
    print ('-- Results file already exists and would be clobbered... Aborting')
    return False
  results_handle = open(results_path, 'w')
  

  # Dictionary mapping from source file to compilation ids
  src_file_compilation_ids = read_compilation_ids(compilation_ids_path)

  # Find all executable files in the executable directory
  exec_files = []
  exec_bits = stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH
  for d in os.walk(exec_dir):
    dirpath, _, files = d
    for f in files:
      f = os.path.join(dirpath, f)
      if os.path.isfile(f):
        stat_res = os.stat(f)
        stat_mode = stat_res.st_mode
        if stat_mode & exec_bits:
          # Ignore files which are not ELF
          try:
            elffile = ELFFile(open(f, 'rb'))
          except:
            continue
          exec_files.append(f)

  # Extract results data from executable files
  for exec_file in exec_files:
    # A list of all of the files which make up the executable
    exec_src_files = get_exec_src_files(exec_file)

    # A dictionary of all of the symbols and their sizes in the executable
    sym_sizes = get_exec_sym_sizes(exec_file)

    # Check if there are any compilation ids for each file referenced by the
    # executable.
    for exec_src_file in exec_src_files:
      if exec_src_file not in src_file_compilation_ids:
        print ('-- File referenced in executable with no associated '
               ' compilation ids: \'' + exec_src_file + '\'... Ignoring')
        continue

      src_file_ids = src_file_compilation_ids[exec_src_file]
      module_id = src_file_ids.module
      func_ids = src_file_ids.functions

      # try and find symbols for each function with a compilation id
      # accumulate the function sizes so that we have a data point for the
      # module too
      mod_size = 0
      for func_name, func_id in func_ids:
        if func_name in sym_sizes:
          func_size = sym_sizes[func_name]

          # We have the compilation id and its size, so emit it into the
          # results file.
          results_handle.write(func_id + ',size,' + str(func_size) + '\n')

          # Accumulate module size too
          mod_size = mod_size + func_size
        else:
          # We have a function with a compilation id, but it doesn't appear
          # in the executable. Could just have been inlined (or could be a bug)
          print ('-- Function \'' + func_name.decode() + '\' appeared in a '
                 'compilation but not in the final executable')

      # Now we have the module size too, so we can emit it into the results
      if module_id:
        _, mod_id = module_id
        results_handle.write(mod_id + ',size,' + str(mod_size) + '\n')

  # Now the results are accumulated, output them into the database
  # Make sure the results file is closed first
  results_handle.close()

  mageec_cmd = ['mageec', db_path, '--add-results', results_path]
  print ('-- Adding results to database with command: ', ' '.join(mageec_cmd))
  ret = subprocess.call(mageec_cmd, stderr=open(debug_path, 'a'))
  if ret != 0:
    print ('-- Failed to insert results into the database... Aborting')
    return False

  return True
  




'''
  print(
    'Benchmark measure parameters:',
    '  Benchmark directory: ' + benchmark_dir,
    '  Results file:        ' + results_path,
    sep='\n'
  )

  # make paths absolute
  benchmark_dir = os.path.abspath(benchmark_dir)
  results_path = os.path.abspath(results_path)

  results_handle = open(results_path, 'w')

  # Path to the file containing the compilation ids for the benchmarks
  compilation_ids_path = os.path.join(benchmark_dir, 'compilation.ids')

  # Dictionary mapping from a source file to compilation ids
  src_file_compilation_ids = read_compilation_ids(compilation_ids_path)

  # Find all executable files in the benchmark directory
  exec_files = []
  exec_bits = stat.S_IEXEC | stat.S_IXGRP | stat.S_IXOTH
  for d in os.walk(benchmark_dir):
    dirpath, _, files = d
    for f in files:
      f = os.path.join(dirpath, f)
      if os.path.isfile(f):
        stat_res = os.stat(f)
        stat_mode = stat_res.st_mode
        if stat_mode & exec_bits:
          try:
            elffile = ELFFile(open(f, 'rb'))
          except:
            continue
          exec_files.append(f)

  # Extract results data from executable files
  for exec_file in exec_files:
    # A list of all of the files which make up the executable
    exec_src_files = get_exec_src_files(exec_file)

    # A dictionary of all of the symbols and their sizes in the executable
    sym_sizes = get_exec_sym_sizes(exec_file)

    # Check if there are any compilation ids sourced from each file referenced
    # in the executable.
    for exec_src_file in exec_src_files:
      if exec_src_file not in src_file_compilation_ids:
        print (exec_src_file)
        print (src_file_compilation_ids.keys())
        continue

      src_file_ids = src_file_compilation_ids[exec_src_file]
      module_id = src_file_ids.module
      func_ids = src_file_ids.functions

      # try and find symbols for each function with a compilation id
      # accumulate the function sizes so that we have a data point for the
      # module too
      mod_size = 0
      for func_name, func_id in func_ids:
        if func_name in sym_sizes:
          func_size = sym_sizes[func_name]

          # We now have the compilation id and its size. So we can spit it
          # out into a results file
          results_handle.write(func_id + ',size,' + str(func_size) + '\n')

          # Accumulate module size too
          mod_size = mod_size + func_size
        else:
          # We have a function with a compilation id, but it doesn't appear
          # in the executable. Might not be externally visible, or could be
          # a bug.
          print ("Function '" + func_name.decode() + "' appeared in a "
                 "compilation but not in the final executable")

      # Now have the module size too, so we can spit it out into a results
      # file.
      if module_id:
        _, mod_id = module_id
        results_handle.write(mod_id + ',size,' + str(mod_size) + '\n')
'''


def main():
  parser = argparse.ArgumentParser(
      description='Measure the code size of a provided benchmark build. This '
                  'will produce a new file containing the results for each of '
                  'the executables')
  parser.add_argument('--exec-dir', nargs=1, required=True,
      help='Directory of the built and installed benchmarks')
  parser.add_argument('--database', nargs=1, required=True,
      help='Database which the results will be added to')
  args = parser.parse_args(sys.argv[1:])

  benchmark_dir = args.exec_dir[0]
  db_path       = args.database[0]

  ret_code = measure_benchmark(benchmark_dir, db_path)

  if not ret_code:
    return -1
  return 0

'''
  parser = argparse.ArgumentParser(
    description='Extract benchmark results to file')
  parser.add_argument('--benchmark_dir', nargs=1, required=True)
  parser.add_argument('--results_file', nargs=1, required=True)
  args = parser.parse_args(sys.argv[1:])

  benchmark_dir = args.benchmark_dir[0]
  results_path = args.results_file[0]

  return measure_benchmark(benchmark_dir, results_path)
'''

if __name__ == '__main__':
  main()
