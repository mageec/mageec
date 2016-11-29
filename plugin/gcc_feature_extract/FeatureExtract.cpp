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

#include "FeatureExtract.h"
#include "Features.h"
#include "mageec/AttributeSet.h"

// GCC Plugin headers                                                           
// Undefine these as gcc-plugin.h redefines them                                
#undef PACKAGE_BUGREPORT                                                        
#undef PACKAGE_NAME                                                             
#undef PACKAGE_STRING                                                           
#undef PACKAGE_TARNAME                                                          
#undef PACKAGE_VERSION                                                          

// Version of GCC that the plugin will be used with
#ifndef BUILDING_GCC_VERSION
  #include "bversion.h"
  #define GCC_VERSION BUILDING_GCC_VERSION
#endif

#if (GCC_VERSION >= 4005) && (GCC_VERSION < 4009)
  #include "gcc-plugin.h"
  #include "coretypes.h"
  // For 4.5, 4.6 and 4.7 there is an error in tree.h because a structure has
  // thread_local as a member and this conflicts with c++11
  #include "gimple.h"
#elif (GCC_VERSION >= 4009)
  #include "gcc-plugin.h"
  #include "tree.h"
  #include "basic-block.h"
  #include "tree-ssa-alias.h"
  #include "internal-fn.h"
  #include "tree-eh.h"
  #include "gimple-expr.h"
  #include "is-a.h"
  #include "gimple.h"
  #include "gimple-iterator.h"
#endif

#include <memory>
#include <set>
#include <vector>
#include <algorithm>


//===----------------------------------------------------------------------===//
//
//                        Feature Extraction Routines
//
//===----------------------------------------------------------------------===//


