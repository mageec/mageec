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
  kFuncFeaturesBegin = 0x0000,
  kModuleFeaturesBegin  = 0x1000,
};


namespace FuncFeatureID {
  enum : unsigned {
    kBBCount              = kFuncFeaturesBegin +  1,
    kBBOneSucc            = kFuncFeaturesBegin +  2,
    kBBTwoSucc            = kFuncFeaturesBegin +  3,
    kBBGT2Succ            = kFuncFeaturesBegin +  4,
    kBBOnePred            = kFuncFeaturesBegin +  5,
    kBBTwoPred            = kFuncFeaturesBegin +  6,
    kBBGT2Pred            = kFuncFeaturesBegin +  7,
    kBBOnePredOneSucc     = kFuncFeaturesBegin +  8,
    kBBOnePredTwoSucc     = kFuncFeaturesBegin +  9,
    kBBTwoPredOneSucc     = kFuncFeaturesBegin + 10,
    kBBTwoPredTwoSucc     = kFuncFeaturesBegin + 11,
    kBBGT2PredGT2Succ     = kFuncFeaturesBegin + 12,
    kBBInsnCountLT15      = kFuncFeaturesBegin + 13,
    kBBInsnCount15To500   = kFuncFeaturesBegin + 14,
    kBBInsnCountGT500     = kFuncFeaturesBegin + 15,
    kEdgesInCFG           = kFuncFeaturesBegin + 16,
    kCriticalEdges        = kFuncFeaturesBegin + 17,
    kAbnormalEdges        = kFuncFeaturesBegin + 18,
    kCondStatements       = kFuncFeaturesBegin + 19,
    kDirectCalls          = kFuncFeaturesBegin + 20,
    kAssignments          = kFuncFeaturesBegin + 21,
    kIntOperations        = kFuncFeaturesBegin + 22,
    kFloatOperations      = kFuncFeaturesBegin + 23,
    kBBInsnAverage        = kFuncFeaturesBegin + 24,
    kInsnCount            = kFuncFeaturesBegin + 25,
    kAveragePhiNodeHead   = kFuncFeaturesBegin + 26,
    kAveragePhiArgs       = kFuncFeaturesBegin + 27,
    kBBPhiCount0          = kFuncFeaturesBegin + 28,
    kBBPhi0To3            = kFuncFeaturesBegin + 29,
    kBBPhiGT3             = kFuncFeaturesBegin + 30,
    kBBPhiArgsGT5         = kFuncFeaturesBegin + 31,
    kBBPhiArgs1To5        = kFuncFeaturesBegin + 32,
    kSwitchStatements     = kFuncFeaturesBegin + 33,
    kUnaryOps             = kFuncFeaturesBegin + 34,
    kPointerArith         = kFuncFeaturesBegin + 35,
    kIndirectCalls        = kFuncFeaturesBegin + 39,
    kCallPointerArgs      = kFuncFeaturesBegin + 42,
    kCallGT4Args          = kFuncFeaturesBegin + 43,
    kCallReturnFloat      = kFuncFeaturesBegin + 44,
    kCallRetInt           = kFuncFeaturesBegin + 45,
    kUncondBranches       = kFuncFeaturesBegin + 46,
  };
}


#endif // MAGEEC_GCC_FEATURES_H
