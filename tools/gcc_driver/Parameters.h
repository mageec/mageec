/*  MAGEEC GCC Parameters
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

//===----------------------- MAGEEC GCC Parameters ------------------------===//
//
// This file defines various enumeration types used by the GCC driver
//
//===----------------------------------------------------------------------===//

#ifndef MAGEEC_GCC_PARAMETERS_H
#define MAGEEC_GCC_PARAMETERS_H


enum {
  kFlagParameterBegin = 0,
};

namespace FlagParameterID {
enum : unsigned {
  kFIRST_FLAG_PARAMETER        = kFlagParameterBegin,
  //                                 // -faggressive-loop-optimizations
  //kAggressiveLoopOptimizations = kFIRST_FLAG_PARAMETER,
                                   // -falign-functions
  kAlignFunctions = kFIRST_FLAG_PARAMETER,
  kAlignJumps,                     // -falign-jumps
  kAlignLabels,                    // -falign-labels
  kAlignLoops,                     // -falign-loops
  kBranchCountReg,                 // -fbranch-count-reg
  kBranchTargetLoadOptimize,       // -fbranch-target-load-optimize
  kBranchTargetLoadOptimize2,      // -fbranch-target-load-optimize2
  kBTRBBExclusive,                 // -fbtr-bb-exclusive
  kCallerSaves,                    // -fcaller-saves
  kCombineStackAdjustments,        // -fcombine-stack-adjustments
  kCompareElim,                    // -fcompare-elim
  kConserveStack,                  // -fconserve-stack
  kCPropRegister,                  // -fcprop-registers
  kCrossJumping,                   // -fcrossjumping
  kCSEFollowJumps,                 // -fcse-follow-jumps
  kDCE,                            // -fdce
  kDeferPop,                       // -fdefer-pop
  kDeleteNullPointerChecks,        // -fdelete-null-pointer-checks
  //kDevirtualize,                   // -fdevirtualize
  kDSE,                            // -fdse
  kEarlyInlining,                  // -fearly-inlining
  kExpensiveOptimizations,         // -fexpensive-optimizations
  kForwardPropagate,               // -fforward-propagate
  kGCSE,                           // -fgcse
  kGCSEAfterReload,                // -fgcse-after-reload
  kGCSELAS,                        // -fgcse-las
  kGCSELM,                         // -fgcse-lm
  kGCSESM,                         // -fgcse-sm
  kGuessBranchProbability,         // -fguess-branch-probability
  //kHoistAdjacentLoads,             // -fhoist-adjacent-loads
  kIfConversion,                   // -fif-conversion
  kIfConversion2,                  // -fif-conversion2
  kInline,                         // -finline
  kInlineAtomics,                  // -finline-atomics
  kInlineFunctions,                // -finline-functions
  kInlineFunctionsCalledOnce,      // -finline-functions-called-once
  kInlineSmallFunctions,           // -finline-small-functions
  kIPACP,                          // -fipa-cp
  kIPACPClone,                     // -fipa-cp-clone
  kIPAProfile,                     // -fipa-profile
  kIPAPTA,                         // -fipa-pta
  kIPAPureConst,                   // -fipa-pure-const
  kIPAReference,                   // -fipa-reference
  kIPASRA,                         // -fipa-sra
  kIRAHoistPressure,               // -fira-hoist-pressure
  kIVOpts,                         // -fivopts
  kMergeConstants,                 // -fmerge-constants
  kModuloSched,                    // -fmodulo-sched
  kMoveLoopInvariants,             // -fmove-loop-invariants
  kOmitFramePointer,               // -fomit-frame-pointer
  kOptimizeSiblingCalls,           // -foptimize-sibling-calls
  //kOptimizeStrLen,                 // -foptimize-strlen
  kPeephole,                       // -fpeephole
  kPeephole2,                      // -fpeephole2
  kPredictiveCommoning,            // -fpredictive-commoning
  kPrefetchLoopArrays,             // -fprefetch-loop-arrays
  kRegMove,                        // -fregmove
  kRenameRegisters,                // -frename-registers
  kReorderBlocks,                  // -freorder-blocks
  kReorderFunctions,               // -freorder-functions
  kRerunCSEAfterLoop,              // -frerun-cse-after-loop
  kRescheduleModuloScheduledLoops, // -freschedule-modulo-scheduled-loops
  kSchedCriticalPathHeuristic,     // -fsched-critical-path-heuristic
  kSchedDepCountHeuristic,         // -fsched-dep-count-heuristic
  kSchedGroupHeuristic,            // -fsched-group-heuristic
  kSchedInterblock,                // -fsched-interblock
  kSchedLastInsnHeuristic,         // -fsched-last-insn-heuristic
  kSchedPressure,                  // -fsched-pressure
  kSchedRankHeuristic,             // -fsched-rank-heuristic
  kSchedSpec,                      // -fsched-spec
  kSchedSpecInsnHeuristic,         // -fsched-spec-insn-heuristic
  kSchedSpecLoad,                  // -fsched-spec-load
  kSchedStalledInsns,              // -fsched-stalled-insns
  kSchedStalledInsnsDep,           // -fsched-stalled-insns-dep
  kScheduleInsns,                  // -fschedule-insns
  kScheduleInsns2,                 // -fschedule-insns2
  //kSectionAnchors,                 // -fsection-anchors
  kSelSchedPipelining,             // -fsel-sched-pipelining
  kSelSchedPipeliningOuterLoops,   // -fsel-sched-pipelining-outer-loops
  kSelSchedReschedulePipelined,    // -fsel-sched-reschedule-pipelined
  kSelectiveScheduling,            // -fselective-scheduling
  kSelectiveScheduling2,           // -fselective-scheduling2
  kShrinkWrap,                     // -fshrink-wrap
  kSplitIVsInUnroller,             // -fsplit-ivs-in-unroller
  kSplitWideTypes,                 // -fsplit-wide-types
  kThreadJumps,                    // -fthread-jumps
  kTopLevelReorder,                // -ftoplevel-reorder
  kTreeBitCCP,                     // -ftree-bit-ccp
  kTreeBuiltinCallDCE,             // -ftree-builtin-call-dce
  kTreeCCP,                        // -ftree-ccp
  kTreeCH,                         // -ftree-ch
  //kTreeCoalesceInlinedVars,        // -ftree-coalesce-inlined-vars
  kTreeCoalesceVars,               // -ftree-coalesce-vars
  kTreeCopyProp,                   // -ftree-copy-prop
  kTreeCopyRename,                 // -ftree-copyrename
  kTreeCSEElim,                    // -ftree-cselim
  kTreeDCE,                        // -ftree-dce
  kTreeDominatorOpts,              // -ftree-dominator-opts
  kTreeDSE,                        // -ftree-dse
  kTreeForwProp,                   // -ftree-forwprop
  kTreeFRE,                        // -ftree-fre
  //kTreeLoopDistributePatterns,     // -ftree-loop-distribute-patterns
  kTreeLoopDistribution,           // -ftree-loop-distribution
  kTreeLoopIfConvert,              // -ftree-loop-if-convert
  kTreeLoopIM,                     // -ftree-loop-im
  kTreeLoopIVCanon,                // -ftree-loop-ivcanon
  kTreeLoopOptimize,               // -ftree-loop-optimize
  //kTreePartialPre,                 // -ftree-partial-pre
  kTreePhiProp,                    // -ftree-phiprop
  kTreePre,                        // -ftree-pre
  kTreePTA,                        // -ftree-pta
  kTreeReassoc,                    // -ftree-reassoc
  kTreeSCEVCProp,                  // -ftree-scev-cprop
  kTreeSink,                       // -ftree-sink
  kTreeSLPVectorize,               // -ftree-slp-vectorize
  kTreeSLSR,                       // -ftree-slsr
  kTreeSRA,                        // -ftree-sra
  kTreeSwitchConversion,           // -ftree-switch-conversion
  //kTreeTailMerge,                  // -ftree-tail-merge
  kTreeTER,                        // -ftree-ter
  kTreeVectLoopVersion,            // -ftree-vect-loop-version
  kTreeVectorize,                  // -ftree-vectorize
  kTreeVRP,                        // -ftree-vrp
  kUnrollAllLoops,                 // -funroll-all-loops
  kUnrollLoops,                    // -funroll-loops
  kUnswitchLoops,                  // -funswitch-loops
  kVariableExpansionInUnroller,    // -fvariable-expansion-in-unroller
  kVectCostModel,                  // -fvect-cost-model
  kWeb,                            // -fweb

  kLAST_FLAG_PARAMETER = kWeb,
};
} // end of namespace FlagParameterID


#endif // MAGEEC_GCC_PARAMETERS_H