std::unique_ptr<FunctionFeatures> extractFunctionFeatures(void) {
  // Extracted features for this function, to be populated
  auto features = std::unique_ptr<FunctionFeatures>(new FunctionFeatures());

  // TODO: Unimplemented features
  // features->arg_count
  // features->cyclomatic_complexity
  
  // features->loop_count
  // features->loop_depth

  basic_block bb;
  bool in_phi_header = false;

#if (GCC_VERSION >= 4005) && (GCC_VERSION < 4009)
  FOR_EACH_BB(bb)
#elif (GCC_VERSION >= 4009)
  FOR_ALL_BB_FN(bb, cfun)
#endif
  {
    unsigned bb_index = features->basic_blocks;
    features->basic_blocks++;

    // Successor/Predecessor information
    features->bb_succ.push_back(EDGE_COUNT(bb->succs));
    features->bb_pred.push_back(EDGE_COUNT(bb->preds));

    // CFG information
    edge e;
    edge_iterator ei;
    FOR_EACH_EDGE(e, ei, bb->succs) {
      features->cfg_edges++;
      if (EDGE_CRITICAL_P(e))
        features->critical_path_len++;
      if (e->flags & EDGE_ABNORMAL)
        features->cfg_abnormal_edges++;
    }

    // Instruction counts (per basic block)
    features->bb_instructions.push_back(0);
    features->bb_cond_stmts.push_back(0);
    features->bb_direct_calls.push_back(0);
    features->bb_indirect_calls.push_back(0);
    features->bb_int_ops.push_back(0);
    features->bb_float_ops.push_back(0);
    features->bb_unary_ops.push_back(0);
    features->bb_ptr_arith_ops.push_back(0);
    features->bb_uncond_brs.push_back(0);
    features->bb_assign_stmts.push_back(0);
    features->bb_switch_stmts.push_back(0);
    features->bb_phi_nodes.push_back(0);
    features->bb_phi_header_nodes.push_back(0);

    for (auto gsi = gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi)) {
      #if (GCC_VERSION >= 4005) && (GCC_VERSION < 6001)
        gimple stmt = gsi_stmt(gsi);
      #elif (GCC_VERSION >= 6001)
        gimple *stmt = gsi_stmt(gsi);
      #endif

      in_phi_header = true;

      features->bb_instructions[bb_index]++;

      // Assignment analysis
      if (is_gimple_assign(stmt)) {
        features->bb_assign_stmts[bb_index]++;

        enum gimple_rhs_class grhs_class = 
            get_gimple_rhs_class(gimple_expr_code(stmt));

        if (grhs_class == GIMPLE_UNARY_RHS) {
          features->bb_unary_ops[bb_index]++;
        } else if (grhs_class == GIMPLE_BINARY_RHS) {
          tree arg1 = gimple_assign_rhs1(stmt);
          tree arg2 = gimple_assign_rhs2(stmt);

          if (FLOAT_TYPE_P(TREE_TYPE(arg1)))
            features->bb_float_ops[bb_index]++;
          else if (INTEGRAL_TYPE_P(TREE_TYPE(arg2)))
            features->bb_int_ops[bb_index]++;

          // FIXME: Is this correct for detecting pointer arith
          if (POINTER_TYPE_P(TREE_TYPE(arg1)) ||
              POINTER_TYPE_P(TREE_TYPE(arg2))) {
            features->bb_ptr_arith_ops[bb_index]++;
          }
        }
      }

      // Phi Analysis
      if (gimple_code(stmt) == GIMPLE_PHI) {
        features->bb_phi_nodes[bb_index]++;
        if (in_phi_header)
          features->bb_phi_header_nodes[bb_index]++;

        // count the number of arguments in the phi node
        features->phi_args.push_back(gimple_phi_num_args(stmt));
      } else {
        in_phi_header = false;
      }

      if (gimple_code(stmt) == GIMPLE_SWITCH)
        features->bb_switch_stmts[bb_index]++;

      // Call analysis
      if (is_gimple_call(stmt)) {
        features->call_args.push_back(gimple_call_num_args(stmt));

        // Count the number of pointer arguments
        int64_t ptr_arg_count = 0;
        for (int64_t i = 0; i < gimple_call_num_args(stmt); i++) {
          tree arg = gimple_call_arg(stmt, i);
          if (POINTER_TYPE_P(TREE_TYPE(arg)))
            ptr_arg_count++;
        }
        features->call_ptr_args.push_back(ptr_arg_count);

        // Get current statement, if this is not null, then it's a direct call
        tree call_fn = gimple_call_fndecl(stmt);
        if (call_fn)
          features->bb_direct_calls[bb_index]++;
        else
          features->bb_indirect_calls[bb_index]++;

        tree call_ret = gimple_call_lhs(stmt);
        if (call_ret) {
          if (FLOAT_TYPE_P(TREE_TYPE(call_ret)))
            features->call_ret_float++;
          else if (INTEGRAL_TYPE_P(TREE_TYPE(call_ret)))
            features->call_ret_int++;
        }
      }

      if (gimple_code(stmt) == GIMPLE_COND)
        features->bb_cond_stmts[bb_index]++;
    }
  }
  return features;
}


// Insert a single integer feature value into a mageec feature set
static void insertFeature(mageec::FeatureSet &feature_set,
                          unsigned feature_id, int64_t value,
                          std::string name) {
  feature_set.add(std::make_shared<mageec::IntFeature>(feature_id, value,
                                                       name));
}


