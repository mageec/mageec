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

#include "mageec/Framework.h"
#include "mageec/Database.h"
#include "mageec/TrainedML.h"
#include "mageec/Util.h"

#include <map>
#include <memory>
#include <string>


#if !defined(MAGEEC_GCC_PLUGIN_VERSION_MAJOR) || \
    !defined(MAGEEC_GCC_PLUGIN_VERSION_MINOR) || \
    !defined(MAGEEC_GCC_PLUGIN_VERSION_PATCH)
#error "MAGEEC GCC plugin version not defined by the build system"
#endif

static_assert(MAGEEC_GCC_PLUGIN_VERSION_MAJOR == 2 &&
              MAGEEC_GCC_PLUGIN_VERSION_MINOR == 0 &&
              MAGEEC_GCC_PLUGIN_VERSION_PATCH == 0,
              "MAGEEC GCC plugin version does not match");


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


/// \brief Context to hold all of the information needed by the plugin
struct MAGEECContext {
  MAGEECContext()
    : is_init(),
      with_feature_extract(), with_optimize(),
      framework(), db(), ml(), func_features(), func_passes()
  {}

  bool is_init;

  bool with_feature_extract;
  bool with_optimize;

  std::unique_ptr<mageec::Framework> framework;
  std::unique_ptr<mageec::Database>  db;
  std::unique_ptr<mageec::TrainedML> ml;

  /// Feature sets for each function in the module
  std::map<std::string, std::unique_ptr<mageec::FeatureSet>> func_features;

  /// Pass sequence for each function in the module
  std::map<std::string, std::vector<std::string>> func_passes;
};


/// Version number for the plugin
extern const mageec::util::Version mageec_plugin_version;

/// The plugin base_name for our hooks to use to schedule new passes
extern const char *mageec_gcc_plugin_name;

/// Context shared by all components of the plugin
extern MAGEECContext mageec_context;


// GCC callbacks
void mageecStartFile(void *gcc_data, void *user_data);
void mageecFinishFile(void *gcc_data, void *user_data);
void mageecFinish(void *gcc_data, void *user_data);
void mageecPassGate(void *gcc_data, void *user_data);

/// \brief Register feature extraction pass callbacks
void mageecRegisterFeatureExtract(void);


#endif // MAGEEC_PLUGIN_H
