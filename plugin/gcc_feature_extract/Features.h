/*  MAGEEC GCC Features
    Copyright (C) 2015 Embecosm Limited

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

//===----------------------- MAGEEC GCC Features --------------------------===//
//
// This file defines various enumeration types used by the GCC feature
// extractor.
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_GCC_FEATURES_H
#define MAGEEC_GCC_FEATURES_H


enum {
  // A feature can be either a function or module level feature, and it may
  // be produced by applying a number of different reduction functions to
  // a list of values.
  kFeatureTypeMask      = 0xf000,
  kFeatureReductionMask = 0x0f00,
  kFeatureMask          = 0x00ff,

  kFeatureTypeBit = 12,
  kFeatureReductionBit = 8,
  kFeatureBit = 0,

  // Mark where the function and module features begin
  kFuncFeaturesBegin     = 0x1000,
  kModuleFeaturesBegin   = 0x2000,
};


// Each basic feature may be used to derived multiple values by applying
// various reduction functions.
namespace FeatureReduce {
enum : unsigned {
  kTotal,
  kMin,
  kMax,
  kRange,
  kMean,
  kMedian,
  kMode,
  kStdDev,
  kVariance,
};
}


namespace FunctionFeature {
enum : unsigned {
  // General function features
  kArgCount = kFuncFeaturesBegin,
  kCyclomaticComplexity,
  kCFGEdges,
  kCFGAbnormalEdges,
  kCriticalPathLen,

  kLoops,
  kLoopDepth,
  kLoopDepth1,
  kLoopDepth2,
  kLoopDepthGt2,

  kBasicBlocks,
  kBBInLoop,
  kBBOutsideLoop,

  // Basic block counts
  kBBSucc,
  kBBPred,
  kBB1Pred,
  kBB2Pred,
  kBBGt2Pred,
  kBB1Succ,
  kBB2Succ,
  kBBGt2Succ,
  kBB1Pred1Succ,
  kBB1Pred2Succ,
  kBB2Pred1Succ,
  kBB2Pred2Succ,
  kBBGt2PredGt2Succ,

  kBBPhi0,
  kBBPhi0to3,
  kBBPhiGt3,

  kBBInsnLt15,
  kBBInsn15To500,
  KBBInsnGt500,

  // Instructions counts (per basic block)
  kBBInstructions,
  kBBCondStmts,
  kBBDirectCalls,
  kBBIndirectCalls,
  kBBIntOps,
  kBBFloatOps,
  kBBUnaryOps,
  kBBPtrArithOps,
  kBBUncondBrs,
  kBBAssignStmts,
  kBBSwitchStmts,
  kBBPhiNodes,
  kBBPhiHeaderNodes,

  // Function instruction counts
  kPhiArgs,
  kPhiArgs1to5,
  kPhiArgsGt5,
  kCallArgs,
  kCallArgs0,
  kCallArgs1to3,
  kCallArgsGt3,
  kCallPtrArgs,
  kCallRetVoid,
  kCallRetInt,
  kCallRetFloat,
};
} // end of namespace FuncFeatureID


namespace ModuleFeature {
enum : unsigned {
  // General module features
  kFunctions = kModuleFeaturesBegin,
  kSCCs,
  kFuncRetInt,
  kFuncRetFloat,
  kFuncRetVoid,
  kFunc1Arg,
  kFunc1to3Args,
  kFuncGt3Args,

  // Loop features
  kLoopDepth,
  kLoopDepth1,
  kLoopDepth2,
  kLoopDepthGt2,

  // Function features
  kFuncArgs,
  kFuncCyclomaticComplexity,
  kFuncCFGEdges,
  kFuncCFGAbnormalEdges,
  kFuncCriticalPathLen,

  kFuncLoops,

  kFuncBasicBlocks,
  kFuncBBInLoop,
  kFuncBBOutsideLoop,

  // Instruction counts (per function)
  kFuncInsnCount,
  kFuncCondStmts,
  kFuncDirectCalls,
  kFuncIndirectCalls,
  kFuncIntOps,
  kFuncFloatOps,
  kFuncUnaryOps,
  kFuncPtrArithOps,
  kFuncUncondBrs,
  kFuncAssignStmts,
  kFuncSwitchStmts,
  kFuncPhiNodes,
  kFuncPhiHeaderNodes,
};
} // end of namespace ModuleFeatureID


#endif // MAGEEC_GCC_FEATURES_H
