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

#include "mageec/Framework.h"
#include "mageec/Database.h"
#include "mageec/TrainedML.h"
#include "mageec/Util.h"


namespace mageec {
  class FeatureSet;
} // end of namespace mageec



/// \brief The different modes which the plugin may run in
enum class MAGEECMode : unsigned {
  kPluginInfo,
  kPrintMachineLearners,
  kFeatureExtract,
  kFeatureExtractAndSave,
  kFeatureExtractAndOptimize,
  kFeatureExtractSaveAndOptimize
};


/// \brief The context which holds all the information need by the plugin
struct MAGEECContext {
  MAGEECContext()
    : is_init(), mode(), framework(), database(), machine_learner(), features()
  {}

  bool is_init;
  bool with_debug;

  std::unique_ptr<MAGEECMode> mode;
  std::unique_ptr<mageec::Framework> framework;
  std::unique_ptr<mageec::Database>  database;
  std::unique_ptr<mageec::TrainedML> machine_learner;

  /// Last feature set extracted
  std::unique_ptr<mageec::FeatureSet> features;
};


/// The plugin base_name for our hooks to use to schedule new passes
extern const char *mageec_gcc_plugin_name;

/// Context shared by all components of the plugin
extern MAGEECContext mageec_context;


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