// Insert a set of features into the mageec feature set, deriving the
// features values from the provided vector of values, as well as a set of
// reduction functions to run over those values.
static void insertFeatures(mageec::FeatureSet &feature_set,
                           unsigned feature_id, std::vector<int64_t> values,
                           const char *name, std::set<unsigned> reductions) {
  // Don't insert any data if none was extracted
  if (values.size() == 0)
    return;

  for (auto reduce_op : reductions) {
    switch (reduce_op) {
    case FeatureReduce::kTotal: {
      unsigned total_feature_id = feature_id;
      total_feature_id |=
          kFeatureReductionMask & (reduce_op << kFeatureReductionBit);

      int64_t total = 0;
      for (int64_t val : values)
        total += val;

      insertFeature(feature_set, total_feature_id, total,
                    std::string(name) + " (Total)");
      break;
    }
    case FeatureReduce::kMin: {
      unsigned min_feature_id = feature_id;
      min_feature_id |=
          kFeatureReductionMask & (reduce_op << kFeatureReductionBit);

      int64_t min = values[0];
      for (int64_t val : values)
        min = std::min(min, val);

      insertFeature(feature_set, min_feature_id, min,
                    std::string(name) + " (Min)");
      break;
    }
    case FeatureReduce::kMax: {
      unsigned max_feature_id = feature_id;
      max_feature_id |=
          kFeatureReductionMask & (reduce_op << kFeatureReductionBit);

      int64_t max = values[0];
      for (int64_t val : values)
        max = std::max(max, val);

      insertFeature(feature_set, max_feature_id, max,
                    std::string(name) + " (Max)");
      break;
    }
    case FeatureReduce::kRange: {
      unsigned range_feature_id = feature_id;
      range_feature_id |=
          kFeatureReductionMask & (reduce_op << kFeatureReductionBit);

      int64_t min = values[0];
      int64_t max = values[0];
      for (int64_t val : values) {
        min = std::min(min, val);
        max = std::max(max, val);
      }
      uint64_t range = max - min;

      insertFeature(feature_set, range_feature_id, range,
                    std::string(name) + " (Range)");
      break;
    }
    case FeatureReduce::kMean: {
      unsigned mean_feature_id = feature_id;
      mean_feature_id |=
          kFeatureReductionMask & (reduce_op << kFeatureReductionBit);

      int64_t total = 0;
      for (int64_t val : values)
        total += val;
      int64_t mean = total / values.size();

      insertFeature(feature_set, mean_feature_id, mean,
                    std::string(name) + " (Mean)");
      break;
    }
    case FeatureReduce::kMedian: {
      unsigned median_feature_id = feature_id;
      median_feature_id |=
          kFeatureReductionMask & (reduce_op << kFeatureReductionBit);

      std::sort(values.begin(), values.end());
      int64_t median = values[values.size() / 2];

      insertFeature(feature_set, median_feature_id, median,
                    std::string(name) + " (Median)");
      break;
    }
    case FeatureReduce::kMode:
    case FeatureReduce::kStdDev:
    case FeatureReduce::kVariance:
      assert(0 && "Unimplemented reduction function");
      break;
    }
  }
}


std::unique_ptr<ModuleFeatures>
extractModuleFeatures(std::vector<const FunctionFeatures *> fn_features) {
  // Extracted features for this module, to be populated
  auto features = std::unique_ptr<ModuleFeatures>(new ModuleFeatures());

  // TODO Unimplemented features
  // features->sccs
  
  features->functions = fn_features.size();

  for (const FunctionFeatures *fn : fn_features) {
    // FIXME: Support
    //if (fn->ret_int)
    //  features->functions_ret_int++;
    //if (fn->ret_float)
    //  features->functions_ret_float++;

    // Function features
    features->fn_args.push_back(fn->args);
    features->fn_cyclomatic_complexity.push_back(fn->cyclomatic_complexity);
    features->fn_cfg_edges.push_back(fn->cfg_edges);
    features->fn_cfg_abnormal_edges.push_back(fn->cfg_abnormal_edges);
    features->fn_critical_path_len.push_back(fn->critical_path_len);

    features->fn_loops.push_back(fn->loops);
    for (int64_t depth : fn->loop_depth)
      features->loop_depth.push_back(depth);

    features->fn_basic_blocks.push_back(fn->basic_blocks);
    features->fn_bb_in_loop.push_back(fn->bb_in_loop);
    features->fn_bb_outside_loop.push_back(fn->bb_outside_loop);

    // Sum instruction counts from the basic blocks in each function
    features->fn_instructions.push_back(0);
    for (int64_t count : fn->bb_instructions)
      features->fn_instructions.back() += count;

    features->fn_cond_stmts.push_back(0);
    for (int64_t count : fn->bb_cond_stmts)
      features->fn_cond_stmts.back() += count;

    features->fn_direct_calls.push_back(0);
    for (int64_t count : fn->bb_direct_calls)
      features->fn_direct_calls.back() += count;

    features->fn_indirect_calls.push_back(0);
    for (int64_t count : fn->bb_indirect_calls)
      features->fn_indirect_calls.back() += count;

    features->fn_int_ops.push_back(0);
    for (int64_t count : fn->bb_int_ops)
      features->fn_int_ops.back() += count;

    features->fn_float_ops.push_back(0);
    for (int64_t count : fn->bb_float_ops)
      features->fn_float_ops.back() += count;

    features->fn_unary_ops.push_back(0);
    for (int64_t count : fn->bb_unary_ops)
      features->fn_unary_ops.back() += count;

    features->fn_ptr_arith_ops.push_back(0);
    for (int64_t count : fn->bb_ptr_arith_ops)
      features->fn_ptr_arith_ops.back() += count;

    features->fn_uncond_brs.push_back(0);
    for (int64_t count : fn->bb_uncond_brs)
      features->fn_uncond_brs.back() += count;

    features->fn_assign_stmts.push_back(0);
    for (int64_t count : fn->bb_assign_stmts)
      features->fn_assign_stmts.back() += count;

    features->fn_switch_stmts.push_back(0);
    for (int64_t count : fn->bb_switch_stmts)
      features->fn_switch_stmts.back() += count;

    features->fn_phi_nodes.push_back(0);
    for (int64_t count : fn->bb_phi_nodes)
      features->fn_phi_nodes.back() += count;

    features->fn_phi_header_nodes.push_back(0);
    for (int64_t count : fn->bb_phi_header_nodes)
      features->fn_phi_header_nodes.back() += count;
  }
  return features;
}


