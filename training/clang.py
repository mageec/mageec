#!/usr/bin/env python2

#   Clang driver script
#
#   This script masquerades as clang in order to do custom handling of the
#   optimization passes. If it is detected that we are compiling from a C
#   source file to an object file we intercept and instead compile in stages,
#   using the PASSES_TO_RUN file as input to opt. opt and clang are found
#   through PATH
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

import subprocess, sys, os, re


obj_file = ''
ir_file  = ''
opt_file = ''
asm_file = ''


# Check that we are compiling from a c source file to a named output file,
# if not the following steps don't make sense so we just default to
# using clang.
#
# The reason for requiring a named output file is because otherwise configure
# mis-detects the default obj file output extension
to_obj   = False
with_obj = False
from_src = False

arg = 1
while arg < len(sys.argv):
    if re.search('-c', sys.argv[arg]):
        to_obj = True
    if re.search('.*\.c$', sys.argv[arg]):
        from_src = True
    if re.search('-o(.*)$', sys.argv[arg]):
        with_obj = True
    arg += 1


# by default just use clang
if not (to_obj and with_obj and from_src):
    compile_args = list(sys.argv)
    compile_args[0] = 'clang'
    print >>sys.stderr, compile_args
    ret = subprocess.call(compile_args)
    sys.exit(ret)


# Use clang to produce llvm ir
# add -S and -emit-llvm to change the output to ir
# Change the output file name, and suppress unused arguments
compile_args = ['clang']
compile_args.append('-Qunused-arguments')

arg = 1
while arg < len(sys.argv):
    res = re.search('-o\s*(\S*)\s*$', sys.argv[arg])
    if res:
        compile_args.append('-o')
        if res.group(1):
            obj_file = res.group(1)
        else:
            arg += 1
            obj_file = sys.argv[arg]

        ir_file  = obj_file + '.ll'
        asm_file = obj_file + '.asm'
        compile_args.append(ir_file)
    elif sys.argv[arg] == '-c':
        compile_args.extend(['-S', '-emit-llvm'])
    else:
        compile_args.append(sys.argv[arg])
    arg += 1


# Do the compilation to ir
try:
    print >>sys.stderr, compile_args
    subprocess.call(compile_args)
except:
    print >>sys.stderr, 'Compilation to LLVM IR failed\n'
    sys.exit(1)


# Get the optimisation flags to run from the filename stored in
# MAGEEC_EXECUTELIST and put them into a list
pass_file = os.getenv('MAGEEC_EXECUTELIST', None)
passes_to_run = ''
if not pass_file:
    print >>sys.stderr, 'MAGEEC_EXECUTELIST not set, cannot find file containing passes to run\n'
    sys.exit(1)

if os.path.isfile(pass_file):
    passes_to_run = open(pass_file, 'r').read()
    passes_to_run = [x for x in re.split('\n*', passes_to_run) if x]
else:
    print >>sys.stderr, 'No file containing passes to run found, default to no passes\n'
    passes_to_run = ''


# build and run the optimize command
opt_file = ir_file + '.opt'
opt_cmd = ['opt', '-debug-pass=Structure', '-S']

# only add passes to run if its not empty
if passes_to_run:
    opt_cmd.extend(passes_to_run)

opt_cmd.extend(['-o', opt_file, ir_file])

try:
    print >>sys.stderr, opt_cmd
    subprocess.call(opt_cmd)
except:
    print >>sys.stderr, 'Optimisation of LLVM IR failed'
    sys.exit(1)


# Compile the optimised llvm ir to assembly
try:
    llc_args = ['llc', '-o', asm_file, opt_file]
    print >>sys.stderr, llc_args
    subprocess.call(['llc', '-o', asm_file, opt_file])
except:
    print >>sys.stderr, 'Assembly of LLVM IR failed'
    sys.exit(1)


# Assemble the objects by using clang
# add -emit-obj and -Qunused-arguments to the flags
# change the source file to be the previously assembled file
asm_args = ['clang']
asm_args.append('-Qunused-arguments')

arg = 1
while arg < len(sys.argv):
    if sys.argv[arg] == '-c':
        asm_args.extend(['-c', '-emit-obj'])
    elif re.match('.*\.c$', sys.argv[arg]):
        asm_args.append(asm_file)
    else:
        asm_args.append(sys.argv[arg])
    arg += 1

try:
    print >>sys.stderr, asm_args
    subprocess.call(asm_args)
except:
    print >>sys.stderr, 'Object file compilation failed'
    sys.exit(1)

