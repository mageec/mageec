/*  MAGEEC Machine Learning Interface
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

/** @file mageec-ml.h MAGEEC Machine Learner Interface. */
#ifndef __MAGEEC_ML_H__
#define __MAGEEC_ML_H__

#include <string>
#include <vector>
#include <stdint.h>
#include "mageec-decision.h"
#include "mageec-features.h"
#include "mageec-passes.h"
#include "mageec-result.h"
#include "mageec-db.h"

namespace mageec
{
/**
 * MAGEEC Machine Learner
 */
class mageec_ml
{
  database *db;
  std::vector<mageec_pass*> known_passes;
public:
  virtual ~mageec_ml() {}

  /**
   * Initilizes the MAGEEC Machine Learner
   * @param dbfilename Path of database file to open.
   * @returns 0 if MAGEEC successfully set up, 1 otherwise.
   */
  virtual int init (std::string dbfilename);

  /**
   * Initilizes the MAGEEC Machine Learner
   * @param compiler_version Compiler and version, e.g. GCC-4.8.
   * @param compiler_target Compiler target, e.g. arm-none-gnueabi.
   * @returns 0 if MAGEEC successfully set up, 1 otherwise.
   */
  virtual int init (std::string compiler_version,
                    std::string compiler_target);
  /**
   * Prepares Machine Learner for a new file.
   */
  void new_file (void);

  /**
   * Declares to Machine Learner processing has finished.
   */
  void end_file (void);

  /**
   * Hook for Machine Learner to e.g. disconnect for any resources.
   */
  void finish (void);

  /**
   * Provide a set of program features to the machine learner to base decisions
   * on.
   * @param features List of features for program.
   */
  void provide_features (std::vector<mageec_feature*> features);

  /**
   * Returns a set of passes the machine learner has chosen the compiler to
   * execute.
   * @returns list of passes to execute.
   */
  std::vector<mageec_pass> get_passes();

  /**
   * Returns complete list of passes known to machine learner for disabling
   * upon initialisation.
   * @returns list of all passes.
   */
  std::vector<mageec_pass*> all_passes();

  /**
   * Make a single function/pass decision based on provided feature vector.
   * @param pass Pass to make a decision about.
   * @param features Feature vector of function to run pass on.
   * @returns decision
   */
  virtual decision make_decision (mageec_pass* pass,
                                  std::vector<mageec_feature*> features);

  /**
   * Informs the machine learner that it can process all new data to update
   * internal decisions.
   */
  void process_results();

  /** 
   * Adds a result to the machine learner database.
   * (Depending on the algorithm used, this may be for training purposes only)
   * @param features List of features for program.
   * @param passes List of passes executed during compilation.
   * @param metric Performance metric for optimisation.
   * @param good Whether the result of compilation was valid (for machine
   * learners that understand bad combinations.)
   */
  void add_result (std::vector<mageec_feature*> features,
                   std::vector<mageec_pass*> passes,
                   int64_t metric,
                   bool good);

  /**
   * Adds a result to the machine learner database.
   * @param res Result structure containing program, pass and metric info
   */
  void add_result (result res);

  /**
   * Adds a set of result points to machine learner raw database.
   * @param features List of features for program.
   * @param passes List of passes executed during compilation.
   * @param metrics Performance metrics.
   * @param good Whether the result of compilation was valid (for machine
   * learners that understand bad combinations.)
   */   
  void raw_result (std::vector<mageec_feature*> features,
                   std::vector<mageec_pass*> passes,
                   std::vector<int64_t> metrics,
                   bool good);
};

/**
 * File based "Machine Learner"
 *
 * This is a static decision maker that loads a file containing passes to
 * execute. This is intended to be used for generating training data.
 */
class file_ml : public mageec_ml
{
  std::vector<std::string> passlist;
public:
  virtual int init (std::string dbfilename);
  virtual int init (std::string compiler_version,
                    std::string compiler_target);
  virtual decision make_decision (mageec_pass* pass,
                                  std::vector<mageec_feature*> features);
};

} // End namespace mageec

#endif
