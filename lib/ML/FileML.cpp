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


//===------------------------ File Machine Learner ------------------------===//
//
// This implements the file based machine learner for MAGEEC. FileML does
// not make any decisions of its own, instead relying on the user to
// predefine the decisions that should be made.
//
// As the user provides the decisions, it does not need to store anything
// in the database.
//
//===----------------------------------------------------------------------===//

#include <cassert>
#include <cctype>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include "mageec/Database.h"
#include "mageec/Decision.h"
#include "mageec/FeatureSet.h"
#include "mageec/ML.h"
#include "mageec/ML/FileML.h"
#include "mageec/Util.h"


namespace mageec {


// UUID for FileML
const util::UUID FileML::uuid =
    util::UUID::parse("8998d958-5d9c-4b6b-a90b-679d8722d63a").get();


FileML::FileML()
  : IMachineLearner(),
    m_have_decision_config(false), m_decision_map()
{}

FileML::~FileML()
{}

bool FileML::setDecisionConfig(std::string config_path)
{
  // Load the input config file for parsing
  std::ifstream config_file(config_path);
  if (!config_file) {
    return false;
  }

  std::string line;
  while (std::getline(config_file, line)) {
    std::string request_id;
    std::string result_value;

    auto line_it = line.begin();
    for (; std::isspace(*line_it) && line_it != line.end(); line_it++);
    if (line_it == line.end() || *line_it == '#') {
      continue;
    }

    // read the id of the request
    for (; !std::isspace(*line_it) && line_it != line.end(); line_it++) {
      request_id.push_back(*line_it);
    }
    for (; std::isspace(*line_it) && line_it != line.end(); line_it++);

    // read the result value
    if (line_it == line.end() || *line_it == '#') {
      // no result value field
      MAGEEC_DEBUG("Malformed FileML config file field:\n" << line);
      return false;
    }
    for (; !std::isspace(*line_it) && line_it != line.end(); line_it++) {
      result_value.push_back(*line_it);
    }

    for (; std::isspace(*line_it) && line_it != line.end(); line_it++);
    if (line_it != line.end() && *line_it != '#') {
      // junk on the end of the line
      MAGEEC_DEBUG("Malformed FileML config file field:\n" << line);
      return false;
    }

    if (m_decision_map.count(request_id) != 0) {
      MAGEEC_DEBUG("Multiple entries for decision: " << request_id
          << " in the config file");
      return false;
    }

    assert(request_id.length() > 0);
    assert(result_value.length() > 0);

    m_decision_map[request_id] = result_value;
  }

  if (m_decision_map.size() == 0) {
    MAGEEC_DEBUG("No entries in the FileML config file\n");
  }
  m_have_decision_config = true;
  return true;
}


std::unique_ptr<DecisionBase>
FileML::makeDecision(const DecisionRequestBase& request,
                     const FeatureSet&,
                     const std::vector<uint8_t>&) const
{
  // String identifier used to find the decision to be made in the
  // decision map. Most requests have integer ids, so these are converted
  // to integers first.
  // NOTE: There is a chance of a name conflict if a request uses a
  // string identifier which is also a valid integer.
  std::string request_id;

  // The type of the resultant decision, this provides the information
  // needed to parse the result field in the decision map.
  DecisionType decision_type;

  switch (request.getType()) {
  case DecisionRequestType::kBool:
  case DecisionRequestType::kRange:
  case DecisionRequestType::kPassList: {
    // Get the identifier for the decision request as a string.
    // Also retrieve the expected type of the resultant decision.
    if (request.getType() == DecisionRequestType::kBool) {
      auto &bool_request = *static_cast<const BoolDecisionRequest*>(&request);
      request_id = std::to_string(bool_request.getID());
      decision_type = bool_request.getDecisionType();
    }
    else if (request.getType() == DecisionRequestType::kRange) {
      auto &range_request = *static_cast<const RangeDecisionRequest*>(&request);
      request_id = std::to_string(range_request.getID());
      decision_type = range_request.getDecisionType();
    }
    else if (request.getType() == DecisionRequestType::kPassList) {
      auto &pass_request =
          *static_cast<const PassListDecisionRequest*>(&request);
      request_id = std::to_string(pass_request.getID());
      decision_type = pass_request.getDecisionType();
    }
    else {
      assert(0 && "Unreachable");
    }
    break;
  }
  case DecisionRequestType::kPassGate: {
    auto &pass_request = *static_cast<const PassGateDecisionRequest*>(&request);
    request_id = pass_request.getID();
    decision_type = pass_request.getDecisionType();
    break;
  }
  }

  // See if there is a decision in the map for the current request id.
  // If not, just return the native decision
  if (m_decision_map.count(request_id) == 0) {
    return std::unique_ptr<DecisionBase>(new NativeDecision());
  }

  const auto result_iter = m_decision_map.find(request_id);
  assert(result_iter != m_decision_map.end());
  std::string result_str = result_iter->second;

  switch (decision_type) {
  case DecisionType::kBool:
    if (result_str == "true") {
      return std::unique_ptr<DecisionBase>(new BoolDecision(true));
    }
    else if (result_str == "false") {
      return std::unique_ptr<DecisionBase>(new BoolDecision(false));
    }
    MAGEEC_DEBUG("Unknown value for boolean decision:\n" << result_str);
    return std::unique_ptr<DecisionBase>(new NativeDecision());
  case DecisionType::kRange: {
    int64_t result_value;
    std::istringstream result_stream(result_str);

    result_stream >> result_value;
    if (!result_stream.fail()) {
      return std::unique_ptr<DecisionBase>(new RangeDecision(result_value));
    }
    MAGEEC_DEBUG("Unknown value for range decision:\n" << result_str);
    return std::unique_ptr<DecisionBase>(new NativeDecision());
  }
  case DecisionType::kPassList:
    assert(0 && "Unsupport decision type");
    return std::unique_ptr<DecisionBase>(new NativeDecision());
  case DecisionType::kNative:
    assert(0 && "Unreachable");
    return std::unique_ptr<DecisionBase>(new NativeDecision());
  }
}


const std::vector<uint8_t>
FileML::train(std::set<FeatureDesc>, std::set<ParameterDesc>,
              std::set<std::string>,
              ResultIterator) const
{
  assert(0 && "FileML should not be trained");
  return std::vector<uint8_t>();
}


const std::vector<uint8_t>
FileML::train(std::set<FeatureDesc>, std::set<ParameterDesc>,
              std::set<std::string>,
              ResultIterator,
              std::vector<uint8_t>) const
{
  assert(0 && "FileML should not be trained");
  return std::vector<uint8_t>();
}


} // end of namespace mageec 
