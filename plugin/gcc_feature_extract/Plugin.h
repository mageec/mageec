/*  MAGEEC GCC Feature Extractor Plugin
    Copyright (C) 2013-2016 Embecosm Limited and University of Bristol

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

//===---------------- MAGEEC GCC Feature Extractor Plugin -----------------===//
//
// Defines callbacks for the plugin which perform feature extraction on an
// input program
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_GCC_FEATURE_EXTRACT_PLUGIN_H
#define MAGEEC_GCC_FEATURE_EXTRACT_PLUGIN_H

#include "FeatureExtract.h"
#include "mageec/AttributeSet.h"
#include "mageec/Framework.h"
#include "mageec/Database.h"
#include "mageec/Util.h"

#include <fstream>
#include <map>
#include <memory>
#include <string>


#if !defined(GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MAJOR) || \
    !defined(GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MINOR) || \
    !defined(GCC_FEATURE_EXTRACT_PLUGIN_VERSION_PATCH)
#error "GCC Feature Extractor plugin version not defined by build system"
#endif
static_assert(GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MAJOR == 2 &&
              GCC_FEATURE_EXTRACT_PLUGIN_VERSION_MINOR == 0 &&
              GCC_FEATURE_EXTRACT_PLUGIN_VERSION_PATCH == 0,
              "GCC Feature Extractor plugin version does not match");


/// \brief Context to hold information needed by the plugin
class FeatureExtractContext {
public:
  FeatureExtractContext()
      : m_framework(), m_db(), m_outfile(), m_func_features()
  {}

  FeatureExtractContext(const FeatureExtractContext &) = delete;
  void operator=(const FeatureExtractContext &) = delete;

  static FeatureExtractContext& getInstance() {
    static FeatureExtractContext instance;
    return instance;
  }

  void setFramework(std::unique_ptr<mageec::Framework> framework) {
    m_framework = std::move(framework);
  }
  mageec::Framework& getFramework() {
    assert(m_framework);
    return *m_framework;
  }

  void loadDatabase(std::string db_path) {
    assert(db_path != "");
    assert(m_framework);
    m_db = m_framework->getDatabase(db_path, false);
  }
  mageec::Database& getDatabase() {
    assert(m_db);
    return *m_db;
  }

  void openOutFile(std::string file) {
    assert(file != "");
    m_outfile.reset(new std::ofstream(file, std::ofstream::app));
  }
  std::ofstream& getOutFile(void) {
    assert(m_outfile);
    return *m_outfile;
  }

  std::map<std::string, std::unique_ptr<FunctionFeatures>>&
  getFunctionFeatures(void) {
    return m_func_features;
  }

private:
  std::unique_ptr<mageec::Framework> m_framework;
  std::unique_ptr<mageec::Database>  m_db;

  /// Output file into which feature group identifiers will be emitted into
  std::unique_ptr<std::ofstream> m_outfile;

  /// Extract features for each function in the module
  std::map<std::string, std::unique_ptr<FunctionFeatures>> m_func_features;
};


/// The plugin base_name for our hooks to use to schedule new passes
extern const char *feature_extract_plugin_name;

/// version number for the plugin
extern const mageec::util::Version feature_extract_plugin_version;

/// context shared by all components of the plugin
extern FeatureExtractContext feature_extract_context;


/// \brief Register the feature extraction pass
void registerFeatureExtractPass(void);

/// \brief Execute the feature extractor on a single function
unsigned featureExtractExecute();

/// \brief Clear the set of function features ready for the next unit
void featureExtractStartUnit(void *gcc_data, void *user_data);

/// \brief Output identifiers for the extracted sets of features
void featureExtractFinishUnit(void *gcc_data, void *user_data);


#endif /*MAGEEC_GCC_FEATURE_EXTRACT_PLUGIN_H*/
