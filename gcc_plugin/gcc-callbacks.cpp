/*  MAGEEC GCC Plugin Callbacks
    Copyright (C) 2013 Embecosm Limited and University of Bristol

    This file is part of MAGEEC.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>. */

/** @file gcc-callbacks.cpp GCC Plugin Callbacks. */
/* We need to undefine these as gcc-plugin.h redefines them */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "gcc-plugin.h"
#include "mageec-plugin.h"
#include "mageec.h"
#include <stdio.h>

void mageec_gcc_finish (void *gcc_data __attribute__((unused)),
			void *user_data __attribute__((unused)))
{
  printf ("GCC:     Finish\n");
  mageec_inst.finish();
}

void dummy_callback (void *gcc_data, void *user_data)
{
  printf ("Dummy Called\n");
  printf (" gcc_data:  %p\n", gcc_data);
  printf (" user_data: %p\n", user_data);
}

void mageec_gcc_start_file (void *gcc_data, void *user_data)
{
  mageec_inst.new_file ("example.c");
  printf ("GCC:     Start File\n");
  printf ("          gcc_data:  %p\n", gcc_data);
  printf ("          user_data: %p\n", user_data);

  /* FIXME: Register as a pass to run next or run directly? */
  //mageec_gcc_feature_extract();
}

void mageec_gcc_finish_file (void *gcc_data, void *user_data)
{
  mageec_inst.end_file ();
  printf ("GCC:     End File\n");
  printf ("          gcc_data:  %p\n", gcc_data);
  printf ("          user_data: %p\n", user_data);
}
