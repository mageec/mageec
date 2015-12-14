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

#include <vector>

#include "ML.h"


namespace {

/// \brief Data stored in the blob held in the database for the machine
/// learner
struct C5Context {
  C5Context();

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
  static std::unique_ptr<C5Context> fromBlob(const std:vector<uint8_t> &blob);


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


/// \brief Comparator to sort results into ascending order based on their
/// metric value.
struct ResultComparator {
  bool operator() (const Result &lhs, const Result &rhs) const {
    return lhs.getValue() < rhs.getValue();
  }
};


/// \brief Types of field found in the machine learner blob for the C5.0
/// classifier.
enum class C5BlobField {
  kFeatureDesc,
  kParameterDesc,
  kPassName,
  kParameterClassifierTree,
  kPassClassifierTree
};

} // end of anonymous namespace


static std::unique_ptr<C5Context>
C5Context::fromBlob(std::vector<uint8_t> blob)
{
  std::unique_ptr<C5Context> context(new C5Context());

  auto it = blob.cbegin();
  while (it != blob.cend()) {
    C5BlobField field = static_cast<C5BlobField>(util::read16LE(it));
    switch (field) {
    case C5BlobField::kFeatureDesc: {
      // | feat_id | feat_type |
      unsigned feat_id = util::read16LE(it);
      FeatureType feat_type =
          static_cast<FeatureType>(util::read16LE(it));
      context.feature_descs.insert(feat_id, feat_type);
    }
    case C5BlobField::kParameterDesc: {
      // | param_id | param_type |
      unsigned param_id = util::read16LE(it);
      ParameterType param_type =
          static_cast<ParameterType>(util::read16LE(it));
      context.parameter_descs.insert(param_id, param_type);
    }
    case C5BlobField::kPassName: {
      // | pass_name_len | pass_name |
      unsigned len = util::read16LE(it);
      std::string pass_name(it, len);
      it += len;
      context.passes.insert(pass_name);
    }
    case C5BlobField::kParameterClassifierTree: {
      // | param_id | classifier_len | classifier_blob |
      unsigned param_id = util::read16LE(it);
      unsigned len = util::read16LE(it);
      std::vector<uint8_t> classifier_blob(len);
      std::copy(it, it + len, classifier_blob);
      it += len;

      context.parameter_classifier_trees.push_back(param_id, classifier_blob);
    }
    case C5BlobField::kPassClassifierTree: {
      // | pass_name_len | pass_name | classifier_len | classifier_blob |
      unsigned pass_name_len = util::read16LE(it);
      std::string pass_name(it, pass_name_len);
      it += pass_name_len;

      unsigned classifier_len = util::read16LE(it);
      std::vector<uint8_t> classifier_blob(len);
      std::copy(it, it + classifier, classifier_blob);
      it += classifier_len;

      context.pass_classifier_trees.push_back(pass_name, classifier_blob);
    }
    default:
      assert(0 && "Unknown field of C5.0 classifier blob!");
  }
  return context;
}


std::vector<uint8_t> C5Context::toBlob()
{
  // Buffer used to store the training blob
  std::vector<uint8_t> blob;

  // Store descriptions of features/parameters and passes that were trained
  // against.
  for (auto feat : feature_descs) {
    // | feat_id | feat_type |
    unsigned feat_id = feat.id;
    unsigned feat_type = static_cast<unsigned>(f.type);

    util::write16LE(blob, static_cast<unsigned>(C5BlobField::kFeatureDesc);
    util::write16LE(blob, feat_id);
    util::write16LE(blob, feat_type);
  }
  for (auto param : parameter_descs) {
    // | param_id | param_type |
    unsigned param_id = param.id;
    unsigned param_type = static_cast<unsigned>(p.type);

    util::write16LE(blob, static_cast<unsigned>(C5BlobField::kParameterDesc);
    util::write16LE(blob, param_id);
    util::write16LE(blob, param_type);
  }
  for (auto pass : passes) {
    // | pass_name_len | pass_name |
    std::string pass_name = pass;
    unsigned pass_name_len = static_cast<unsigned>(pass_name.length());

    util::write16LE(blob, static_cast<unsigned>(C5BlobField::kPassDesc)
    util::write16LE(blob, pass_name_len);
    for (auto it : pass_name) {
      blob.push_back(*it);
    }
  }

  // Store the classifier tree for each of the parameters
  for (auto param_tree : parameter_classifier_trees) {
    // | param_id | classifier_len | classifier_blob |
    unsigned param_id = param_tree.first;
    std::vector<uint8_t> classifier_blob = param_tree.second;
    unsigned classifier_len = static_cast<unsigned>(classifier_blob.length());

    util::write16LE(blob,
        static_cast<unsigned>(C5BlobField::kParameterClassifierTree));
    util::write16LE(blob, param_id);
    util::write16LE(blob, classifier_len);
    for (auto it : classifier_blob) {
      blob.push_back(*it);
    }
  }

  // Store the classifier tree for each of the passes
  for (auto pass_tree : pass_classifier_trees) {
    // | pass_name_len | pass_name | classifier_len | classifier_blob |
    std::string pass_name = pass_tree.first;
    unsigned pass_name_len = static_cast<unsigned>(pass_name.length());
    std::vector<uint8_t> classifier_blob = pass_tree.second;
    unsigned classifier_len = static_cast<unsigned>(classifier_blob.length());

    util::write16LE(blob,
        static_cast<unsigned>(C5BlobField::kPassClassifierTree));
    util::write16LE(blob, pass_name_len);
    for (auto it : pass_name) {
      blob.push_back(*it);
    }
    util::write16LE(blob, classifier_len);
    for (auto it : classifier_blob) {
      blob.push_back(*it);
    }
  }
  return blob;
}


/// \brief Machine learner which drives an external C5.0 classifier
class C5Driver : IMachineLearner {
private:
  /// Version 4 UUID
  ///
  /// This identifies both the machine learner and its version. If a
  /// breaking change is made to the machine learner then a new UUID should
  /// be generated in order to avoid conflicts.
  static const util::UUID uuid;

public:
  C5Driver() = default;
  ~C5Driver() override;

  /// \brief Get the unique identifier for this machine learner
  util::UUID getUUID(void) const override {
    return uuid;
  }

  /// \brief Get the name of the machine learner
  /// 
  /// Note that this is a driver in case a C5.0 classifier is later included
  /// with MAGEEC.
  std::string getName(void) const override { return "C5.0 Driver"; }

  std::unique_ptr<DecisionBase>
  makeDecision(const DecisionRequestBase& request,
               const FeatureSet& features,
               const std::vector<uint8_t>& blob) const override;

  const std::vector<uint8_t>
  train(std::set<FeatureDesc> feature_descs,
        std::set<ParameterDesc> parameter_descs,
        std::set<std::string> passes,
        ResultIterator results) const override;

  const std::vector<uint8_t>
  train(std::set<FeatureDesc> feature_descs,
        std::set<ParameterDesc> parameter_descs,
        std::set<std::string> passes,
        ResultIterator results
        std::vector<uint8_t> old_blob) const override;
};

const util::UUID C5Driver::uuid =
    util::UUID::parse("ccf7593c-78d9-429a-b544-2a7339d4325e");


std::unique_ptr<DecisionBase>
C5Driver::makeDecision(const DecisionRequestBase& request,
                       const FeatureSet& features,
                       const std::vector<uint8_t>& blob) const
{
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
      const auto &bool_request =
          static_cast<const BoolDecisionRequest>(request);
      param_id = bool_request.getID();

      assert(bool_request.getDecisionType() == DecisionType::kBool);
      param_type = ParameterType::kBool;
    }
    else if (param_type == DecisionRequestType::kRange) {
      const auto &range_request =
          static_cast<const RangeDecisionRequest>(request);
      param_id = range_request.getID();

      assert(range_request.getDecisionType() == DecisionType::kRange);
      param_type = ParameterType::kRange;
    }

    // Check if we have a classifier tree for this parameter.
    const auto res = context.parameter_classifier_trees.find(param_id);
    if (res == context.parameter_classifier_trees.cend()) {
      return std::unique_ptr<DecisionBase>(new NativeDecision());
    }

    // Output the classifier tree to a file (.tree)
    const std::vector<uint8_t> &classifier_tree = *res;
    std::ofstream name_file("/tmp/parameter_" + param_id + ".tree");
    name_file << classifier_tree;


    // Output names file (columns for classifier) for this parameter
    // FIXME: This is copied from the 'train' code and should be factored out
    // TODO: Platform independent temporary file
    std::ofstream name_file("/tmp/parameter_" + param_id + ".names");

    // Output the target parameter first
    // TODO: Comment containing parameter description
    name_file << "parameter_" << param_id << "." << std::endl;

    // Output columns for all of the features which we have seen in the
    // training set.
    // TODO: Add comment containing feature description
    for (auto feat : context.feature_descs) {
      std::string feat_id(feat.first);
      name_file << "feature_" << feat_id << ": "

      switch (feat.second) {
      case FeatureType::kBool:
        name_file << "t, f.";
        break;
      case FeatureType::kInt:
        name_file << "continuous."
        break;
      }
      name_file << std::endl;
    }
    name_file << std::endl;

    // Output a column for the target parameter
    name_file << "parameter_" << param_id << ": ";
    switch (param_type) {
    case ParameterType::kBool:
      name_file << "t, f."
      break;
    case ParameterType::kRange:
      name_file << "continuous."
      break;
    }
    name_file << std::endl;
    name_file.close();


    // Output cases files (.cases), containing the feature set
    std::ofstream cases_file("/tmp/parameter_" + param_id + ".cases");
    
    for (auto feat : feature_descs) {
      // Featre values are output in the order they appear in the feature
      // description map (ascending order of feature id)
      FeatureBase *f = features.get(feat.first);
      if (f) {
        // There is a value for this feature in the feature set, output it
        assert(f->getType() == feat.second);

        switch (f.second) {
        case FeatureType::kBool: {
          bool value = static_cast<BoolFeature*>(f)->getValue();
          cases_file << (value ? "t" : "f");
          break;
        }
        case FeatureType::kInt: {
          int64_t value = static_cast<IntFeature*>(f)->getValue();
          cases_file << value;
          break;
        }
        default:
          assert(0 && "Unhandled FeatureType!");
        }
      }
      else {
        // No value for this feature in the feature set
        cases_file << "?,";
      }
    }
    // Finally, add the 'unknown' for our target parameter
    cases_file << "?" << std::endl;
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
    const auto &bool_request = static_cast<const BoolDecisionRequest>(request);
    assert(bool_request.getDecisionType() == DecisionType::kBool);
    return new std::unique_ptr<BoolDecision>(new BoolDecision(true));
  }
  case DecisionRequestType::kRange: {
    const auto &range_request = static_cast<const RangeDecisionRequest>(request);
    assert(range_request.getDecisionType() == DecisionType::kRange);
    return new std::unique_ptr<RangeDecision>(new RangeDecision(0xdeadbeef));
  }
  default:
    assert(0 && "Unhandled DecisionRequest type!");
    return nullptr;
  }
}


const std::vector<uint8_t>
C5Driver::train(std::set<FeatureDesc>   feature_descs,
                std::set<ParameterDesc> parameter_descs,
                std::set<std::string> passes,
                ResultIterator result_iter) const
{
  // Read all of the result data in one go.
  // C5 can only train on known 'good' results, so the results are stored
  // sorted by value so that it is possible to filter out bad results before
  // providing them to the machine learner.
  std::set<Result, ResultComparator> results;

  while ((util::Option<Result> r = *result_iter)) {
    results.emplace(r.get();
    result_iter = result_iter.next();
  }
  
  // Create a classifier trained for each tunable parameter in turn.
  for (auto param : parameter_descs) {
    std::string param_id(param.id);

    // Output names file (columns for classifier) for this parameter
    // TODO: Platform independent temporary file
    std::ofstream name_file("/tmp/parameter_" + param_id + ".names");

    // Output the target parameter first
    // TODO: Comment containing parameter description
    name_file << "parameter_" << param_id << "." << std::endl;

    // Output columns for all of the features which we have seen in the
    // training set.
    // TODO: Add comment containing feature description
    for (auto feat : feature_descs) {
      name_file << "feature_" << std::string(feat.id) << ": "

      switch (feat.type) {
      case FeatureType::kBool:
        name_file << "t, f.";
        break;
      case FeatureType::kInt:
        name_file << "continuous."
        break;
      }
      name_file << std::endl;
    }
    name_file << std::endl;

    // Output a column for the target parameter
    name_file << "parameter_" << param_id << ": ";
    switch (param.type) {
    case ParameterType::kBool:
      name_file << "t, f."
      break;
    case ParameterType::kRange:
      name_file << "continuous."
      break;
    }
    name_file << std::endl;
    name_file.close();

    // For the current parameter, output a data (.data) file containing the
    // training data.
    std::ofstream data_file("/tmp/parameter_" + param_id + ".data");
    
    ParameterSet parameters = res.getParameters();
    FeatureSet features = res.getFeatures();
    for (auto res : results) {
      // Check that this result has an entry for this parameter. If not then
      // skip as we can't use it for training.
      ParameterBase *p = parameters.get(param.id);
      if (!p) {
        continue;
      }

      for (auto feat : feature_descs) {
        // Feature values are output in the order they appear in the feature
        // description map (ascending order of feature id)
        FeatureBase *f = features.get(feat.id);
        if (f) {
          // The result has a value for this feature, output it
          assert(f->getType() == feat.second);

          switch (feat.second) {
          case FeatureType::kBool: {
            bool value = static_cast<BoolFeature*>(f)->getValue();
            name_file << (value ? "t" : "f");
            break;
          }
          case FeatureType::kInt: {
            int64_t value = static_cast<IntFeature*>(f)->getValue();
            name_file << value;
            break;
          }
          default:
            assert(0 && "Unhandled FeatureType");
          }
          name_file << ",";
        }
        else {
          // No value for this feature for this result.
          name_file << "?,";
        }
      }
      // Output the parameter value last.
      assert(p->getType() == param.second);

      switch (param.second) {
      case ParameterType::kBool: {
        bool value = static_cast<BoolParameter*>(p)->getValue();
        name_file << (value ? "t" : "f");
        break;
      }
      case ParameterType::kRange: {
        int64_t value = static_cast<RangeParameter*>(p)->getValue();
        name_file << value;
        break;
      }
      default:
        assert(0 && "Unhandled ParameterType");
      }
      name_file << ",";
    }

    // Now we have .names and .data files, run the classifier over them to
    // generate a tree
    // TODO: Running command on windows?
    std::string command_str("c5.0 -f /tmp/parameter_") + param_id;
    FILE *fpipe = popen(command_str.c_str(), "r");
    if (!fpipe) {
      assert(0 && "Process spawn failed!");
    }

    // Read until we get EOF
    std::array<uint8_t, 1024> tree_str;
    while (!feof(fpipe)) {
      fgets(tree_str.data(), 1024, fpipe);
    }

    // Read in the generated tree to be stored in the machine learner blob
    std::ifstream tree_file("/tmp/parameter_" + param_id + ".tree");
    if (tree_file) {
      tree_file.seekg(0, tree_file.end);
      std::streampos tree_size = tree_file.tellg();

      std::vector<uint8_t> tree_blob;
      tree_blob.reserve(tree_size);

      tree_file.seekg(0, tree_file.beg);
      tree_file.read(tree_blob.data(), tree_size);

      // save the tree for the current parameter
      context.parameter_classifier_trees.insert(param_id, tree_blob);
    }
    tree_file.close();
  }
  // Serialize the context to a blob
  return context.toBlob();
}


const std::vector<uint8_t>
C5Driver::train(std::set<FeatureDesc>   feature_descs,
                std::set<ParameterDesc> parameter_descs,
                std::set<std::string> passes,
                ResultIterator result_iter,
                std::vector<uint8_t> old_blob) const
{
  return old_blob;
}
