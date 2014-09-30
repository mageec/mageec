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

ROOT=$(dirname $0)

cd ${ROOT}/run-$1

date > output.log
export MAGEEC_EXECUTELIST=${ROOT}/run-$1/PASSES_TO_RUN

OTHER_CFLAGS="-save-temps -fplugin=/usr/local/lib/libmageec_gcc.so -fplugin-arg-libmageec_gcc-dumppasses -O2" \
	${ROOT}/beebs/configure --host=avr --with-chip=atmega328p \
	--with-board=shrimp >> output.log 2>&1
if [ $? -gt 0 ]; then
	echo "Configure Failed" >> output.log 2>&1
	exit 1
fi

# We give the -k flag to make as we would expect some tests to fail to build
# at this stage, but want to build as many as possible.
# FIXME: Should this be removed for stable release, as this is really only
# for testing the framework and will almost always result in some failures?
make -k >> output.log 2>&1
gzip -9 output.log

