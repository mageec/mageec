/*  Copyright (C) 2015, Embecosm Limited

    This file is part of MAGEEC

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

//===------------------------ MAGEEC C5.0 Driver --------------------------===//
//
// This implements the machine learner interface by using a driver to drive
// an external C5.0 machine learner. It calls out to the C5.0 command line
// in order to make decisions and train.
//
//===----------------------------------------------------------------------===//

#include "mageec/Database.h"
#include "mageec/ML/C5.h"
#include "mageec/ML.h"
#include "mageec/Result.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <map>
#include <set>
#include <string>
#include <sstream>
#include <vector>

// Training and prediction interfaces to the C5.0 machine learner library
extern "C" {
  void c50(char **namesv,
           char **datav,
           char **costv,
           int *subset,
           int *rules,
           int *utility,
           int *trials,
           int *winnow,
           double *sample,
           int *seed,
           int *noGlobalPruning,
           double *CF,
           int *minCases,
           int *fuzzyThreshold,
           int *earlyStopping,
           char **treev,
           char **rulesv,
           char **outputv);

  void predictions(char **casev,
                   char **namesv,
                   char **treev,
                   char **rulesv,
                   char **costv,
                   int *predv,
			             double *confidencev,
			             int *trials,
                   char **outputv);
};

namespace mageec {
namespace {

/// \struct C5Context
///
/// \brief Data stored in the blob held in the database for the machine
/// learner
struct C5Context {
  /// \brief Output the data required by the C5.0 machine learner to a blob
  ///
  /// This blob must be parsable by the fromBlob function. When this method
  /// is called, the context must be in a fully initialized state.
  ///
  /// \return  The serialized blob
  std::vector<uint8_t> toBlob();

  /// \brief Parse the data for the C5.0 machine learner from an input blob
  ///
  /// The input blob must be valid for this machine learner. It is a fatal
  /// error if the provided blob is invalid or malformed.
  ///
  /// \param blob  The binary blob containing C5.0 training data
  /// \return  The parsed context
  static std::unique_ptr<C5Context> fromBlob(const std::vector<uint8_t> &blob);

  /// Holds a set of all of the features seen when training the classifier.
  /// These are stored ordered, and this defines the order which the features
  /// appear in the .names, .data and .cases file for the classifier.
  std::set<FeatureDesc> feature_descs;

  /// Holds a set of all of the parameters that we are training the classifier
  /// for.
  std::set<ParameterDesc> parameter_descs;

  /// Holds all of the passes that we have trained for.
  std::set<std::string> passes;

  /// Holds a classifier tree for each of the parameters we trained against.
  std::map<unsigned, std::vector<uint8_t>> parameter_classifier_trees;

  /// Classifier trees for each of the passes we trained for.
  std::map<std::string, std::vector<uint8_t>> pass_classifier_trees;
};

/// \brief Types of field found in the machine learner blob for the C5.0
/// classifier.
enum class C5BlobField {
  kFeatureDesc,
  kParameterDesc,
  kPassDesc,
  kParameterClassifierTree,
  kPassClassifierTree
};

} // end of anonymous namespace

std::unique_ptr<C5Context>
C5Context::fromBlob(const std::vector<uint8_t> &blob) {
  std::unique_ptr<C5Context> context(new C5Context());

  auto it = blob.cbegin();
  while (it != blob.cend()) {
    C5BlobField field = static_cast<C5BlobField>(util::read16LE(it));
    switch (field) {
    case C5BlobField::kFeatureDesc: {
      // | feat_id | feat_type |
      unsigned feat_id = util::read16LE(it);
      FeatureType feat_type = static_cast<FeatureType>(util::read16LE(it));
      context->feature_descs.insert({feat_id, feat_type});
      break;
    }
    case C5BlobField::kParameterDesc: {
      // | param_id | param_type |
      unsigned param_id = util::read16LE(it);
      ParameterType param_type = static_cast<ParameterType>(util::read16LE(it));
      context->parameter_descs.insert({param_id, param_type});
      break;
    }
    case C5BlobField::kPassDesc: {
      // | pass_name_len | pass_name |
      unsigned len = util::read16LE(it);
      std::string pass_name(it, it + len);
      it += len;
      context->passes.insert(pass_name);
      break;
    }
    case C5BlobField::kParameterClassifierTree: {
      // | param_id | classifier_len | classifier_blob |
      unsigned param_id = util::read16LE(it);
      unsigned len = util::read16LE(it);

      std::vector<uint8_t> classifier_blob(len);
      for (unsigned i = 0; i < len; ++i) {
        classifier_blob[i] = *(it + i);
      }
      it += len;

      context->parameter_classifier_trees.insert(
          std::make_pair(param_id, classifier_blob));
      break;
    }
    case C5BlobField::kPassClassifierTree: {
      // | pass_name_len | pass_name | classifier_len | classifier_blob |
      unsigned pass_name_len = util::read16LE(it);
      std::string pass_name(it, it + pass_name_len);
      it += pass_name_len;

      unsigned classifier_len = util::read16LE(it);

      std::vector<uint8_t> classifier_blob(classifier_len);
      for (unsigned i = 0; i < classifier_len; ++i) {
        classifier_blob[i] = *(it + i);
      }
      it += classifier_len;

      context->pass_classifier_trees.insert(
          std::make_pair(pass_name, classifier_blob));
      break;
    }
    }
  }
  return context;
}

std::vector<uint8_t> C5Context::toBlob() {
  // Buffer used to store the training blob
  std::vector<uint8_t> blob;

  // Store descriptions of features/parameters and passes that were trained
  // against.
  for (auto feat : feature_descs) {
    // | feat_id | feat_type |
    unsigned feat_id = feat.id;
    unsigned feat_type = static_cast<unsigned>(feat.type);

    util::write16LE(blob, static_cast<unsigned>(C5BlobField::kFeatureDesc));
    util::write16LE(blob, feat_id);
    util::write16LE(blob, feat_type);
  }
  for (auto param : parameter_descs) {
    // | param_id | param_type |
    unsigned param_id = param.id;
    unsigned param_type = static_cast<unsigned>(param.type);

    util::write16LE(blob, static_cast<unsigned>(C5BlobField::kParameterDesc));
    util::write16LE(blob, param_id);
    util::write16LE(blob, param_type);
  }
  for (auto pass : passes) {
    // | pass_name_len | pass_name |
    std::string pass_name = pass;
    unsigned pass_name_len = static_cast<unsigned>(pass_name.length());

    util::write16LE(blob, static_cast<unsigned>(C5BlobField::kPassDesc));
    util::write16LE(blob, pass_name_len);
    for (auto it : pass_name) {
      blob.push_back(static_cast<uint8_t>(it));
    }
  }

  // Store the classifier tree for each of the parameters
  for (auto param_tree : parameter_classifier_trees) {
    // | param_id | classifier_len | classifier_blob |
    unsigned param_id = param_tree.first;
    std::vector<uint8_t> classifier_blob = param_tree.second;
    unsigned classifier_len = static_cast<unsigned>(classifier_blob.size());

    util::write16LE(
        blob, static_cast<unsigned>(C5BlobField::kParameterClassifierTree));
    util::write16LE(blob, param_id);
    util::write16LE(blob, classifier_len);
    for (auto it : classifier_blob) {
      blob.push_back(it);
    }
  }

  // Store the classifier tree for each of the passes
  for (auto pass_tree : pass_classifier_trees) {
    // | pass_name_len | pass_name | classifier_len | classifier_blob |
    std::string pass_name = pass_tree.first;
    unsigned pass_name_len = static_cast<unsigned>(pass_name.length());
    std::vector<uint8_t> classifier_blob = pass_tree.second;
    unsigned classifier_len = static_cast<unsigned>(classifier_blob.size());

    util::write16LE(blob,
                    static_cast<unsigned>(C5BlobField::kPassClassifierTree));
    util::write16LE(blob, pass_name_len);
    for (auto it : pass_name) {
      blob.push_back(static_cast<uint8_t>(it));
    }
    util::write16LE(blob, classifier_len);
    for (auto it : classifier_blob) {
      blob.push_back(it);
    }
  }
  return blob;
}

// UUID for the C5 Driver
const util::UUID C5Driver::uuid =
    util::UUID::parse("ccf7593c-78d9-429a-b544-2a7339d4325e").get();

C5Driver::C5Driver() : IMachineLearner() {}

C5Driver::~C5Driver() {}

std::unique_ptr<DecisionBase>
C5Driver::makeDecision(const DecisionRequestBase &request,
                       const FeatureSet &features,
                       const std::vector<uint8_t> &blob) const {
  // Deserialize the machine learner data from the blob
  std::unique_ptr<C5Context> context = C5Context::fromBlob(blob);

  // Find the appropriate classifier tree for the provided decision request
  DecisionRequestType request_type = request.getType();

  assert((request_type == DecisionRequestType::kBool ||
          request_type == DecisionRequestType::kRange) &&
         "Unhandled decision request type");

  unsigned param_id;
  ParameterType param_type;

  // The ID is the identifier of the tunable parameter
  if (request_type == DecisionRequestType::kBool) {
    const auto *bool_request =
        static_cast<const BoolDecisionRequest *>(&request);
    param_id = bool_request->getID();

    assert(bool_request->getDecisionType() == DecisionType::kBool);
    param_type = ParameterType::kBool;
  } else if (request_type == DecisionRequestType::kRange) {
    const auto *range_request =
        static_cast<const RangeDecisionRequest *>(&request);
    param_id = range_request->getID();

    assert(range_request->getDecisionType() == DecisionType::kRange);
    param_type = ParameterType::kRange;
  } else {
    assert(0 && "Unreachable");
  }

  // Check if we have a classifier tree for this parameter.
  const auto res = context->parameter_classifier_trees.find(param_id);
  if (res == context->parameter_classifier_trees.cend()) {
    return std::unique_ptr<DecisionBase>(new NativeDecision());
  }

  // Output the classifier tree to a buffer
  const std::vector<uint8_t> &tree_blob = res->second;
  std::ostringstream tree_data;
  for (auto c : tree_blob) {
    tree_data << c;
  }

  // Output names data (columns for classifier) for this parameter
  // FIXME: This is copied from the 'train' code and should be factored out
  // TODO: Platform independent temporary file
  std::ostringstream names_data;
    
  // Output the target parameter first
  // TODO: Comment containing parameter description
  names_data << "parameter_" << std::to_string(param_id) << ".\n";

  // Output columns for all of the features which we have seen in the
  // training set.
  // TODO: Add comment containing feature description
  for (auto feat : context->feature_descs) {
    names_data << "feature_" << std::to_string(feat.id) << ": ";

    switch (feat.type) {
    case FeatureType::kBool:
      names_data << "t, f.";
      break;
    case FeatureType::kInt:
      names_data << "continuous.";
      break;
    }
    names_data << '\n';
  }
  names_data << '\n';

  // Output a column for the target parameter
  names_data << "parameter_" << std::to_string(param_id) << ": ";
  switch (param_type) {
  case ParameterType::kBool:
    names_data << "t, f.";
    break;
  case ParameterType::kRange:
    names_data << "continuous.";
    break;
  default:
    break;
  }
  names_data << '\n';

  // Output cases file (.cases) data, containing the feature set
  std::ostringstream cases_data;

  for (auto feat : context->feature_descs) {
    // Feature values are output in the order they appear in the feature
    // description map (ascending order of feature id)

    // FIXME: Avoid the dumb linear search here
    FeatureBase *f = nullptr;
    for (std::shared_ptr<FeatureBase> it : features) {
      if (it->getID() == feat.id) {
        f = it.get();
      }
    }

    if (f) {
      // There is a value for this feature in the feature set, output it
      assert(f->getType() == feat.type);

      switch (feat.type) {
      case FeatureType::kBool: {
        bool value = static_cast<BoolFeature *>(f)->getValue();
        cases_data << (value ? "t" : "f");
        break;
      }
      case FeatureType::kInt: {
        int64_t value = static_cast<IntFeature *>(f)->getValue();
        cases_data << value;
        break;
      }
      }
      cases_data << ",";
    } else {
      // No value for this feature in the feature set
      cases_data << "?,";
    }
  }
  // Finally, add the 'unknown' for our target parameter
  cases_data << "?" << '\n';

  // Now we have a .tree, .names and .cases data, run the classifier over them
  // to make a prediction
  // TODO: Useful debug here
  MAGEEC_DEBUG("Running the C5.0 classifier for decision");

  // input files as buffers
  std::string cases_str = cases_data.str();
  std::string names_str = names_data.str();
  std::string tree_str = tree_data.str();

  char *casev = (char*)malloc(cases_str.size() + 1);
  strcpy(casev, cases_str.c_str());
  char *namesv = (char*)malloc(names_str.size() + 1);
  strcpy(namesv, names_str.c_str());
  char *treev = (char*)malloc(tree_str.size() + 1);
  strcpy(treev, tree_str.c_str());

  char *rulesv = (char*)malloc(1); rulesv[0] = '\0';
  char *costv = (char*)malloc(1); costv[0] = '\0';
  // default parameters for C5.0
  int trials = 1;
  // output parameters
  int *predv = (int*)malloc(1);
  double confidencev;
  char *outputv = nullptr;

  predictions(&casev, &namesv, &treev, &rulesv, &costv, predv, &confidencev,
              &trials, &outputv);

  // free memory for the inputs, and unused outputs
  free(casev);
  free(namesv);
  free(treev);
  free(rulesv);
  free(costv);
  if (outputv != nullptr)
    free(outputv);

  // The result was written back to predv
  int predict_res = predv[0];
  free(predv);

  // Get the value of the returned decision
  // FIXME: Call out to the C5.0 classifier
  switch (request_type) {
  case DecisionRequestType::kBool: {
    const auto *bool_request =
        static_cast<const BoolDecisionRequest *>(&request);
    assert(bool_request->getDecisionType() == DecisionType::kBool);

    // The result is actually the index into the class 't,f', with the index
    // starting from 1
    assert(predict_res == 1 || predict_res == 2);
    bool bool_res = (predict_res == 1) ? true : false;

    return std::unique_ptr<BoolDecision>(new BoolDecision(bool_res));
  }
  case DecisionRequestType::kRange: {
    const auto *range_request =
        static_cast<const RangeDecisionRequest *>(&request);
    assert(range_request->getDecisionType() == DecisionType::kRange);
    return std::unique_ptr<RangeDecision>(new RangeDecision(predict_res));
  }
  default:
    assert(0 && "Unhandled DecisionRequest type!");
    return nullptr;
  }
}

const std::vector<uint8_t>
C5Driver::train(std::set<FeatureDesc> feature_descs,
                std::set<ParameterDesc> parameter_descs,
                std::set<std::string> passes,
                ResultIterator result_iter) const {
  std::unique_ptr<C5Context> context(new C5Context());
  context->feature_descs = feature_descs;
  context->parameter_descs = parameter_descs;
  context->passes = passes;

  MAGEEC_DEBUG("Training database using C5 Machine Learner");

  // Read all of the results data in one go. For each distinct set of input
  // features, store the best set of results. This is achieved by storing a
  // hash from a feature set to its current best result.
  MAGEEC_DEBUG("Collecting results");
  std::map<uint64_t, Result> result_map;
  for (util::Option<Result> result; (result = *result_iter);
       result_iter = result_iter.next()) {
    FeatureSet features = result.get().getFeatures();
    uint64_t value = result.get().getValue();
    uint64_t hash = features.hash();

    bool result_handled = false;
    while (!result_handled) {
      auto curr_entry = result_map.find(hash);
      if (curr_entry != result_map.end()) {
        const FeatureSet &curr_features = curr_entry->second.getFeatures();
        uint64_t curr_value = curr_entry->second.getValue();

        if (features == curr_features) {
          if (value < curr_value) {
            result_map.at(hash) = result.get();
          }
          result_handled = true;
        } else {
          hash++;
        }
      } else {
        result_map.emplace(hash, result.get());
        result_handled = true;
      }
    }
  }

  // Create a classifier trained for each tunable parameter in turn.
  MAGEEC_DEBUG("Training for tunable parameters");

  unsigned curr_param = 0;
  unsigned param_count = static_cast<unsigned>(parameter_descs.size());

  // Train for all of the simple parameter types. Training for pass sequences
  // is a little more complicated and is handled separately later.
  for (auto param : parameter_descs) {
    if (param.type == ParameterType::kPassSeq) {
      continue;
    }

    MAGEEC_DEBUG("Training parameter " << curr_param << " of "
                 << param_count - 1);
    curr_param++;

    // Output names file (columns for classifier) for this parameter
    MAGEEC_DEBUG("Building .names file data");
    std::ostringstream names_data;

    // Output the target parameter first
    // TODO: Comment containing parameter description
    names_data << "parameter_" << param.id << ".\n";

    // Output columns for all of the features which we have seen in the
    // training set.
    // TODO: Add comment containing feature description
    for (auto feat : feature_descs) {
      names_data << "feature_" << feat.id << ": ";

      switch (feat.type) {
      case FeatureType::kBool:
        names_data << "t, f.";
        break;
      case FeatureType::kInt:
        names_data << "continuous.";
        break;
      }
      names_data << '\n';
    }
    names_data << '\n';

    // Output a column for the target parameter
    names_data << "parameter_" << param.id << ": ";
    switch (param.type) {
    case ParameterType::kBool:
      names_data << "t, f.";
      break;
    case ParameterType::kRange:
      names_data << "continuous.";
      break;
    default:
      break;
    }
    names_data << '\n';

    // For the current parameter, generate the values in the .data file
    // containing all of the training data
    MAGEEC_DEBUG("Building .data file data");
    std::ostringstream data_data;

    for (auto res : result_map) {
      ParameterSet parameters = res.second.getParameters();
      FeatureSet features = res.second.getFeatures();

      // Check that this result has an entry for this parameter. If not then
      // skip as we can't use it for training.
      // FIXME: Don't use a dumb linear search here
      ParameterBase *p = nullptr;
      for (auto it : parameters) {
        if (it->getID() == param.id) {
          p = it.get();
        }
      }
      if (!p)
        continue;

      for (auto feat : feature_descs) {
        // Feature values are output in the order they appear in the feature
        // description map (ascending order of feature id)
        // FIXME: Don't use a dumb linear search here
        FeatureBase *f = nullptr;
        for (auto it : features) {
          if (it->getID() == feat.id) {
            f = it.get();
          }
        }

        if (f) {
          // The result has a value for this feature, output it
          assert(f->getType() == feat.type);

          switch (feat.type) {
          case FeatureType::kBool: {
            bool value = static_cast<BoolFeature *>(f)->getValue();
            data_data << (value ? "t" : "f");
            break;
          }
          case FeatureType::kInt: {
            int64_t value = static_cast<IntFeature *>(f)->getValue();
            data_data << value;
            break;
          }
          }
          data_data << ",";
        } else {
          // No value for this feature for this result.
          data_data << "?,";
        }
      }
      // Output the parameter value last.
      assert(p->getType() == param.type);

      switch (param.type) {
      case ParameterType::kBool: {
        bool value = static_cast<BoolParameter *>(p)->getValue();
        data_data << (value ? "t" : "f");
        break;
      }
      case ParameterType::kRange: {
        int64_t value = static_cast<RangeParameter *>(p)->getValue();
        data_data << value;
        break;
      }
      default:
        break;
      }
      data_data << "\n";
    }

    // Now we have .names and .data files, run the classifier over them to
    // generate a tree
    MAGEEC_DEBUG("Running the C5.0 classifier for parameter "
                 << std::to_string(param.id));

    // input files as buffers
    std::string names_str = names_data.str();
    std::string data_str = data_data.str();

    char *namesv = (char*)malloc(names_str.size() + 1);
    strcpy(namesv, names_str.c_str());
    char *datav = (char*)malloc(data_str.size() + 1);
    strcpy(datav, data_str.c_str());
    char *costv = (char*)malloc(1); costv[0] = '\0';
    // default parameters for C5.0
    int subset = 1;
    int rules = 0;
    int utility = 0;
    int trials = 1;
    int winnow = 0;
    double sample = 0.0;
    int seed = 0xbeef;
    int noGlobalPruning = 0;
    double CF = 0.25;
    int minCases = 2;
    int fuzzyThreshold = 0;
    int earlyStopping = 1;
    // output parameters
    char *treev = nullptr;
    char *rulesv = nullptr;
    char *outputv = nullptr;

    c50(&namesv, &datav, &costv, &subset, &rules, &utility, &trials,
        &winnow, &sample, &seed, &noGlobalPruning, &CF, &minCases,
        &fuzzyThreshold, &earlyStopping, &treev, &rulesv, &outputv);

    // free memory for all of the unused parameters
    free(namesv);
    free(datav);
    if (rulesv != nullptr)
      free(rulesv);
    if (outputv != nullptr)
      free(outputv);

    // Retrieve the tree
    assert(treev != nullptr);
    std::vector<uint8_t> tree_blob;
    tree_blob.resize(strlen(treev));
    for (unsigned i = 0; i < strlen(treev); ++i)
      tree_blob.data()[i] = treev[i];
    // free the memory for the tree buffer
    free(treev);

    // save the tree for the current parameter
    context->parameter_classifier_trees.insert(
        std::make_pair(param.id, tree_blob));
  }

  MAGEEC_DEBUG("Training passes");

  // Create a classifier trained for each of the input passes in turn
  for (auto pass : passes) {
    MAGEEC_DEBUG("Training for pass '" << pass << "'");

    // Output names file (columns for classifier) for this pass
    MAGEEC_DEBUG("Building .names file data");
    std::ostringstream names_data;

    // Output the target pass first
    // TODO: Comment containing pass description
    names_data << "pass_" << pass << ".\n";

    // Output columns for all of the features which we have seen in the
    // training set.
    // TODO: Add comment containing feature description
    for (auto feat : feature_descs) {
      names_data << "feature_" << feat.id << ": ";

      switch (feat.type) {
      case FeatureType::kBool:
        names_data << "t, f.";
        break;
      case FeatureType::kInt:
        names_data << "continuous.";
        break;
      }
      names_data << '\n';
    }
    names_data << '\n';

    // Output a column for the target pass
    names_data << "pass_" << pass << ": t, f.\n";

    // For the current pass, generate the values in the .data file
    // containing all of the training data
    MAGEEC_DEBUG("Building .data file data");
    std::ostringstream data_data;

    for (auto res : result_map) {
      FeatureSet features = res.second.getFeatures();
      ParameterSet parameters = res.second.getParameters();

      // Find the parameter in the parameter set which holds the pass
      // sequence.
      // TODO: Don't use a linear search to do this?
      std::vector<std::string> pass_seq;
      for (auto p : parameters) {
        if (p->getType() == ParameterType::kPassSeq) {
          pass_seq = static_cast<PassSeqParameter *>(p.get())->getValue();
          break;
        }
      }
      assert(pass_seq.size());

      for (auto feat : feature_descs) {
        // Feature values are output in the order they appear in the feature
        // description map (ascending order of feature id)
        // FIXME: Don't use a dumb linear search here
        FeatureBase *f = nullptr;
        for (auto it : features) {
          if (it->getID() == feat.id) {
            f = it.get();
          }
        }

        if (f) {
          // The result has a value for this feature, output it
          assert(f->getType() == feat.type);

          switch (feat.type) {
          case FeatureType::kBool: {
            bool value = static_cast<BoolFeature *>(f)->getValue();
            data_data << (value ? "t" : "f");
            break;
          }
          case FeatureType::kInt: {
            int64_t value = static_cast<IntFeature *>(f)->getValue();
            data_data << value;
            break;
          }
          }
          data_data << ",";
        } else {
          // No value for this feature for this result.
          data_data << "?,";
        }
      }
      // Output whether the pass was run or not last.
      bool run_pass = false;
      for (auto p : pass_seq) {
        if (p == pass) {
          run_pass = true;
          break;
        }
      }
      data_data << (run_pass ? "t" : "f");
      data_data << "\n";
    }

    // Now we have .names and .data files, run the classifier over them to
    // generate a tree
    MAGEEC_DEBUG("Running the C5.0 classifier for pass " << pass);

    // input files as buffers
    std::string names_str = names_data.str();
    std::string data_str = data_data.str();

    char *namesv = (char*)malloc(names_str.size() + 1);
    strcpy(namesv, names_str.c_str());
    char *datav = (char*)malloc(data_str.size() + 1);
    strcpy(datav, data_str.c_str());
    char *costv = (char*)malloc(1); costv[0] = '\0';
    // default parameters for C5.0
    int subset = 1;
    int rules = 0;
    int utility = 0;
    int trials = 1;
    int winnow = 0;
    double sample = 0.0;
    int seed = 0xbeef;
    int noGlobalPruning = 0;
    double CF = 0.25;
    int minCases = 2;
    int fuzzyThreshold = 0;
    int earlyStopping = 1;
    // output parameters
    char *treev = nullptr;
    char *rulesv = nullptr;
    char *outputv = nullptr;

    c50(&namesv, &datav, &costv, &subset, &rules, &utility, &trials,
        &winnow, &sample, &seed, &noGlobalPruning, &CF, &minCases,
        &fuzzyThreshold, &earlyStopping, &treev, &rulesv, &outputv);

    // free memory for all of the unused parameters
    free(namesv);
    free(datav);
    if (rulesv != nullptr)
      free(rulesv);
    if (outputv != nullptr)
      free(outputv);

    // Retrieve the tree
    assert(treev != nullptr);
    std::vector<uint8_t> tree_blob;
    tree_blob.resize(strlen(treev));
    for (unsigned i = 0; i < strlen(treev); ++i)
      tree_blob.data()[i] = treev[i];
    // free the memory for the tree buffer
    free(treev);

    // save the tree for the current parameter
    context->pass_classifier_trees.insert(
        std::make_pair(pass, tree_blob));
  }
  MAGEEC_DEBUG("Training finished");

  // Serialize the context to a blob
  return context->toBlob();
}

} // end of namespace mageec
