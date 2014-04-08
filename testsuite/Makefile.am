#   Copyright (C) 2013, 2014 Embecosm Limited and University of Bristol

#   This file is part of MAGEEC.

#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.

#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.

#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.

# Initilize DejaGnu
AUTOMAKE_OPTIONS = dejagnu

# Include tests with distribution
EXTRA_DIST = config/unix.exp lib/mageec.exp

# Set extra configuration variables in this file
EXTRA_DEJAGNU_SITE_CONFIG = config/site.exp

# Run tests in seperate tools
DEJATOOL = mageec

SUBDIRS = mageec.tests

if ENABLE_GCC_PLUGIN
DEJATOOL += gcc_plugin
SUBDIRS += gcc_plugin.tests
endif

if ENABLE_PYTHON
DEJATOOL += python
SUBDIRS += python.tests
endif
