#!/usr/bin/env python3

import argparse
import os
import subprocess
import sys


def main():
  parser = argparse.ArgumentParser(
      description='Generate a build of a provided benchmark suite')
  parser.add_argument('--generator-script', nargs=1, required=True,
      help='Script to use to generate the benchmark build')
  parser.add_argument('--measure-script',   nargs=1, required=True,
      help='Script to use to measure the performance of the benchmark')
  parser.add_argument('--gcc-plugin-path',  nargs=1, required=True,
      help='Path to the MAGEEC gcc plugin')
  parser.add_argument('--benchmark',        nargs=1, required=True,
      help='Directory of the benchmark to be built')
  parser.add_argument('--database',         nargs=1, required=True,
      help='Path of the database features and results are added to')
  parser.add_argument('--config',           nargs=1, required=True,
      help='FileML configuration to be provided to the compiler')
  parser.add_argument('--out-dir',          nargs=1, required=True,
      help='Directory for the benchmark build and intermediate output')

  args = parser.parse_args(sys.argv[1:])
  generator       = args.generator_script[0]
  measurer        = args.measure_script[0]
  gcc_plugin_path = args.gcc_plugin_path[0]
  benchmark_dir   = args.benchmark[0]
  db_path         = args.database[0]
  config_path     = args.config[0]
  out_dir         = args.out_dir[0]

  generate_cmd = [
    generator,
    '--gcc-plugin-path',  gcc_plugin_path,
    '--benchmark',        benchmark_dir,
    '--database',         db_path,
    '--config',           config_path,
    '--out-dir',          out_dir
  ]
  print ('-- Generate benchmark with command: ' + ' '.join(generate_cmd))
  ret = subprocess.call(generate_cmd)
  if ret != 0:
    print ('-- Failed to generate benchmark set... Aborting')
    return -1

  measure_cmd = [
    measurer,
    '--exec-dir', out_dir,
    '--database', db_path
  ]
  print ('-- Measuring benchmark with command: ' + ' '.join(measure_cmd))
  ret = subprocess.call(measure_cmd)
  if ret != 0:
    print ('-- Failed to measure benchmark set... Aborting')
    return -1

  return 0;


if __name__ == '__main__':
  main()
