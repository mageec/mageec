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

#include "MAGEECPlugin.h"
#include "mageec/Util.h"

#include "gcc-plugin.h"
#include "tree-pass.h"


/// \brief Print information about the plugin to stdout
///
/// \param plugin_info  GCC plugin information.
/// \param version  GCC version information.
void mageecPluginInfo(struct plugin_name_args *plugin_info,
                      struct plugin_gcc_version *version)
{
  MAGEEC_STATUS("MAGEEC Plugin Information" << '\n'
             << "=========================" << '\n'
             << "base_name: " << plugin_info->base_name << '\n'
             << "full_name: " << plugin_info->full_name << '\n'
             << "    #args: " << plugin_info->argc << '\n'
             << "  version: " << plugin_info->version << '\n'
             << "     help: " << plugin_info->help << '\n');

  MAGEEC_STATUS("GCC Information" << '\n'
             << "---------------" << '\n'
             << "  basever: " << version->basever << '\n'
             << "datestamp: " << version->datestamp << '\n'
             << " devphase: " << version->devphase << '\n'
             << " revision: " << version->revision << '\n'
             << " confargs: " << version->configuration_arguments << '\n');
}
