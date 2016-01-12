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

#include "Features.h"
#include "mageec/Feature.h"
#include "mageec/FeatureSet.h"
#include "MAGEECPlugin.h"

// GCC Plugin headers
// Undefine these as gcc-plugin.h redefines them
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION

#ifndef BUILDING_GCC_VERSION
    #include "bversion.h"
    #define GCC_VERSION BUILDING_GCC_VERSION
#endif
#if (BUILDING_GCC_VERSION < 4009)
  #include "gcc-plugin.h"
  #include "tree-pass.h"
  #include "gimple.h"
  #include "function.h"
  #include "toplev.h"
#else
  #include "gcc-plugin.h"
  #include "config.h"
  #include "system.h"
  #include "coretypes.h"
  #include "tm.h"
  #include "tree.h"
  #include "stringpool.h"
  #include "toplev.h"
  #include "basic-block.h"
#if (BUILDING_GCC_VERSION < 5001)
  #include "pointer-set.h"
#endif // (BUILDING_GCC_VERSION < 5001)
  #include "hash-table.h"
  #include "vec.h"
  #include "ggc.h"
  #include "basic-block.h"
  #include "tree-ssa-alias.h"
  #include "internal-fn.h"
  #include "gimple-fold.h"
  #include "tree-eh.h"
  #include "gimple-expr.h"
  #include "is-a.h"
  #include "gimple.h"
  #include "gimple-iterator.h"
  #include "tree.h"
  #include "tree-pass.h"
  #include "intl.h"
  #include "diagnostic.h"
  #include "context.h"
#endif

#include <iostream>
#include <memory>
#include <vector>


using namespace mageec;


/// \brief Gate for whether to run the MAGEEC Feature Extractor.
///
/// \return true to always run
static bool featureExtractGate(void)
{
  return true;
}

