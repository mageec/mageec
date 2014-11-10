#!/bin/bash

#   Second level MAGEEC Build Script
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
# (This script is meant to be called by task spooler to run a job)
# Usage: "test-pt2.sh RUNNUMBER"

if [ "x$INMAGEEC" != "xTRUE" ]; then
	echo "This script should be run via the top level test.py" > /dev/stderr
	exit 1
fi

# base directory of the current test run which we are in
ROOT=`pwd`

# script directory
EXECDIR=$(dirname $0)

# BEEBSBASE should have been set prior to running test.py
if [ "xBEEBSBASE" == "x" ]; then
    echo "BEEBSBASE not set, cannot continue" >> build.log 2>&1
    exit 1
fi

#cd ${ROOT}/run-$1

date > build.log
export MAGEEC_EXECUTELIST=${ROOT}/PASSES_TO_RUN

# When using the clang driver script, CFLAGS="-O3" is not necessary here as it
# adds the passes itself. For other configurations where passes are selectively
# removed rather than added it will be necessary
${BEEBSBASE}/configure --host=arm-none-eabi --with-board=stm32vldiscovery  \
  --with-chip=stm32f100 CC="${EXECDIR}/clang.py -target arm-none-eabi" \
  >> build.log 2>&1
if [ $? -gt 0 ]; then
	echo "Configure Failed" >> build.log 2>&1
	exit 1
fi

# We now forcibly export the use of FILEML now we have configured
# When using the clang driver script, we do not want this defined as we handle
# the database creation ourself since currently creating it through LLVM does
# not work.
#export MAGEEC_MODE=FILEML

# We give the -k flag to make as we would expect some tests to fail to build
# at this stage, but want to build as many as possible.
# FIXME: Should this be removed for stable release, as this is really only
# for testing the framework and will almost always result in some failures?
make -k V=1 >> build.log 2>&1
#gzip -9 build.log

# Now to run tests...
# We first run our test to calibrate the board
date > execute.log
echo "Basic test" >> execute.log
platformrun -v stm32vldiscovery ${BEEBSBASE}/basic-2dfir >> execute.log 2>&1
echo "Basic test 2" >> execute.log
platformrun -v stm32vldiscovery ${BEEBSBASE}/basic-2dfir >> execute.log 2>&1

# Now run all the tests that managed to compile
(
  for i in $(find src -executable -type f); do
    echo $i;
    platformrun -v stm32vldiscovery $i;
  done
) 2>&1 | tee -a execute.log

# Run one final basic test to finish the test
echo "Basic test 3" >> execute.log
platformrun -v stm32vldiscovery ${BEEBSBASE}/basic-2dfir >> execute.log 2>&1

#gzip -9 execute.log