std::unique_ptr<mageec::FeatureSet>
convertFunctionFeatures(const FunctionFeatures &features) {
  using namespace FeatureReduce;

  // Features as they are represented in mageec
  std::unique_ptr<mageec::FeatureSet> feature_set(new mageec::FeatureSet());

  insertFeature(*feature_set, FunctionFeature::kArgCount,
                features.args,
                "Func: Num of arguments");
  // TODO: kCyclomaticComplexity
  //insertFeature(*feature_set, FunctionFeature::kCyclomaticComplexity,
  //              features.cyclomatic_complexity,
  //              "Func: Cyclomatic complexity");
  insertFeature(*feature_set, FunctionFeature::kCFGEdges,
                features.cfg_edges,
                "Func: Control flow graph edges");
  insertFeature(*feature_set, FunctionFeature::kCFGAbnormalEdges,
                features.cfg_abnormal_edges,
                "Func: Number of abnormal control flow graph edges");
  // TODO: kCriticalPathLen
  //insertFeature(*feature_set, FunctionFeature::kCriticalPathLen,
  //              features.critical_path_len,
  //              "Func: Length of the CFG critical path");

  // Loop features
  insertFeature(*feature_set, FunctionFeature::kLoops,
                features.loops,
                "Func: Number of loops");
  insertFeatures(*feature_set, FunctionFeature::kLoopDepth,
                 features.loop_depth,
                 "Func: Depth of loops",
                 {kMin, kMax, kRange, kMean, kMedian});
  unsigned loop_depth_1 = 0;
  unsigned loop_depth_2 = 0;
  unsigned loop_depth_gt2 = 0;
  for (unsigned depth : features.loop_depth) {
    if (depth == 1)
      loop_depth_1++;
    if (depth == 2)
      loop_depth_2++;
    if (depth > 2)
      loop_depth_gt2++;
  }
  insertFeature(*feature_set, FunctionFeature::kLoopDepth1,
                loop_depth_1,
                "Func: Number of loops of depth 1");
  insertFeature(*feature_set, FunctionFeature::kLoopDepth2,
                loop_depth_2,
                "Func: Number of loops of depth 2");
  insertFeature(*feature_set, FunctionFeature::kLoopDepthGt2,
                loop_depth_gt2,
                "Func: Number of loops of depth >2");

  // Basic block counts
  insertFeature(*feature_set, FunctionFeature::kBasicBlocks,
                features.basic_blocks,
                "Func: Number of basic blocks");
  insertFeature(*feature_set, FunctionFeature::kBBInLoop,
                features.bb_in_loop,
                "Func: Number of basic blocks in a loop");
  insertFeature(*feature_set, FunctionFeature::kBBOutsideLoop,
                features.bb_outside_loop,
                "Func: Number of basic blocks outside a loop");

  insertFeatures(*feature_set, FunctionFeature::kBBSucc,
                 features.bb_succ,
                 "Func: Number of successors for a basic block",
                 {kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBPred,
                 features.bb_pred,
                 "Func: Number of predecessors for a basic block",
                 {kMin, kMax, kRange, kMean, kMedian});
  unsigned bb_1pred = 0;
  unsigned bb_2pred = 0;
  unsigned bb_gt2pred = 0;
  unsigned bb_1succ = 0;
  unsigned bb_2succ = 0;
  unsigned bb_gt2succ = 0;
  unsigned bb_1pred_1succ = 0;
  unsigned bb_1pred_2succ = 0;
  unsigned bb_2pred_1succ = 0;
  unsigned bb_2pred_2succ = 0;
  unsigned bb_gt2pred_gt2succ = 0;
  for (unsigned i = 0; i < features.basic_blocks; ++i) {
    if (features.bb_pred[i] == 1) {
      bb_1pred++;
      if (features.bb_succ[i] == 1)
        bb_1pred_1succ++;
      if (features.bb_succ[i] == 2)
        bb_1pred_2succ++;
    }
    if (features.bb_pred[i] == 2) {
      bb_2pred++;
      if (features.bb_succ[i] == 1)
        bb_2pred_1succ++;
      if (features.bb_succ[i] == 2)
        bb_2pred_2succ++;
    }
    if (features.bb_pred[i] > 2) {
      bb_gt2pred++;
      if (features.bb_succ[i] > 2)
        bb_gt2pred_gt2succ++;
    }

    if (features.bb_succ[i] == 1)
      bb_1succ++;
    if (features.bb_succ[i] == 2)
      bb_2succ++;
    if (features.bb_succ[i] > 2)
      bb_gt2succ++;
  }
  insertFeature(*feature_set, FunctionFeature::kBB1Pred, bb_1pred,
                "Func: Number of basic blocks with 1 predecessor");
  insertFeature(*feature_set, FunctionFeature::kBB2Pred, bb_2pred,
                "Func: Number of basic blocks with 2 predecessors");
  insertFeature(*feature_set, FunctionFeature::kBBGt2Pred, bb_gt2pred,
                "Func: Number of basic blocks with >2 predecessors");

  insertFeature(*feature_set, FunctionFeature::kBB1Succ, bb_1succ,
                "Func: Number of basic blocks with 1 successor");
  insertFeature(*feature_set, FunctionFeature::kBB2Succ, bb_2succ,
                "Func: Number of basic blocks with 2 successor");
  insertFeature(*feature_set, FunctionFeature::kBBGt2Succ, bb_gt2succ,
                "Func: Number of basic blocks with >2 successor");

  insertFeature(*feature_set, FunctionFeature::kBB1Pred1Succ, bb_1pred_1succ,
                "Func: Number of basic blocks with 1 predecessor, 1 successor");
  insertFeature(*feature_set, FunctionFeature::kBB1Pred2Succ, bb_1pred_2succ,
                "Func: Number of basic blocks with 1 predecessor, 2 successors");
  insertFeature(*feature_set, FunctionFeature::kBB2Pred1Succ, bb_2pred_1succ,
                "Func: Number of basic blocks with 2 predecessors, 1 successor");
  insertFeature(*feature_set, FunctionFeature::kBB2Pred2Succ, bb_2pred_2succ,
                "Func: Number of basic blocks with 2 predecessors, 2 successors");
  insertFeature(*feature_set, FunctionFeature::kBBGt2PredGt2Succ, bb_gt2pred_gt2succ,
                "Func: Number of basic blocks with >2 predecessors, >2 successors");

  // TODO: kBBPhi0
  // TODO: kBBPhi0to3
  // TODO: kBBPhiGt3

  // TODO: kBBInsnLt15
  // TODO: kBBInsn15To500
  // TODO: kBBInsnGt500

  // Instruction counts (per basic block)
  insertFeatures(*feature_set, FunctionFeature::kBBInstructions,
                 features.bb_instructions,
                 "Func: Number of instructions in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBCondStmts,
                 features.bb_cond_stmts,
                 "Func: Number of conditional statements in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBDirectCalls,
                 features.bb_direct_calls,
                 "Func: Number of direct calls in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBIndirectCalls,
                 features.bb_indirect_calls,
                 "Func: Number of indirect calls in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBIntOps,
                 features.bb_int_ops,
                 "Func: Number of integer operations in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBFloatOps,
                 features.bb_float_ops,
                 "Func: Number of floating-point operations in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBUnaryOps,
                 features.bb_unary_ops,
                 "Func: Number of unary operations in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBPtrArithOps,
                 features.bb_ptr_arith_ops,
                 "Func: Number of pointer arithmetic operations in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBUncondBrs,
                 features.bb_uncond_brs,
                 "Func: Number of unconditional branches in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBAssignStmts,
                 features.bb_assign_stmts,
                 "Func: Number of assignments in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBSwitchStmts,
                 features.bb_switch_stmts,
                 "Func: Number of switches in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBPhiNodes,
                 features.bb_phi_nodes,
                 "Func: Number of phi nodes in basic block",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, FunctionFeature::kBBPhiHeaderNodes,
                 features.bb_phi_header_nodes,
                 "Func: Number of phi header nodes in basic block",
                 {kTotal, kMax, kMean, kMedian});

  // Function instruction counts
  insertFeatures(*feature_set, FunctionFeature::kPhiArgs,
                 features.phi_args,
                 "Func: Number of arguments in phi nodes",
                 {kMax, kMean, kMedian});
  unsigned phi_args_1to5 = 0;
  unsigned phi_args_gt5 = 0;
  for (int64_t n_args : features.phi_args) {
    if (n_args >= 1 && n_args <= 5)
      phi_args_1to5++;
    if (n_args > 5)
      phi_args_gt5++;
  }
  insertFeature(*feature_set, FunctionFeature::kPhiArgs1to5,
                phi_args_1to5,
                "Func: Number of phi nodes with between 1 and 5 arguments");
  insertFeature(*feature_set, FunctionFeature::kPhiArgsGt5,
                phi_args_gt5,
                "Func: Number of phi nodes with >5 arguments");
  
  insertFeatures(*feature_set, FunctionFeature::kCallArgs,
                 features.call_args,
                 "Func: Number of arguments in call instructions",
                 {kMax, kMean, kMedian});
  unsigned call_args_0 = 0;
  unsigned call_args_1to3 = 0;
  unsigned call_args_gt3 = 0;
  for (int64_t n_args : features.call_args) {
    if (n_args == 0)
      call_args_0++;
    if (n_args >= 1 && n_args <= 3)
      call_args_1to3++;
    if (n_args > 3)
      call_args_gt3++;
  }
  insertFeature(*feature_set, FunctionFeature::kCallArgs0,
                call_args_0,
                "Func: Number of call instructions with 0 arguments");
  insertFeature(*feature_set, FunctionFeature::kCallArgs1to3,
                call_args_1to3,
                "Func: Number of call instructions with between 1 and 3 arguments");
  insertFeature(*feature_set, FunctionFeature::kCallArgsGt3,
                call_args_gt3,
                "Func: Number of call instructions with >3 arguments");

  insertFeatures(*feature_set, FunctionFeature::kCallPtrArgs,
                 features.call_ptr_args,
                 "Func: Number of pointer arguments in call instructions",
                 {kMax, kMin, kMedian});

  // TODO: kCallRetVoid
  insertFeature(*feature_set, FunctionFeature::kCallRetInt,
                features.call_ret_int,
                "Func: Number of call instructions returning integers");
  insertFeature(*feature_set, FunctionFeature::kCallRetFloat,
                features.call_ret_float,
                "Func: Number of call instructions returning floats");

  return feature_set;
}


std::unique_ptr<mageec::FeatureSet>
convertModuleFeatures(const ModuleFeatures &features) {
  using namespace FeatureReduce;

  // Features as they are represented in mageec
  std::unique_ptr<mageec::FeatureSet> feature_set(new mageec::FeatureSet());

  // General module features
  insertFeature(*feature_set, ModuleFeature::kFunctions,
                features.functions,
                "Module: Number of functions");
  insertFeature(*feature_set, ModuleFeature::kSCCs,
                features.sccs,
                "Module: Number of SCCs");
  insertFeature(*feature_set, ModuleFeature::kFuncRetInt,
                features.fn_ret_int,
                "Module: Number of functions returning integers");
  insertFeature(*feature_set, ModuleFeature::kFuncRetFloat,
                features.fn_ret_float,
                "Module: Number of functions returning floats");
  // TODO: kFuncRetVoid

  // TODO: kFunc1Arg
  // TODO: kFunc1to3Args
  // TODO: kFuncGt3Args

  // Loops
  insertFeatures(*feature_set, ModuleFeature::kLoopDepth,
                 features.loop_depth,
                 "Module: Depth of loops",
                 {kMin, kMax, kMean, kMedian});
  unsigned loop_depth_1 = 0;
  unsigned loop_depth_2 = 0;
  unsigned loop_depth_gt2 = 0;
  for (int64_t depth : features.loop_depth) {
    if (depth == 1)
      loop_depth_1++;
    if (depth == 2)
      loop_depth_2++;
    if (depth > 2)
      loop_depth_gt2++;
  }
  insertFeature(*feature_set, ModuleFeature::kLoopDepth1,
                loop_depth_1,
                "Module: Number of loops of depth 1");
  insertFeature(*feature_set, ModuleFeature::kLoopDepth2,
                loop_depth_2,
                "Module: Number of loops of depth 2");
  insertFeature(*feature_set, ModuleFeature::kLoopDepthGt2,
                loop_depth_gt2,
                "Module: Number of loops of depth >2");

  // Function features
  insertFeatures(*feature_set, ModuleFeature::kFuncArgs,
                 features.fn_args,
                 "Module: Number of arguments to a function",
                 {kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncCyclomaticComplexity,
                 features.fn_cyclomatic_complexity,
                 "Module: Cyclomatic complexity of a function",
                 {kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncCFGEdges,
                 features.fn_cfg_edges,
                 "Module: CFG edges of a function",
                 {kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncCFGAbnormalEdges,
                 features.fn_cfg_abnormal_edges,
                 "Module: Abnormal CFG edges of a function",
                 {kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncCriticalPathLen,
                 features.fn_critical_path_len,
                 "Module: CFG critical path length of a function",
                 {kMin, kMax, kRange, kMean, kMedian});

  insertFeatures(*feature_set, ModuleFeature::kFuncLoops,
                 features.fn_loops,
                 "Module: Number of loops in a function",
                 {kTotal, kMin, kMax, kRange, kMean, kMedian});

  insertFeatures(*feature_set, ModuleFeature::kFuncBasicBlocks,
                 features.fn_basic_blocks,
                 "Module: Number of basic blocks in a function",
                 {kTotal, kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncBBInLoop,
                 features.fn_bb_in_loop,
                 "Module: Number of basic blocks inside a loop in a function",
                 {kTotal, kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncBBOutsideLoop,
                 features.fn_bb_outside_loop,
                 "Module: Number of basic blocks outside a loop in a function",
                 {kTotal, kMin, kMax, kRange, kMean, kMedian});

  // Instruction features
  insertFeatures(*feature_set, ModuleFeature::kFuncInsnCount,
                 features.fn_instructions,
                 "Module: Number of instructions in a function",
                 {kTotal, kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncCondStmts,
                 features.fn_cond_stmts,
                 "Module: Number of conditional statments in a function",
                 {kTotal, kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncDirectCalls,
                 features.fn_direct_calls,
                 "Module: Number of direct calls in a function",
                 {kTotal, kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncIndirectCalls,
                 features.fn_indirect_calls,
                 "Module: Number of indirect calls in a function",
                 {kTotal, kMin, kMax, kRange, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncIntOps,
                 features.fn_int_ops,
                 "Module: Number of integer operations in function",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncFloatOps,
                 features.fn_float_ops,
                 "Module: Number of floating-point operations in function",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncUnaryOps,
                 features.fn_unary_ops,
                 "Module: Number of unary operations in function",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncPtrArithOps,
                 features.fn_ptr_arith_ops,
                 "Module: Number of pointer arithmetic operations in function",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncUncondBrs,
                 features.fn_uncond_brs,
                 "Module: Number of unconditional branches in a function",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncAssignStmts,
                 features.fn_assign_stmts,
                 "Module: Number of assignments in a function",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncSwitchStmts,
                 features.fn_switch_stmts,
                 "Module: Number of switches in a function",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncPhiNodes,
                 features.fn_phi_nodes,
                 "Module: Number of phi nodes in a function",
                 {kTotal, kMax, kMean, kMedian});
  insertFeatures(*feature_set, ModuleFeature::kFuncPhiHeaderNodes,
                 features.fn_phi_header_nodes,
                 "Module: Number of phi header nodes in a function",
                 {kTotal, kMax, kMean, kMedian});

  return feature_set;
}
