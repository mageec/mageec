/*  GIMPLE Feature Extractor
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

//===--------------------- GIMPLE Feature Extractor -----------------------===//
//
// Extracts features from the GCC gimple internal representation.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_GCC_FEATURE_EXTRACT_H
#define MAGEEC_GCC_FEATURE_EXTRACT_H

#include "mageec/AttributeSet.h"

#include <memory>
#include <vector>


/// \class FunctionFeatures
///
/// \brief Defines function level features extracted by the plugin
///
/// This holds function level features as they are extracted by the
/// feature extractor. This is later turned into a FeatureSet which can
/// be used with MAGEEC.
class FunctionFeatures {
public:
  /// \brief Create an empty set of FunctionFeatures to be populated
  FunctionFeatures() :
    args(0), cyclomatic_complexity(0), cfg_edges(0), cfg_abnormal_edges(0),
    critical_path_len(0),

    loops(0), loop_depth(),

    basic_blocks(0), bb_in_loop(0), bb_outside_loop(0),
    bb_pred(), bb_succ(),

    bb_instructions(), bb_cond_stmts(), bb_direct_calls(), bb_indirect_calls(),
    bb_int_ops(), bb_float_ops(), bb_unary_ops(), bb_ptr_arith_ops(),
    bb_uncond_brs(), bb_assign_stmts(), bb_switch_stmts(), bb_phi_nodes(),
    bb_phi_header_nodes(),

    phi_args(), 
    call_args(), call_ptr_args(), call_ret_int(0), call_ret_float(0)
  {}

  // General function features
  int64_t args;                    ///< Number of arguments to the function
  int64_t cyclomatic_complexity;   ///< Cyclomatic complexity of the function
  int64_t cfg_edges;               ///< Number of CFG edges in the function
  int64_t cfg_abnormal_edges;      ///< Number of abnormal CFG edges
  int64_t critical_path_len;       ///< Length of the critical path

  int64_t loops;                   ///< Number of loops
  std::vector<int64_t> loop_depth; ///< Depth of each loop as it is encountered

  // Basic block counts
  int64_t basic_blocks;            ///< Number of basic blocks
  int64_t bb_in_loop;              ///< Number of basic blocks inside a loop
  int64_t bb_outside_loop;         ///< Number of basic blocks outside a loop

  std::vector<int64_t> bb_pred;    ///< Number of predecessors for each block
  std::vector<int64_t> bb_succ;    ///< Number of successors for each block

  // Instructions counts (per basic block)
  /// Number of instructions in each basic block
  std::vector<int64_t> bb_instructions;
  /// Number of conditional statements in each basic block
  std::vector<int64_t> bb_cond_stmts;
  /// Number of direct calls per basic block
  std::vector<int64_t> bb_direct_calls;
  /// Number of indirect calls per basic block
  std::vector<int64_t> bb_indirect_calls;
  /// Number of integer operations per basic block
  std::vector<int64_t> bb_int_ops;
  /// Count of float operations per basic block
  std::vector<int64_t> bb_float_ops;
  /// Count of unary operations per basic block
  std::vector<int64_t> bb_unary_ops;
  /// Number of pointer arithmetic operations in each basic block
  std::vector<int64_t> bb_ptr_arith_ops;
  /// Count of unconditional branches in each basic block
  std::vector<int64_t> bb_uncond_brs;
  /// Assignment count in basic blocks
  std::vector<int64_t> bb_assign_stmts;
  /// Switch statements in basic block
  std::vector<int64_t> bb_switch_stmts;
  /// Phi nodes per basic block
  std::vector<int64_t> bb_phi_nodes;
  /// Phi header node count per basic block
  std::vector<int64_t> bb_phi_header_nodes;

  // Function instruction counts
  /// Number of arguments in each phi node
  std::vector<int64_t> phi_args;
  /// Number of arguments to each call instruction
  std::vector<int64_t> call_args;
  /// Number of pointer arguments to each call instruction
  std::vector<int64_t> call_ptr_args;
  /// Number of call instructions returning integer values
  int64_t call_ret_int;
  /// Number of call instructions returning float values
  int64_t call_ret_float;
};

/// \class ModuleFeatures
///
/// \brief Defines module level features extracted by the plugin
///
/// This holds module level features as they are extracted by the
/// feature extractor. This is later turned into a FeatureSet which can
/// be used with MAGEEC.
///
/// The majority of module features are just aggregations of the function
/// level features.
class ModuleFeatures {
public:
  /// \brief Create an empty set of ModuleFeatures to be populated
  ModuleFeatures() :
    functions(0), sccs(0), fn_ret_int(0), fn_ret_float(0),
    loop_depth(),

    fn_args(), fn_cyclomatic_complexity(), fn_cfg_edges(),
    fn_cfg_abnormal_edges(), fn_critical_path_len(),

    fn_loops(),
    
    fn_basic_blocks(), fn_bb_in_loop(), fn_bb_outside_loop(),

    fn_instructions(), fn_cond_stmts(), fn_direct_calls(), fn_indirect_calls(),
    fn_int_ops(), fn_float_ops(), fn_unary_ops(), fn_ptr_arith_ops(),
    fn_uncond_brs(), fn_assign_stmts(), fn_switch_stmts(), fn_phi_nodes(),
    fn_phi_header_nodes()
  {}

  // General module features
  int64_t functions;    ///< Number of functions in the module
  int64_t sccs;         ///< Number of SCCs in the module
  int64_t fn_ret_int;   ///< Count of functions in the module returning floats
  int64_t fn_ret_float; ///< Count of functions in the module returning integers

  /// Depth of all of the loops in all functions in the module
  std::vector<int64_t> loop_depth;

  // Function features
  /// Number of arguments to each function in the module
  std::vector<int64_t> fn_args;
  /// Cyclomatic complexity of each function in the module
  std::vector<int64_t> fn_cyclomatic_complexity;
  /// Number of CFG edges in each function in the module
  std::vector<int64_t> fn_cfg_edges;
  /// Number of abnormal CFG edges in each module function
  std::vector<int64_t> fn_cfg_abnormal_edges;
  /// Critical path length of each function in the module
  std::vector<int64_t> fn_critical_path_len;

  /// Number of loops in each function in the module
  std::vector<int64_t> fn_loops;

  /// Number of basic blocks in each function in the module
  std::vector<int64_t> fn_basic_blocks;
  /// Number of basic blocks inside a loop in each function in the module
  std::vector<int64_t> fn_bb_in_loop;
  /// Number of basic blocks outside a loop in each function in the module
  std::vector<int64_t> fn_bb_outside_loop;

  // Instructions counts (per function)
  /// Count of instructions per function in the module
  std::vector<int64_t> fn_instructions;
  /// Count of conditional statements in each function in the module
  std::vector<int64_t> fn_cond_stmts;
  /// Number of direct calls in each function in the module
  std::vector<int64_t> fn_direct_calls;
  /// Number of indirect calls in each function in the module
  std::vector<int64_t> fn_indirect_calls;
  /// Number of integer operations in each function in the module
  std::vector<int64_t> fn_int_ops;
  /// Count of floating point operations in the functions in the module
  std::vector<int64_t> fn_float_ops;
  /// Count of the unary operations per function in the module
  std::vector<int64_t> fn_unary_ops;
  /// Number of pointer arithmetic operations in each function in the module
  std::vector<int64_t> fn_ptr_arith_ops;
  /// Number of unconditional branches in each function in the module
  std::vector<int64_t> fn_uncond_brs;
  /// Number of assign statements per function in the module
  std::vector<int64_t> fn_assign_stmts;
  /// Count of switch statements in each function in the module
  std::vector<int64_t> fn_switch_stmts;
  /// Number of phi nodes in each function in the module
  std::vector<int64_t> fn_phi_nodes;
  /// Count of phi header nodes in each function in the module
  std::vector<int64_t> fn_phi_header_nodes;
};


/// \brief Perform feature extraction on a function
///
/// This performs the extraction of features from the GIMPLE representation
/// of a function. The features will subsequently be added into the database.
///
/// \return A created set of features derived from the current function.
std::unique_ptr<FunctionFeatures> extractFunctionFeatures(void);


/// \brief Extract module level features
///
/// Derive module level features from feature extracted from it's consituent
/// functions. The extracted features will subsequently be added into the
/// database.
///
/// \param func_features  Features for all of the functions that make up
/// this module.
/// \return A created set of features derived from the current module.
std::unique_ptr<ModuleFeatures>
extractModuleFeatures(std::vector<const FunctionFeatures*> func_features);


/// \brief Converts function features into a FeatureSet used by MAGEEC
///
/// \param features  The function features to be converted
/// \return The FeatureSet for the given FunctionFeatures
std::unique_ptr<mageec::FeatureSet>
convertFunctionFeatures(const FunctionFeatures &features);


/// \brief Convert module features into a FeatureSet used by MAGEEC
///
/// \param features  The module features to be converted
/// \return The FeatureSet for the given ModuleFeatures
std::unique_ptr<mageec::FeatureSet>
convertModuleFeatures(const ModuleFeatures &features);


#endif // MAGEEC_GCC_FEATURE_EXTRACT_H
