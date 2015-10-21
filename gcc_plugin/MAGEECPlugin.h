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

//===------------------------- MAGEEC GCC Plugin --------------------------===//
//
// Defines callbacks used by the GCC plugin to control MAGEEC
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_PLUGIN_H
#define MAGEEC_PLUGIN_H

#include <map>
#include <memory>
#include <string>


namespace mageec {
  class FeatureSet;
  class Framework;
  class TrainedML;
} // end of namespace mageec


// Handle to the MAGEEC framework and a trained machine learner
extern std::unique_ptr<mageec::Framework> mageec_framework;
extern std::unique_ptr<mageec::TrainedML> mageec_ml;

// The plugin base_name for our hooks to use to schedule new passes
extern const char *mageec_gcc_plugin_name;

// MAGEEC plugin configuration
extern std::map<std::string, int> mageec_config;

// Last features extracted by the feature extractor
extern std::unique_ptr<mageec::FeatureSet> mageec_features;


/// \brief Prints information about the plugin to stdout.
/// 
/// \param plugin_info  GCC plugin information.
/// \param version  GCC version information.
void mageecPluginInfo(struct plugin_name_args *plugin_info,
                      struct plugin_gcc_version *version);

// GCC callbacks
void mageecStartFile(void *gcc_data, void *user_data);
void mageecFinishFile(void *gcc_data, void *user_data);
void mageecFinish(void *gcc_data, void *user_data);
void mageecPassGate(void *gcc_data, void *user_data);

/// \brief Register feature extraction pass callbacks
void mageecRegisterFeatureExtract(void);


#endif // MAGEEC_PLUGIN_H