/// \brief Execute the MAGEEC feature extractor
///
/// This performs the extraction of features from the GIMPLE representation,
/// and provides these features to the framework.
///
/// \return 0
static unsigned featureExtractExecute(void)
{
  basic_block bb;
  gimple_stmt_iterator gsi;
  edge e;
  edge_iterator ei;

  // Variables for holding feature information
  std::vector<unsigned> insn_counts;
  unsigned bb_count = 0;
  unsigned bb_single_successor = 0;
  unsigned bb_two_successors = 0;
  unsigned bb_gt2_successors = 0;
  unsigned bb_single_predecessor = 0;
  unsigned bb_two_predecessors = 0;
  unsigned bb_gt2_predecessors = 0;
  unsigned bb_1pred_1succ = 0;
  unsigned bb_1pred_2succ = 0;
  unsigned bb_2pred_1succ = 0;
  unsigned bb_2pred_2succ = 0;
  unsigned bb_gt2pred_gt2succ = 0;
  unsigned insn_count_lt15 = 0;
  unsigned insn_count_15_to_500 = 0;
  unsigned insn_count_gt500 = 0;
  unsigned edges_in_cfg = 0;
  unsigned edges_critical = 0;
  unsigned edges_abnormal = 0;
  unsigned method_cond_stmt = 0;
  unsigned call_direct = 0;
  unsigned method_assignments = 0;
  unsigned int_operations = 0;
  unsigned float_operations = 0;
  unsigned average_phi_node_head = 0;
  unsigned average_phi_args = 0;
  unsigned bb_phi_count_0 = 0;
  unsigned bb_phi_count_0_to_3 = 0;
  unsigned bb_phi_count_gt3 = 0;
  unsigned bb_phi_args_gt5 = 0;
  unsigned bb_phi_args_1_to_5 = 0;
  unsigned method_switch_stmt = 0;
  unsigned method_unary_ops = 0;
  unsigned pointer_arith = 0;
  unsigned call_indirect = 0;
  unsigned call_ptr_arg = 0;
  unsigned call_gt4_args = 0;
  unsigned call_ret_float = 0;
  unsigned call_ret_int = 0;
  unsigned uncond_branches = 0;

  // Temporaries
  unsigned stmt_count = 0;
  unsigned phi_nodes = 0;
  unsigned phi_args = 0;
  bool in_phi_header = false;
  unsigned phi_header_nodes = 0;
  unsigned total_phi_args = 0;
  unsigned total_phi_nodes = 0;

#if (GCC_VERSION < 4009)
  FOR_EACH_BB (bb)
#else
  FOR_ALL_BB_FN (bb, cfun)
#endif
  {
    stmt_count = 0;
    phi_nodes = 0;
    phi_args = 0;
    in_phi_header = true;

    // For this block count instructions and types
    bb_count++;
    for (gsi=gsi_start_bb(bb); !gsi_end_p(gsi); gsi_next(&gsi))
    {
      gimple stmt = gsi_stmt (gsi);
      stmt_count++;
      // Assignment analysis
      if (is_gimple_assign (stmt))
      {
        method_assignments++;
        enum gimple_rhs_class grhs_class =
          get_gimple_rhs_class (gimple_expr_code (stmt));
        if (grhs_class == GIMPLE_UNARY_RHS)
          method_unary_ops++;
        if (grhs_class == GIMPLE_BINARY_RHS)
        {
          tree arg1 = gimple_assign_rhs1 (stmt);
          tree arg2 = gimple_assign_rhs2 (stmt);
          if (FLOAT_TYPE_P (TREE_TYPE (arg1)))
            float_operations++;
          else if (INTEGRAL_TYPE_P (TREE_TYPE (arg2)))
            int_operations++;
          //FIXME: Is this correct for detecting pointer arith?
          if (POINTER_TYPE_P (TREE_TYPE (arg1)) ||
              POINTER_TYPE_P (TREE_TYPE (arg2)))
            pointer_arith++;
        }
      }

      // Phi Analysis
      if (gimple_code (stmt) == GIMPLE_PHI)
      {
        phi_nodes++;
        total_phi_nodes++;
        if (in_phi_header)
          phi_header_nodes++;
        phi_args += gimple_phi_num_args (stmt);
        total_phi_args += gimple_phi_num_args (stmt);
      }
      else
        in_phi_header = false;
      if (gimple_code (stmt) == GIMPLE_SWITCH)
        method_switch_stmt++;

      // Call analysis
      if (is_gimple_call (stmt))
      {
        if (gimple_call_num_args (stmt) > 4)
          call_gt4_args++;
        // Check if call has a pointer argument
        bool call_has_ptr = false;
        for (unsigned i = 0; i < gimple_call_num_args (stmt); i++)
        {
          tree arg = gimple_call_arg (stmt, i);
          if (POINTER_TYPE_P (TREE_TYPE (arg)))
            call_has_ptr = true;
        }
        if (call_has_ptr)
          call_ptr_arg++;
        // Get current statement, if not null, then direct
        tree call_fn = gimple_call_fndecl (stmt);
        if (call_fn)
          call_direct++;
        else
          call_indirect++;
        tree call_ret = gimple_call_lhs (stmt);
        if(call_ret)
        {
          if (FLOAT_TYPE_P (TREE_TYPE (call_ret)))
            call_ret_float++;
          else if (INTEGRAL_TYPE_P (TREE_TYPE (call_ret)))
            call_ret_int++;
        }
      }

      if (gimple_code (stmt) == GIMPLE_COND)
        method_cond_stmt++;
    } // end gimple iterator

    // Successor/predecessor information
    if (EDGE_COUNT(bb->succs) == 1)
      bb_single_successor++;
    else if (EDGE_COUNT(bb->succs) == 2)
      bb_two_successors++;
    else if (EDGE_COUNT(bb->succs) > 2)
      bb_gt2_successors++;
    if (EDGE_COUNT(bb->preds) == 1)
      bb_single_predecessor++;
    else if (EDGE_COUNT(bb->preds) == 2)
      bb_two_predecessors++;
    else if (EDGE_COUNT(bb->preds) > 2)
      bb_gt2_predecessors++;
    if ((EDGE_COUNT(bb->preds) == 1) && (EDGE_COUNT(bb->succs) == 1))
      bb_1pred_1succ++;
    if ((EDGE_COUNT(bb->preds) == 1) && (EDGE_COUNT(bb->succs) == 2))
      bb_1pred_2succ++;
    if ((EDGE_COUNT(bb->preds) == 2) && (EDGE_COUNT(bb->succs) == 1))
      bb_2pred_1succ++;
    if ((EDGE_COUNT(bb->preds) == 2) && (EDGE_COUNT(bb->succs) == 2))
      bb_2pred_2succ++;
    if ((EDGE_COUNT(bb->preds) > 2) && (EDGE_COUNT(bb->succs) > 2))
      bb_gt2pred_gt2succ++;

    // CFG information
    FOR_EACH_EDGE (e, ei, bb->succs)
    {
      edges_in_cfg++;
      if (EDGE_CRITICAL_P (e))
        edges_critical++;
      if (e->flags & EDGE_ABNORMAL)
        edges_abnormal++;
    }


    // Store processed data about this block
    insn_counts.push_back(stmt_count);
    if (stmt_count < 15)
      insn_count_lt15++;
    else if (stmt_count > 500)
      insn_count_gt500++;
    else
      insn_count_15_to_500++;

    if (phi_nodes == 0)
      bb_phi_count_0++;
    if (phi_nodes <= 3)
      bb_phi_count_0_to_3++;
    else
      bb_phi_count_gt3++;
    if (phi_args > 5)
      bb_phi_args_gt5++;
    else if ((phi_args >= 1) && (phi_args <= 5))
      bb_phi_args_1_to_5++;
  } /* end of basic block */

  // Calculate averages once totals have been collected
  if (total_phi_nodes > 0)
    average_phi_args = total_phi_args / total_phi_nodes;
  if (bb_count > 0)
    average_phi_node_head = phi_header_nodes / bb_count;

  // Calculate total and average instructions for function
  unsigned func_insn_count = 0;
  for (auto I : insn_counts) {
    func_insn_count += I;
  }
  unsigned bb_insn_average = func_insn_count / bb_count;

  // Build feature set to pass to MAGEEC
  std::unique_ptr<FeatureSet> features(new FeatureSet());
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBCount,            bb_count,              "Func: Basic block count"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBOneSucc,          bb_single_successor ,  "Func: Basic blocks with 1 successor"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBTwoSucc,          bb_two_successors,     "Func: Basic blocks with 2 successors"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBGT2Succ,          bb_gt2_successors,     "Func: Basic blocks with > 2 successors"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBOnePred,          bb_single_predecessor, "Func: Basic blocks with 1 predecessor"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBTwoPred,          bb_two_predecessors,   "Func: Basic blocks with 2 predecessors"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBGT2Pred,          bb_gt2_predecessors,   "Func: Basic blocks with > 2 predecessors"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBOnePredOneSucc,   bb_1pred_1succ,        "Func: Basic blocks with 1 predecessor, 1 successor"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBOnePredTwoSucc,   bb_1pred_2succ,        "Func: Basic blocks with 1 predecessor, 2 successors"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBTwoPredOneSucc,   bb_2pred_1succ,        "Func: Basic blocks with 2 predecessors, 1 successor"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBTwoPredTwoSucc,   bb_2pred_2succ,        "Func: Basic blocks with 2 predecessors, 2 successors"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBGT2PredGT2Succ,   bb_gt2pred_gt2succ,    "Func: Basic blocks with > 2 predecessors, > 2 successors"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBInsnCountLT15,    insn_count_lt15,       "Func: Basic blocks with < 15 instructions"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBInsnCount15To500, insn_count_15_to_500,  "Func: Basic blocks with between 15 and 500 instructions"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBInsnCountGT500,   insn_count_gt500,      "Func: Basic blocks with > 500 instructions"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kEdgesInCFG,         edges_in_cfg,          "Func: Number of edges in the CFG"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kCriticalEdges,      edges_critical,        "Func: Number of critical edges in the CFG"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kAbnormalEdges,      edges_abnormal,        "Func: Number of abnormal edges in the CFG"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kCondStatements,     method_cond_stmt,      "Func: Number of conditional statements"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kDirectCalls,        call_direct,           "Func: Number of direct calls"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kAssignments,        method_assignments,    "Func: Number of assignments"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kIntOperations,      int_operations,        "Func: Number of integer operations"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kFloatOperations,    float_operations,      "Func: Number of floating point operations"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kInsnCount,          func_insn_count,       "Func: Total number of instructions"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBInsnAverage,      bb_insn_average,       "Func: Average number of instructions per basic block"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kAveragePhiNodeHead, average_phi_node_head, "Func: Average number of phi nodes at the top of a basic block"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kAveragePhiArgs,     average_phi_args,      "Func: Average number of phi arguments"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBPhiCount0,        bb_phi_count_0,        "Func: Basic blocks with 0 phi nodes"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBPhi0To3,          bb_phi_count_0_to_3,   "Func: Basic blocks with between 0 and 3 phi nodes"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBPhiGT3,           bb_phi_count_gt3,      "Func: Basic blocks with > 3 phi nodes"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBPhiArgsGT5,       bb_phi_args_gt5,       "Func: Basic blocks with phi nodes with > 5 arguments"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kBBPhiArgs1To5,      bb_phi_args_1_to_5,    "Func: Basic blocks with phi nodes with between 1 and 5 arguments"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kSwitchStatements,   method_switch_stmt,    "Func: Number of switch statements"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kUnaryOps,           method_unary_ops,      "Func: Number of unary operations"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kPointerArith,       pointer_arith,         "Func: Number of pointer arithmetic operations"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kIndirectCalls,      call_indirect,         "Func: Number of indirect calls"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kCallPointerArgs,    call_ptr_arg,          "Func: Number of calls with pointer arguments"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kCallGT4Args,        call_gt4_args,         "Func: Number of calls with > 4 arguments"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kCallReturnFloat,    call_ret_float,        "Func: Number of calls returning a float"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kCallRetInt,         call_ret_int,          "Func: Number of calls returning an int"));
  features->add(std::make_shared<IntFeature>(FuncFeatureID::kUncondBranches,     uncond_branches,       "Func: Number of unconditional branches"));

  std::string func_name = current_function_name();

  assert((mageec_context.func_features.count(func_name) == 0) &&
         "Features for function already extracted");
  mageec_context.func_features[func_name] = std::move(features);

  MAGEEC_DEBUG("Extracting features for function '" << func_name << "'");
  MAGEEC_DEBUG("Dumping feature vector");
  if (util::withDebug()) {
    mageec_context.func_features[func_name]->print(
        mageec::util::dbg(), " |- ");
  }
  MAGEEC_DEBUG("Finished dumping feature vector");
  return 0;
}


