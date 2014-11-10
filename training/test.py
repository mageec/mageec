#!/usr/bin/env python2

#   MAGEEC Training Folder Preparation
#
#   Copyright (C) 2014 Embecosm Limited and University of Bristol
#
#   This file is part of MAGEEC.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

# FIXME: Doesn't (yet) support automatic resuming

import subprocess, sys, os, re

# Size of our matrix, this should equal the number of entries in
# _MAYBE_RUN_PASSES as these are the passes which can be toggled
_N_ENTRIES = 54

# Index for resuming
_startfrom = 0


# Pass setup
#
# _ALWAYS_RUN_PASSES is only relevant to GCC and contains a newline separated
# string of passes which should always be run
#
# _MAYBE_RUN_PASSES contains a list of passes (for gcc) or flags (for llvm)
# which may be toggled on or off by the compiler.

# Passes to always run
_ALWAYS_RUN_PASSES="""
"""

# Passes that may be toggled on or off to produce a training configuration
_MAYBE_RUN_PASSES = [
'-adce',
'-always-inline',
'-argpromotion',
'-bb-vectorize',
'-break-crit-edges',
'-codegenprepare',
'-constmerge',
'-constprop',
'-dce',
'-deadargelim',
'-die',
'-dse',
'-functionattrs',
'-globaldce',
'-globalopt',
'-gvn',
'-indvars',
'-inline',
'-instcombine',
'-internalize',
'-ipconstprop',
'-ipsccp',
'-jump-threading',
'-lcssa',
'-licm',
'-loop-deletion',
'-loop-extract',
'-loop-extract-single',
'-loop-reduce',
'-loop-rotate',
'-loop-simplify',
'-loop-unroll',
'-loop-unswitch',
'-loweratomic',
'-lowerinvoke',
'-lowerswitch',
'-mem2reg',
'-memcpyopt',
'-mergefunc',
'-mergereturn',
'-partial-inliner',
'-prune-eh',
'-reassociate',
'-reg2mem',
'-scalarrepl',
'-sccp',
'-simplifycfg',
'-sink',
'-strip',
'-strip-dead-debug-info',
'-strip-dead-prototypes',
'-strip-debug-declare',
'-strip-nondebug',
'-tailcallelim'
]

# BEEBSBASE should be set to the base directory of BEEBS
if not os.getenv('BEEBSBASE', None):
    sys.stderr.write('Environment variable BEEBSBASE not set, exiting\n')
    sys.exit(1)

# Should be a matrix to use in the current directory
try:
    _inputfile = open('matrix','r')
except:
    sys.stderr.write('Unable to open matrix of pass configurations\n')
    sys.exit(1)

EXECDIR  = os.path.dirname(os.path.realpath(__file__))
STARTDIR = os.getcwd()

# We use this as a guard to prevent accidental execution of test-exec, so
# set this here.
os.environ['INMAGEEC'] = "TRUE"

if len(_MAYBE_RUN_PASSES) != _N_ENTRIES:
  sys.stderr.write("Number of entries != Number of possible passes\n")
  sys.exit(1)

for line in _inputfile:
  # Remove newline
  if line[-1] == '\n':
    line = line[:-1]
  
  test = re.split(' *', line)
  print ' * Running test set %s' % test[0]
  if int(test[0]) < _startfrom:
    continue
  dirname = ''
  for i in range(1, _N_ENTRIES + 1):
    dirname = dirname + test[i]

  # Try and run this test if it doesnt already exist
  try:
    os.mkdir(STARTDIR + '/run-' + dirname)
    os.chdir(STARTDIR + '/run-' + dirname)

    # Save the newline separated string of passes/flags to be run
    testlist = open('PASSES_TO_RUN','w')
    testlist.write(_ALWAYS_RUN_PASSES)
    for i in range(1, _N_ENTRIES + 1):
      if test[i] == '1':
        testlist.write(_MAYBE_RUN_PASSES[i - 1])
        testlist.write('\n')
    testlist.close()

    # Configure, build and execute
    subprocess.call([EXECDIR + '/test-exec.sh', dirname])
  except:
    print '   FAILED! %s' % dirname

