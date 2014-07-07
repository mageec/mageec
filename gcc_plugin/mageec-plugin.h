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

/** @file mageec-plugin.h MAGEEC GCC Plugin. */
#ifndef __MAGEEC_PLUGIN_H__
#define __MAGEEC_PLUGIN_H__

#include "mageec/mageec.h"
#include <map>
#include <string>

extern mageec::mageec_framework mageec_inst;

void mageec_gcc_finish (void *gcc_data,
			void *user_data);

/** The plugin base_name for our hooks to use to schedule new passes */
extern const char *mageec_gcc_plugin_name;

/** MAGEEC Plugin Configuration */
extern std::map<std::string, int> mageec_config;

/**
 * Prints information about the plugin to stdout.
 * @param plugin_name_args GCC plugin information.
 * @param plugin_gcc_version GCC version information. 
 */
void mageec_gcc_plugin_info (struct plugin_name_args *plugin_info,
			     struct plugin_gcc_version *version);

void dummy_callback (void *gcc_data,
                     void *user_data);

void mageec_gcc_start_file (void *gcc_data, void *user_data);
void mageec_gcc_finish_file (void *gcc_data, void *user_data);

void mageec_pass_gate (void *gcc_data, void *user_data);

void register_featextract (void);

#endif
