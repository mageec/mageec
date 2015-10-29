/*  MAGEEC GCC Plugin
    Copyright (C) 2013-2015 Embecosm Limited and University of Bristol

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

//===---------------------- Plugin Information ----------------------------===//
//
// Prints GCC plugin and version information
//
//===----------------------------------------------------------------------===//

// We need to undefine these as gcc-plugin.h redefines them
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#include <iostream>

#include "MAGEECPlugin.h"

#include "gcc-plugin.h"
#include "tree-pass.h"


/// \brief Print information about the plugin to stdout
///
/// \param plugin_info  GCC plugin information.
/// \param version  GCC version information.
void mageecPluginInfo(struct plugin_name_args *plugin_info,
                      struct plugin_gcc_version *version)
{
  mageecDbg() << "MAGEEC Plugin Information" << std::endl
              << "=========================" << std::endl
              << "base_name: " << plugin_info->base_name << std::endl
              << "full_name: " << plugin_info->full_name << std::endl
              << "    #args: " << plugin_info->argc << std::endl
              << "  version: " << plugin_info->version << std::endl
              << "     help: " << plugin_info->help << std::endl;

  mageecDbg() << "GCC Information" << std::endl
              << "---------------" << std::endl
              << "  basever: " << version->basever << std::endl
              << "datestamp: " << version->datestamp << std::endl
              << " devphase: " << version->devphase << std::endl
              << " revision: " << version->revision << std::endl
              << " confargs: " << version->configuration_arguments << std::endl;
}