//===------------ MAGEEC feature extractor pass definition ----------------===//


#if (GCC_VERSION < 4009)
static struct gimple_opt_pass mageec_feature_extract =
{
  {
    GIMPLE_PASS,
    "mageec-feature-extractor",   // name
    OPTGROUP_NONE,                // optinfo_flags
    featureExtractGate,           // gate
    featureExtractExecute,        // execute
    NULL,                         // sub
    NULL,                         // next
    0,                            // static_pass_number
    TV_NONE,                      // tv_id
    PROP_ssa,                     // properties_required
    0,                            // properties_provided
    0,                            // properties_destroyed
    0,                            // todo_flags_start
    0                             // todo_flags_finish
  }
};
#elif (GCC_VERSION >= 4009)
namespace {
  
const pass_data pass_data_mageec_feature_extract =
{
  GIMPLE_PASS,                    // type
  "mageec-feature-extractor",     // name
  OPTGROUP_NONE,                  // optinfo_flags

#if (GCC_VERSION < 5001)
  true,                           // has_gate
  true,                           // has_execute
#endif // (GCC_VERSION < 5001)

  TV_NONE,                        // tv_id
  PROP_ssa,                       // properties_required
  0,                              // properties_provided
  0,                              // properties_destroyed
  0,                              // todo_flags_start
  0,                              // todo_flags_finish
};


class FeatureExtractPass : public gimple_opt_pass {
public:
  FeatureExtractPass(gcc::context *context)
    : gimple_opt_pass(pass_data_mageec_feature_extract, context)
  {}

