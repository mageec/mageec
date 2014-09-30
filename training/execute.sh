#!/bin/bash

#   Script to launch parallel testing
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

# Set up ts
TS_MAXCONN=50 TS_SOCKET=/tmp/ts_execute ts

for i in {1..32768}; do
 (
  cd run-$i && TS_SOCKET=/tmp/ts_execute ts bash ../execute-pt2.sh
 )
done
