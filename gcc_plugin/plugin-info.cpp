/*  MAGEEC GCC Plugin
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

/** @file plugin-info.cpp Prints GCC plugin and version information. */
/* We need to undefine these as gcc-plugin.h redefines them */
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include "gcc-plugin.h"
#include "mageec-plugin.h"

#include <stdio.h>

/**
 * Prints information about the plugin to stdout.
 * @param plugin_name_args GCC plugin information.
 * @param plugin_gcc_version GCC version information. 
 */
void mageec_gcc_plugin_info (struct plugin_name_args *plugin_info,
                             struct plugin_gcc_version *version)
{
  printf ("MAGEEC Plugin Information\n=========================\n");
  printf ("base_name: %s\n", plugin_info->base_name);
  printf ("full_name: %s\n", plugin_info->full_name);
  printf ("#args    : %i\n", plugin_info->argc);
  printf ("version  : %s\n", plugin_info->version);
  printf ("help     : %s\n", plugin_info->help);
  printf ("\n");

  printf ("GCC Information\n---------------\n");
  printf ("basever  : %s\n", version->basever);
  printf ("datestamp: %s\n", version->datestamp);
  printf ("devphase : %s\n", version->devphase);
  printf ("revision : %s\n", version->revision);
  printf ("confargs : %s\n", version->configuration_arguments);
  printf ("\n");
}
