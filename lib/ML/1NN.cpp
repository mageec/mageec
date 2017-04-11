/*  Copyright (C) 2017, Embecosm Limited

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

//===----------------------- MAGEEC 1-NN Classifier -----------------------===//
//
// This implements an incredibly naive 1-NN 'machine learner'. This finds the
// closest feature set in the training set to the input feature set, and then
// uses that configuration to make decisions.
//
//===----------------------------------------------------------------------===//

#include "mageec/Database.h"
#include "mageec/ML/1NN.h"
#include "mageec/ML.h"
#include "mageec/Result.h"
#include "mageec/Types.h"
#include "mageec/Util.h"

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <limits>

namespace mageec {

OneNN::OneNN() : IMachineLearner() {}
OneNN::~OneNN() {}

std::unique_ptr<DecisionBase>
OneNN::makeDecision(const DecisionRequestBase &request,
                    const FeatureSet &features,
                    const std::vector<uint8_t> &blob) const {
  // Deserialize from the blob
  std::map<unsigned, std::pair<double, double>> feature_max_min;
  std::vector<OneNN::Point> feature_points;
  
  auto it = blob.cbegin();

  // Read the number of features, followed by the feature ids, and the
  // min and max ranges for each feature
  // |    16     |  16  | 64  | 64  |...
  // |NumFeatures|FeatID| max | min |...
  unsigned n_features = util::read16LE(it);
  for (unsigned i = 0; i < n_features; ++i) {
    unsigned feature_id = util::read16LE(it);
    uint64_t max = util::read64LE(it);
    uint64_t min = util::read64LE(it);
    // FIXME: Make this safe and portable
    feature_max_min[feature_id] =
        std::pair<double, double>(*reinterpret_cast<double *>(&max),
                                  *reinterpret_cast<double *>(&min));
  }
  // Read the number of feature points, followed by each feature point in
  // turn.
  // |   16    |    ??      |    ??      |
  // |NumPoints|FeaturePoint|FeaturePoint|...
  unsigned n_points = util::read16LE(it);
  for (unsigned i = 0; i < n_points; ++i) {
    // Read each feature point. This consists of each feature value in turn,
    // followed by each parameter in turn
    // |    16     |  16  | 64  |...|      16     |  16   | 64  |...
    // |NumFeatures|FeatID|value|...|NumParameters|ParamID|value|...
    unsigned tmp_n_features = util::read16LE(it);
    assert(tmp_n_features == n_features);

    std::map<unsigned, double> features;
    for (unsigned j = 0; j < n_features; ++j) {
      unsigned id = util::read16LE(it);
      uint64_t value = util::read64LE(it);
      features[id] = *reinterpret_cast<double*>(&value);
    }
    unsigned n_parameters = util::read16LE(it);
    std::map<unsigned, int64_t> parameters;
    for (unsigned j = 0; j < n_parameters; ++j) {
      unsigned id = util::read16LE(it);
      int64_t value = util::read64LE(it);
      parameters[id] = value;
    }
    OneNN::Point point = {features, parameters};
    feature_points.push_back(point);
  }

  // Take the input features and normalize them
  std::map<unsigned, double> query_features;
  for (auto f : features) {
    double max = feature_max_min[f->getID()].first;
    double min = feature_max_min[f->getID()].second;
    switch(f->getType()) {
    case FeatureType::kBool: {
      bool value = static_cast<BoolFeature *>(f.get())->getValue();
      double double_value = value ? 1.0 : 0;
      query_features[f->getID()] = double_value;
      break;
    }
    case FeatureType::kInt: {
      int64_t value = static_cast<IntFeature*>(f.get())->getValue();
      double double_value = static_cast<double>(value);
      if ((max - min) != 0.0)
        double_value = (double_value - min) / (max - min);
      else
        double_value = 0.0;
      query_features[f->getID()] = double_value;
      break;
    }
    }
  }

  // Find the closest point to the query point
  // TODO: Don't use a dumb n^2 search here
  double min_squared_distance = std::numeric_limits<double>::max();
  OneNN::Point nearest_neighbor;
  for (auto point : feature_points) {
    // calculate the distance between the query point and this point. If
    // any feature is missing, then just ignore it.
    for (auto query_feature : query_features) {
      double squared_distance = 0.0;
      if (point.features.count(query_feature.first)) {
        double value = point.features[query_feature.first];
        double query_value = query_feature.second;
        double diff = value - query_value;
        double squared_diff = diff * diff;
        squared_distance += squared_diff;
      }
      if (squared_distance < min_squared_distance) {
        min_squared_distance = squared_distance;
        nearest_neighbor = point;
      }
    }
  }

  // Get the parameter from the parameter set associated with the nearest
  // neighbor.
  DecisionRequestType request_type = request.getType();

  if (request_type == DecisionRequestType::kBool) {
    const auto *bool_request =
        static_cast<const BoolDecisionRequest *>(&request);
    unsigned param_id = bool_request->getID();

    if (nearest_neighbor.parameters.count(param_id)) {
      int64_t res = nearest_neighbor.parameters[param_id];
      return std::unique_ptr<BoolDecision>(new BoolDecision(res));
    }
    return std::unique_ptr<NativeDecision>(new NativeDecision());
  } else if (request_type == DecisionRequestType::kRange) {
    const auto *range_request =
        static_cast<const RangeDecisionRequest *>(&request);
    unsigned param_id = range_request->getID();

    if (nearest_neighbor.parameters.count(param_id)) {
      int64_t res = nearest_neighbor.parameters[param_id];
      return std::unique_ptr<RangeDecision>(new RangeDecision(res));
    }
    return std::unique_ptr<NativeDecision>(new NativeDecision());
  } else {
    assert(0 && "Unhandled decision request type");
  }
  return std::unique_ptr<NativeDecision>(new NativeDecision());
}

const std::vector<uint8_t>
OneNN::train(std::set<FeatureDesc> feature_descs,
             std::set<ParameterDesc>,
             std::set<std::string>,
             ResultIterator result_iter) const {
  // Read all of the results data in one go. For each distinct set of input
  // features, store the best set of results. This is achieved by storing a
  // hash from a feature set to its current best result.
  MAGEEC_DEBUG("Collecting results");
  std::map<uint64_t, Result> result_map;
  for (util::Option<Result> result; (result = *result_iter);
       result_iter = result_iter.next()) {
    FeatureSet features = result.get().getFeatures();
    double value = result.get().getValue();
    uint64_t hash = features.hash();

    bool result_handled = false;
    while (!result_handled) {
      auto curr_entry = result_map.find(hash);
      if (curr_entry != result_map.end()) {
        const FeatureSet &curr_features = curr_entry->second.getFeatures();
        double curr_value = curr_entry->second.getValue();

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

  std::map<unsigned, FeatureType> feature_type;
  std::map<unsigned, std::pair<double, double>> feature_max_min;
  std::vector<OneNN::Point> feature_points;

  // Get all of the feature ids and their types
  for (auto desc : feature_descs) {
    // Only integer and boolean feature types are allowed at the moment
    if (desc.type == FeatureType::kBool ||
        desc.type == FeatureType::kInt) {
      feature_type[desc.id] = desc.type;
    }
  }

  // Find the max and min of each feature
  for (auto res : result_map) {
    FeatureSet features = res.second.getFeatures();
    for (auto f : features) {
      assert(feature_type.count(f->getID()));
      assert(feature_type[f->getID()] == f->getType());

      switch (f->getType()) {
      case FeatureType::kBool: {
        if (feature_max_min.count(f->getID()) == 0)
          feature_max_min[f->getID()] = std::make_pair<double, double>(1.0, 0.0);
      }
      case FeatureType::kInt: {
        int64_t value = static_cast<IntFeature *>(f.get())->getValue();
        double double_value = static_cast<double>(value);
        if (feature_max_min.count(f->getID()) == 0) {
          feature_max_min[f->getID()] =
              std::pair<double, double>(double_value, double_value);
        } else {
          auto &entry = feature_max_min[f->getID()];
          if (double_value < entry.first)
            entry.first = double_value;
          if (double_value > entry.second)
            entry.second = double_value;
        }
      }
      }
    }
  }

  // Add a point for each feature set, normalize the features in the process to
  // the range [0, 1]
  for (auto res : result_map) {
    ParameterSet parameters = res.second.getParameters();
    FeatureSet features = res.second.getFeatures();

    OneNN::Point point;
    for (auto f : features) {
      assert(feature_type[f->getID()] == f->getType());

      switch (f->getType()) {
      case FeatureType::kBool: {
        bool value = static_cast<BoolFeature *>(f.get())->getValue();
        point.features[f->getID()] = value ? 1.0 : 0.0;
        break;
      }
      case FeatureType::kInt: {
        int64_t value = static_cast<IntFeature *>(f.get())->getValue();
        double double_value = static_cast<double>(value);
        double max = feature_max_min[f->getID()].first;
        double min = feature_max_min[f->getID()].second;
        if ((max - min) != 0.0)
          double_value = (double_value - min) / (max - min);
        else
          double_value = 0.0;
        point.features[f->getID()] = double_value;
        break;
      }
      }
    }
    for (auto p : parameters) {
      switch(p->getType()) {
      case ParameterType::kBool: {
        bool value = static_cast<BoolParameter*>(p.get())->getValue();
        point.parameters[p->getID()] = value;
        break;
      } 
      case ParameterType::kRange: {
        int64_t value = static_cast<RangeParameter*>(p.get())->getValue();
        point.parameters[p->getID()] = value;
        break;
      }
      default:
        assert(0 && "Unhandled parameter type");
        break;
      }
    }
    feature_points.push_back(point);
  }

  // Serialize to a blob
  // Buffer used to store the training blob
  std::vector<uint8_t> blob;

  // Emit the number of features, followed by the feature ids, and the
  // min and max ranges for each feature
  // |    16     |  16  | 64  | 64  |...
  // |NumFeatures|FeatID| max | min |...
  util::write16LE(blob, feature_max_min.size());
  for (auto feature_max_min : feature_max_min) {
    util::write16LE(blob, feature_max_min.first);
    // FIXME: Make this safe and portable
    util::write64LE(blob, *reinterpret_cast<uint64_t*>(&feature_max_min.second.first));
    util::write64LE(blob, *reinterpret_cast<uint64_t*>(&feature_max_min.second.second));
  }
  // Emit the number of feature points, followed by each feature point in
  // turn.
  // |   16    |    ??      |    ??      |
  // |NumPoints|FeaturePoint|FeaturePoint|...
  util::write16LE(blob, feature_points.size());
  for (auto point : feature_points) {
    // Emit each feature point. This consists of each feature value in turn,
    // followed by each parameter in turn
    // |    16     |  16  | 64  |...|      16     |  16   | 64  |...
    // |NumFeatures|FeatID|value|...|NumParameters|ParamID|value|...
    util::write16LE(blob, point.features.size());
    for (auto feature : point.features) {
      util::write16LE(blob, feature.first);
      // FIXME: Make this safe and portable
      util::write64LE(blob, *reinterpret_cast<uint64_t*>(&feature.second));
    }
    util::write16LE(blob, point.parameters.size());
    for (auto parameter : point.parameters) {
      util::write16LE(blob, parameter.first);
      util::write64LE(blob, *reinterpret_cast<uint64_t*>(&parameter.second));
    }
  }
  return blob;
}

} // end of namespace mageec