  // opt_pass methods
#if (GCC_VERSION < 5001)
  bool gate() override {
    return featureExtractGate();
  }
  unsigned execute() override {
    return featureExtractExecute();
  }
#else
  bool gate(function *) override {
    return featureExtractGate();
  }
  unsigned execute(function *) override {
    return featureExtractExecute();
  }
#endif
}; // class MageecFeatureExtractPass

} // end of anonymous namespace


/// \brief Create a new feature extractor pass
static gimple_opt_pass *makeMAGEECFeatureExtractPass(gcc::context *context)
{
  return new FeatureExtractPass(context);
}
#endif /* (GCC_VERSION < 4009) */


/// \brief Registers the feature extractor pass in the pass list
///
/// Currently runs after the CFG pass
void mageecRegisterFeatureExtract(void)
{
  struct register_pass_info pass;

#if (GCC_VERSION < 4009)
  pass.pass = &mageec_feature_extract.pass;
#else
  pass.pass = makeMAGEECFeatureExtractPass(g);
#endif
  pass.reference_pass_name = "ssa";
  pass.ref_pass_instance_number = 1;
  pass.pos_op = PASS_POS_INSERT_AFTER;

  register_callback (mageec_gcc_plugin_name, PLUGIN_PASS_MANAGER_SETUP, NULL,
                     &pass);
}
