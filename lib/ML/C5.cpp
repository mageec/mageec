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
#include <map>
#include <fstream>
#include <set>
#include <string>
#include <vector>

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

/// \brief Comparator to sort results based on their constituent features
///
/// As the C5 learner only uses the best result for a given feature set, we
/// need to be able to find common feature sets in the database. For now,
/// we just do a brute force comparison of features.
///
/// FIXME: In the database, the FeatureGroupID and FeatureSetID *almost*
/// uniquely identify a set of features. If we had that ID at this point,
/// we would not have to do this expensive comparison.
struct ResultFeatureComparator {
  bool operator()(const Result &lhs, const Result &rhs) {
    FeatureSet lhs_feats = lhs.getFeatures();
    FeatureSet rhs_feats = rhs.getFeatures();

    if (lhs_feats.size() < rhs_feats.size()) {
      return true;
    } else if (lhs_feats.size() > rhs_feats.size()) {
      return false;
    }

    auto lhs_iter = lhs_feats.begin();
    auto rhs_iter = rhs_feats.begin();
    while (lhs_iter != lhs_feats.end()) {
      unsigned lhs_id = (*lhs_iter)->getID();
      unsigned rhs_id = (*rhs_iter)->getID();
      if (lhs_id < rhs_id) {
        return true;
      } else if (lhs_id > rhs_id) {
        return false;
      }

      std::vector<uint8_t> lhs_blob = (*lhs_iter)->toBlob();
      std::vector<uint8_t> rhs_blob = (*rhs_iter)->toBlob();
      if (lhs_blob.size() < rhs_blob.size()) {
        return true;
      } else if (lhs_blob.size() > rhs_blob.size()) {
        return false;
      }

      if (lhs_blob < rhs_blob) {
        return true;
      }

      lhs_iter++;
      rhs_iter++;
    }
    return true;
  }
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
  switch (request_type) {
  case DecisionRequestType::kBool:
  case DecisionRequestType::kRange: {
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

    // Output the classifier tree to a file (.tree)
    const std::vector<uint8_t> &classifier_tree = res->second;
    std::ofstream tree_file("/tmp/parameter_" + std::to_string(param_id) +
                                ".tree",
                            std::ofstream::binary);
    for (auto c : classifier_tree) {
      tree_file << c;
    }
    tree_file.close();

    // Output names file (columns for classifier) for this parameter
    // FIXME: This is copied from the 'train' code and should be factored out
    // TODO: Platform independent temporary file
    std::ofstream name_file("/tmp/parameter_" + std::to_string(param_id) +
                                ".names",
                            std::ofstream::binary);

    // Output the target parameter first
    // TODO: Comment containing parameter description
    name_file << "parameter_" << std::to_string(param_id) << ".\n";

    // Output columns for all of the features which we have seen in the
    // training set.
    // TODO: Add comment containing feature description
    for (auto feat : context->feature_descs) {
      name_file << "feature_" << std::to_string(feat.id) << ": ";

      switch (feat.type) {
      case FeatureType::kBool:
        name_file << "t, f.";
        break;
      case FeatureType::kInt:
        name_file << "continuous.";
        break;
      }
      name_file << '\n';
    }
    name_file << '\n';

    // Output a column for the target parameter
    name_file << "parameter_" << std::to_string(param_id) << ": ";
    switch (param_type) {
    case ParameterType::kBool:
      name_file << "t, f.";
      break;
    case ParameterType::kRange:
      name_file << "continuous.";
      break;
    default:
      break;
    }
    name_file << '\n';
    name_file.close();

    // Output cases files (.cases), containing the feature set
    std::ofstream cases_file("/tmp/parameter_" + std::to_string(param_id) +
                             ".cases");

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
          cases_file << (value ? "t" : "f");
          break;
        }
        case FeatureType::kInt: {
          int64_t value = static_cast<IntFeature *>(f)->getValue();
          cases_file << value;
          break;
        }
        }
      } else {
        // No value for this feature in the feature set
        cases_file << "?,";
      }
    }
    // Finally, add the 'unknown' for our target parameter
    cases_file << "?" << '\n';
    cases_file.close();
    break;
  }
  default:
    assert(0 && "Unhandled DecisionRequest type!");
  }

  // Get the value of the returned decision
  // FIXME: Call out to the C5.0 classifier
  switch (request_type) {
  case DecisionRequestType::kBool: {
    const auto *bool_request =
        static_cast<const BoolDecisionRequest *>(&request);
    assert(bool_request->getDecisionType() == DecisionType::kBool);
    return std::unique_ptr<BoolDecision>(new BoolDecision(true));
  }
  case DecisionRequestType::kRange: {
    const auto *range_request =
        static_cast<const RangeDecisionRequest *>(&request);
    assert(range_request->getDecisionType() == DecisionType::kRange);
    return std::unique_ptr<RangeDecision>(new RangeDecision(0xdeadbeef));
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

  // Read all of the results data in one go. We only store the best (lowest)
  // result for each input set of features.
  std::map<Result, uint64_t, ResultFeatureComparator> results;

  MAGEEC_DEBUG("Collecting results");
  for (util::Option<Result> result; (result = *result_iter);
       result_iter = result_iter.next()) {
    Result res = result.get();
    uint64_t value = res.getValue();

    auto curr_elem = results.insert(std::make_pair(res, value));
    if (curr_elem.second == false) {
      if (value < curr_elem.first->second) {
        curr_elem.first->second = value;
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

    MAGEEC_DEBUG("Training parameter " << curr_param << " of " << param_count);
    curr_param++;

    // Output names file (columns for classifier) for this parameter
    // TODO: Platform independent temporary file
    MAGEEC_DEBUG("Building .names file");
    std::ofstream name_file("/tmp/parameter_" + std::to_string(param.id) +
                                ".names",
                            std::ofstream::binary);

    // Output the target parameter first
    // TODO: Comment containing parameter description
    name_file << "parameter_" << param.id << ".names\n";

    // Output columns for all of the features which we have seen in the
    // training set.
    // TODO: Add comment containing feature description
    for (auto feat : feature_descs) {
      name_file << "feature_" << feat.id << ": ";

      switch (feat.type) {
      case FeatureType::kBool:
        name_file << "t, f.";
        break;
      case FeatureType::kInt:
        name_file << "continuous.";
        break;
      }
      name_file << '\n';
    }
    name_file << '\n';

    // Output a column for the target parameter
    name_file << "parameter_" << param.id << ": ";
    switch (param.type) {
    case ParameterType::kBool:
      name_file << "t, f.";
      break;
    case ParameterType::kRange:
      name_file << "continuous.";
      break;
    default:
      break;
    }
    name_file << '\n';
    name_file.close();

    // For the current parameter, output a data (.data) file containing the
    // training data.
    MAGEEC_DEBUG("Building .data file");
    std::ofstream data_file("/tmp/parameter_" + std::to_string(param.id) +
                                ".data",
                            std::ofstream::binary);

    for (auto res : results) {
      ParameterSet parameters = res.first.getParameters();
      FeatureSet features = res.first.getFeatures();

      // Check that this result has an entry for this parameter. If not then
      // skip as we can't use it for training.
      // FIXME: Don't use a dumb linear search here
      ParameterBase *p = nullptr;
      for (auto it : parameters) {
        if (it->getID() == param.id) {
          p = it.get();
        }
      }

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
            data_file << (value ? "t" : "f");
            break;
          }
          case FeatureType::kInt: {
            int64_t value = static_cast<IntFeature *>(f)->getValue();
            data_file << value;
            break;
          }
          }
          data_file << ",";
        } else {
          // No value for this feature for this result.
          data_file << "?,";
        }
      }
      // Output the parameter value last.
      assert(p->getType() == param.type);

      switch (param.type) {
      case ParameterType::kBool: {
        bool value = static_cast<BoolParameter *>(p)->getValue();
        data_file << (value ? "t" : "f");
        break;
      }
      case ParameterType::kRange: {
        int64_t value = static_cast<RangeParameter *>(p)->getValue();
        data_file << value;
        break;
      }
      default:
        break;
      }
      data_file << "\n";
    }

    // Now we have .names and .data files, run the classifier over them to
    // generate a tree
    // TODO: Running command on windows?
    MAGEEC_DEBUG("Running the C5.0 classifier");
    std::string command_str("c5.0 -f /tmp/parameter_" +
                            std::to_string(param.id));
    FILE *fpipe = popen(command_str.c_str(), "r");
    if (!fpipe) {
      assert(0 && "Process spawn failed!");
    }

    // Read until we get EOF
    std::array<uint8_t, 1024> tree_str;
    while (!feof(fpipe)) {
      fgets(reinterpret_cast<char *>(tree_str.data()), 1024, fpipe);
    }

    // Read in the generated tree to be stored in the machine learner blob
    MAGEEC_DEBUG("Reading the generate .tree file");
    std::ifstream tree_file("/tmp/parameter_" + std::to_string(param.id) +
                                ".tree",
                            std::ifstream::binary);
    if (tree_file) {
      tree_file.seekg(0, tree_file.end);
      std::streampos tree_size = tree_file.tellg();

      std::vector<uint8_t> tree_blob;
      tree_blob.reserve(static_cast<size_t>(tree_size));

      tree_file.seekg(0, tree_file.beg);
      tree_file.read(reinterpret_cast<char *>(tree_blob.data()), tree_size);

      // save the tree for the current parameter
      context->parameter_classifier_trees.insert(
          std::make_pair(param.id, tree_blob));
    }
    tree_file.close();
  }

  MAGEEC_DEBUG("Training passes");

  // Create a classifier trained for each of the input passes in turn
  for (auto pass : passes) {
    MAGEEC_DEBUG("Training for pass '" << pass << "'");

    // Output names file (columns for classifier) for this pass
    // TODO: Platform independent temporary file
    MAGEEC_DEBUG("Building .names file");
    std::ofstream name_file("/tmp/pass_" + pass + ".names",
                            std::ofstream::binary);

    // Output the target pass first
    // TODO: Comment containing pass description
    name_file << "pass_" << pass << ".\n";

    // Output columns for all of the features which we have seen in the
    // training set.
    // TODO: Add comment containing feature description
    for (auto feat : feature_descs) {
      name_file << "feature_" << feat.id << ": ";

      switch (feat.type) {
      case FeatureType::kBool:
        name_file << "t, f.";
        break;
      case FeatureType::kInt:
        name_file << "continuous.";
        break;
      }
      name_file << '\n';
    }
    name_file << '\n';

    // Output a column for the target pass
    name_file << "pass_" << pass << ": t, f.\n";
    name_file.close();

    // For the current pass, output a data (.data) file containing the
    // training data.
    MAGEEC_DEBUG("Building .data file");
    std::ofstream data_file("/tmp/pass_" + pass + ".data",
                            std::ofstream::binary);
    for (auto res : results) {
      FeatureSet features = res.first.getFeatures();
      ParameterSet parameters = res.first.getParameters();

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
            data_file << (value ? "t" : "f");
            break;
          }
          case FeatureType::kInt: {
            int64_t value = static_cast<IntFeature *>(f)->getValue();
            data_file << value;
            break;
          }
          }
          data_file << ",";
        } else {
          // No value for this feature for this result.
          data_file << "?,";
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
      data_file << (run_pass ? "t" : "f");
      data_file << "\n";
    }

    // Now we have .names and .data files, run the classifier over them to
    // generate a tree
    // TODO: Running command on windows?
    MAGEEC_DEBUG("Running the C5.0 classifier");
    std::string command_str("c5.0 -f /tmp/pass_" + pass);
    FILE *fpipe = popen(command_str.c_str(), "r");
    if (!fpipe) {
      assert(0 && "Process spawn failed!");
    }

    // Read until we get EOF
    std::array<uint8_t, 1024> tree_str;
    while (!feof(fpipe)) {
      fgets(reinterpret_cast<char *>(tree_str.data()), 1024, fpipe);
    }

    // Read in the generated tree to be stored in the machine learner blob
    MAGEEC_DEBUG("Reading the generated .tree file");
    std::ifstream tree_file("/tmp/pass_" + pass + ".tree",
                            std::ifstream::binary);
    if (tree_file) {
      tree_file.seekg(0, tree_file.end);
      std::streampos tree_size = tree_file.tellg();

      std::vector<uint8_t> tree_blob;
      tree_blob.reserve(static_cast<size_t>(tree_size));

      tree_file.seekg(0, tree_file.beg);
      tree_file.read(reinterpret_cast<char *>(tree_blob.data()), tree_size);

      // save the tree for the current parameter
      context->pass_classifier_trees.insert(std::make_pair(pass, tree_blob));
    }
    tree_file.close();
  }
  MAGEEC_DEBUG("Training finished");

  // Serialize the context to a blob
  return context->toBlob();
}

} // end of namespace mageec
