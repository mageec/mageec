/*  MAGEEC GCC Plugin
    Copyright (C) 2013, 2014 Embecosm Limited and University of Bristol

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

/** @file gcc-plugin.cpp GCC Plugin Interface. */
/* We need to undefine these as gcc-plugin.h redefines them */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "gcc-plugin.h"
#include "tree-pass.h"
#include "mageec/mageec.h"
#include "mageec-plugin.h"
//#include <string>

using namespace mageec;

/** Declared to allow GCC to load. */
int plugin_is_GPL_compatible;

/** Plugin information to pass to GCC. */
static struct plugin_info mageec_plugin_version =
{
  .version = "0.1.0",
  .help = NULL
};

static int print_plugin_info = 0;
int mageec_print_pass_info = 0;
int mageec_no_decision = 0;

/** Stored GCC Plugin name for future register_callbacks. */
const char *mageec_gcc_plugin_name;

/** MAGEEC Instance */
mageec_framework mageec_inst;

static void parse_arguments (int argc, struct plugin_argument *argv)
{
  int i;
  for (i = 0; i < argc; i++)
  {
    if (!strcmp (argv[i].key, "plugininfo"))
      print_plugin_info = 1;
    else if (!strcmp (argv[i].key, "dumppasses"))
      mageec_print_pass_info = 1;
    else if (!strcmp (argv[i].key, "nodecide"))
      mageec_no_decision = 1;
    else
      fprintf (stderr, "MAGEEC Warning: Unknown option %s\n", argv[i].key);
  }
}

/**
 * Initilizes GCC Plugin by calling mageec_init.
 * @param plugin_name_args GCC plugin information.
 * @param plugin_gcc_version GCC version information.
 * @returns 0 if MAGEEC successfully set up, 1 otherwise.
 */
int plugin_init (struct plugin_name_args *plugin_info,
                 struct plugin_gcc_version *version)
{
  /* Register our information, parse arguments. */
  register_callback (plugin_info->base_name, PLUGIN_INFO, NULL,
                     &mageec_plugin_version);
  mageec_gcc_plugin_name = plugin_info->base_name;
  parse_arguments (plugin_info->argc, plugin_info->argv);
  if (print_plugin_info)
    mageec_gcc_plugin_info (plugin_info, version);

  /* Initialize MAGEEC Framework, returning error if failed. */
  /* FIXME: Get real compiler version string and target. */
  std::string compiler_version = "GCC-";
  compiler_version += version->basever;
  if (mageec_inst.init (compiler_version, "SOMETARGET"))
    return 1;

  register_callback (plugin_info->base_name, PLUGIN_START_UNIT,
                     mageec_gcc_start_file, NULL);
  register_callback (plugin_info->base_name, PLUGIN_FINISH_UNIT,
                     mageec_gcc_finish_file, NULL);
  register_callback (plugin_info->base_name, PLUGIN_FINISH,
                     mageec_gcc_finish, NULL);
  register_callback (plugin_info->base_name, PLUGIN_OVERRIDE_GATE,
                     mageec_pass_gate, NULL);
  register_featextract();

  return 0;
}
