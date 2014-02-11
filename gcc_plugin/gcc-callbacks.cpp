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
#include "tree-pass.h"
#include "function.h"
#include "mageec-plugin.h"
#include "mageec/mageec.h"
#include <string>
#include <stdio.h>

void mageec_gcc_finish (void *gcc_data __attribute__((unused)),
      void *user_data __attribute__((unused)))
{
  fprintf (stderr, "GCC:     Finish\n");
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
  fprintf (stderr, "GCC:     Start File\n");
  fprintf (stderr, "          gcc_data:  %p\n", gcc_data);
  fprintf (stderr, "          user_data: %p\n", user_data);
}

void mageec_gcc_finish_file (void *gcc_data, void *user_data)
{
  mageec_inst.end_file ();
  fprintf (stderr, "GCC:     End File\n");
  fprintf (stderr, "          gcc_data:  %p\n", gcc_data);
  fprintf (stderr, "          user_data: %p\n", user_data);
}

/**
 * Get string representing type of pass.
 * @param pass compiler pass
 * @returns string corresponding to type of pass (GIMPLE, RTL, etc.)
 */
static std::string pass_type_str(opt_pass* pass)
{
  if (pass == NULL)
    return "*NULL*";
  switch (pass->type)
  {
    case GIMPLE_PASS:
      return "GIMPLE";
      break;
    case RTL_PASS:
      return "RTL";
      break;
    case SIMPLE_IPA_PASS:
      return "SIMPLE_IPA";
      break;
    case IPA_PASS:
      return "IPA";
      break;
    default:
      return "*UNKNOWN*";
      break;
  }
  return "*UNKNOWN*";
}

/**
 * Use MAGEEC framework to decide whether to execute pass.
 */
void mageec_pass_gate (void *gcc_data,
                       void *user_data __attribute__((unused)))
{
  short *result = (short *)gcc_data;
  if (mageec_print_pass_info)
    fprintf (stderr, "Pass: '%s',  Type: %s,  Function: '%s',  Gate: %hi\n",
             current_pass->name, pass_type_str(current_pass).c_str(),
             current_function_name(), *result);

  if (!mageec_no_decision)
  {
    mageec::decision d = mageec_inst.make_decision(current_pass->name,
                                                   current_function_name());
    switch (d) {
      case mageec::NATIVE_DECISION:
        return;
      case mageec::FORCE_EXECUTE:
        *result = (short)1;
        break;
      case mageec::FORCE_NOEXECUTE:
        *result = (short)0;
    }
    if (mageec_print_pass_info)
      fprintf (stderr, "  New gate: %hi\n", *result);
  }
}
