#include "mageec/Attribute.h"
#include "mageec/Database.h"
#include "mageec/Framework.h"
#include "mageec/ML/C5.h"
#include "mageec/Util.h"
#include "Parameters.h"

#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <sstream>
#include <vector>

#if !defined(GCC_DRIVER_VERSION_MAJOR) ||                                      \
    !defined(GCC_DRIVER_VERSION_MINOR) ||                                      \
    !defined(GCC_DRIVER_VERSION_PATCH)
#error "MAGEEC gcc driver version not defined by the build system"
#endif
static_assert(GCC_DRIVER_VERSION_MAJOR == 1 && GCC_DRIVER_VERSION_MINOR == 0 &&
                  GCC_DRIVER_VERSION_PATCH == 0,
              "MAGEEC gcc driver version does not match");
static const mageec::util::Version gcc_driver_version(GCC_DRIVER_VERSION_MAJOR,
                                                      GCC_DRIVER_VERSION_MINOR,
                                                      GCC_DRIVER_VERSION_PATCH);

enum class DriverMode { kNone, kGather, kOptimize };

static const auto &opt_flags_O0 = *new std::vector<std::string>({
    "-faggressive-loop-optimizations",
    "-fasynchronous-unwind-tables",
    "-fauto-inc-dec",
    "-fdce",
    "-fdelete-null-pointer-checks",
    "-fdse",
    "-fearly-inlining",
    "-ffunction-cse",
    "-fgcse-lm",
    "-finline",
    "-finline-atomics",
    "-fira-hoist-pressure",
    "-fira-share-save-slots",
    "-fira-share-spill-slots",
    "-fivopts",
    "-fjump-tables",
    "-flifetime-dse",
    "-fmath-errno",
    "-fpeephole",
    "-fprefetch-loop-arrays",
    "-frename-registers",
    "-frtti",
    "-fsched-critical-path-heuristic",
    "-fsched-dep-count-heuristic",
    "-fsched-group-heuristic",
    "-fsched-interblock",
    "-fsched-last-insn-heuristic",
    "-fsched-rank-heuristic",
    "-fsched-spec",
    "-fsched-spec-insn-heuristic",
    "-fsched-stalled-insns-dep",
    "-fschedule-fusion",
    "-fshort-enums",
    "-fsigned-zeros",
    "-fsplit-ivs-in-unroller",
    "-fstdarg-opt",
    "-fstrict-volatile-bitfields",
    "-fno-threadsafe-statics",
    "-ftrapping-math",
    "-ftree-coalesce-vars",
    "-ftree-cselim",
    "-ftree-forwprop",
    "-ftree-loop-if-convert",
    "-ftree-loop-im",
    "-ftree-loop-ivcanon",
    "-ftree-loop-optimize",
    "-ftree-phiprop",
    "-ftree-reassoc",
    "-ftree-scev-cprop",
    "-fvar-tracking",
    "-fvar-tracking-assignments",
    "-fweb"});

static const auto &opt_flags_O1 = *new std::vector<std::string>({
    "-faggressive-loop-optimizations",
    "-fasynchronous-unwind-tables",
    "-fauto-inc-dec",
    "-fbranch-count-reg",
    "-fcombine-stack-adjustments",
    "-fcompare-elim",
    "-fcprop-registers",
    "-fdce",
    "-fdefer-pop",
    "-fdelete-null-pointer-checks",
    "-fdse",
    "-fearly-inlining",
    "-fforward-propagate",
    "-ffunction-cse",
    "-fgcse-lm",
    "-fguess-branch-probability",
    "-fif-conversion",
    "-fif-conversion2",
    "-finline",
    "-finline-atomics",
    "-finline-functions-called-once",
    "-fipa-profile",
    "-fipa-pure-const",
    "-fipa-reference",
    "-fira-hoist-pressure",
    "-fira-share-save-slots",
    "-fira-share-spill-slots",
    "-fivopts",
    "-fjump-tables",
    "-flifetime-dse",
    "-fmath-errno",
    "-fmove-loop-invariants",
    "-fpeephole",
    "-fprefetch-loop-arrays",
    "-frename-registers",
    "-frtti",
    "-fsched-critical-path-heuristic",
    "-fsched-dep-count-heuristic",
    "-fsched-group-heuristic",
    "-fsched-interblock",
    "-fsched-last-insn-heuristic",
    "-fsched-rank-heuristic",
    "-fsched-spec",
    "-fsched-spec-insn-heuristic",
    "-fsched-stalled-insns-dep",
    "-fschedule-fusion",
    "-fshort-enums",
    "-fshrink-wrap",
    "-fsigned-zeros",
    "-fsplit-ivs-in-unroller",
    "-fsplit-wide-types",
    "-fssa-phiopt",
    "-fstdarg-opt",
    "-fstrict-volatile-bitfields",
    "-fno-threadsafe-statics",
    "-ftrapping-math",
    "-ftree-bit-ccp",
    "-ftree-ccp",
    "-ftree-ch",
    "-ftree-coalesce-vars",
    "-ftree-copy-prop",
    "-ftree-copyrename",
    "-ftree-cselim",
    "-ftree-dce",
    "-ftree-dominator-opts",
    "-ftree-dse",
    "-ftree-forwprop",
    "-ftree-fre",
    "-ftree-loop-if-convert",
    "-ftree-loop-im",
    "-ftree-loop-ivcanon",
    "-ftree-loop-optimize",
    "-ftree-phiprop",
    "-ftree-pta",
    "-ftree-reassoc",
    "-ftree-scev-cprop",
    "-ftree-sink",
    "-ftree-slsr",
    "-ftree-sra",
    "-ftree-ter",
    "-fvar-tracking",
    "-fvar-tracking-assignments",
    "-fweb"});

static const auto &opt_flags_O2 = *new std::vector<std::string>({
    "-faggressive-loop-optimizations",
    "-falign-functions",
    "-falign-jumps",
    "-falign-labels",
    "-falign-loops",
    "-fasynchronous-unwind-tables",
    "-fauto-inc-dec",
    "-fbranch-count-reg",
    "-fcaller-saves",
    "-fcombine-stack-adjustments",
    "-fcompare-elim",
    "-fcprop-registers",
    "-fcrossjumping",
    "-fcse-follow-jumps",
    "-fdce",
    "-fdefer-pop",
    "-fdelete-null-pointer-checks",
    "-fdevirtualize",
    "-fdevirtualize-speculatively",
    "-fdse",
    "-fearly-inlining",
    "-fexpensive-optimizations",
    "-fforward-propagate",
    "-ffunction-cse",
    "-fgcse",
    "-fgcse-lm",
    "-fguess-branch-probability",
    "-fhoist-adjacent-loads",
    "-fif-conversion",
    "-fif-conversion2",
    "-findirect-inlining",
    "-finline",
    "-finline-atomics",
    "-finline-functions-called-once",
    "-finline-small-functions",
    "-fipa-cp",
    "-fipa-cp-alignment",
    "-fipa-icf",
    "-fipa-icf-functions",
    "-fipa-profile",
    "-fipa-pure-const",
    "-fipa-ra",
    "-fipa-reference",
    "-fipa-sra",
    "-fira-hoist-pressure",
    "-fira-share-save-slots",
    "-fira-share-spill-slots",
    "-fisolate-erroneous-paths-dereference",
    "-fivopts",
    "-fjump-tables",
    "-flifetime-dse",
    "-flra-remat",
    "-fmath-errno",
    "-fmove-loop-invariants",
    "-foptimize-sibling-calls",
    "-foptimize-strlen",
    "-fpartial-inlining",
    "-fpeephole",
    "-fpeephole2",
    "-fprefetch-loop-arrays",
    "-frename-registers",
    "-freorder-blocks",
    "-freorder-blocks-and-partition",
    "-freorder-functions",
    "-frerun-cse-after-loop",
    "-frtti",
    "-fsched-critical-path-heuristic",
    "-fsched-dep-count-heuristic",
    "-fsched-group-heuristic",
    "-fsched-interblock",
    "-fsched-last-insn-heuristic",
    "-fsched-rank-heuristic",
    "-fsched-spec",
    "-fsched-spec-insn-heuristic",
    "-fsched-stalled-insns-dep",
    "-fschedule-fusion",
    "-fschedule-insns2",
    "-fshort-enums",
    "-fshrink-wrap",
    "-fsigned-zeros",
    "-fsplit-ivs-in-unroller",
    "-fsplit-wide-types",
    "-fssa-phiopt",
    "-fstdarg-opt",
    "-fstrict-aliasing",
    "-fstrict-overflow",
    "-fstrict-volatile-bitfields",
    "-fthread-jumps",
    "-fno-threadsafe-statics",
    "-ftrapping-math",
    "-ftree-bit-ccp",
    "-ftree-builtin-call-dce",
    "-ftree-ccp",
    "-ftree-ch",
    "-ftree-coalesce-vars",
    "-ftree-copy-prop",
    "-ftree-copyrename",
    "-ftree-cselim",
    "-ftree-dce",
    "-ftree-dominator-opts",
    "-ftree-dse",
    "-ftree-forwprop",
    "-ftree-fre",
    "-ftree-loop-if-convert",
    "-ftree-loop-im",
    "-ftree-loop-ivcanon",
    "-ftree-loop-optimize",
    "-ftree-phiprop",
    "-ftree-pre",
    "-ftree-pta",
    "-ftree-reassoc",
    "-ftree-scev-cprop",
    "-ftree-sink",
    "-ftree-slsr",
    "-ftree-sra",
    "-ftree-switch-conversion",
    "-ftree-tail-merge",
    "-ftree-ter",
    "-ftree-vrp",
    "-fvar-tracking",
    "-fvar-tracking-assignments",
    "-fweb"});

static const auto &opt_flags_O3 = *new std::vector<std::string>({
    "-faggressive-loop-optimizations",
    "-falign-functions",
    "-falign-jumps",
    "-falign-labels",
    "-falign-loops",
    "-fasynchronous-unwind-tables",
    "-fauto-inc-dec",
    "-fbranch-count-reg",
    "-fcaller-saves",
    "-fcombine-stack-adjustments",
    "-fcompare-elim",
    "-fcprop-registers",
    "-fcrossjumping",
    "-fcse-follow-jumps",
    "-fdce",
    "-fdefer-pop",
    "-fdelete-null-pointer-checks",
    "-fdevirtualize",
    "-fdevirtualize-speculatively",
    "-fdse",
    "-fearly-inlining",
    "-fexpensive-optimizations",
    "-fforward-propagate",
    "-ffunction-cse",
    "-fgcse",
    "-fgcse-after-reload",
    "-fgcse-lm",
    "-fguess-branch-probability",
    "-fhoist-adjacent-loads",
    "-fif-conversion",
    "-fif-conversion2",
    "-findirect-inlining",
    "-finline",
    "-finline-atomics",
    "-finline-functions",
    "-finline-functions-called-once",
    "-finline-small-functions",
    "-fipa-cp",
    "-fipa-cp-alignment",
    "-fipa-cp-clone",
    "-fipa-icf",
    "-fipa-icf-functions",
    "-fipa-profile",
    "-fipa-pure-const",
    "-fipa-ra",
    "-fipa-reference",
    "-fipa-sra",
    "-fira-hoist-pressure",
    "-fira-share-save-slots",
    "-fira-share-spill-slots",
    "-fisolate-erroneous-paths-dereference",
    "-fivopts",
    "-fjump-tables",
    "-flifetime-dse",
    "-flra-remat",
    "-fmath-errno",
    "-fmove-loop-invariants",
    "-foptimize-sibling-calls",
    "-foptimize-strlen",
    "-fpartial-inlining",
    "-fpeephole",
    "-fpeephole2",
    "-fpredictive-commoning",
    "-fprefetch-loop-arrays",
    "-frename-registers",
    "-freorder-blocks",
    "-freorder-blocks-and-partition",
    "-freorder-functions",
    "-frerun-cse-after-loop",
    "-frtti",
    "-fsched-critical-path-heuristic",
    "-fsched-dep-count-heuristic",
    "-fsched-group-heuristic",
    "-fsched-interblock",
    "-fsched-last-insn-heuristic",
    "-fsched-rank-heuristic",
    "-fsched-spec",
    "-fsched-spec-insn-heuristic",
    "-fsched-stalled-insns-dep",
    "-fschedule-fusion",
    "-fschedule-insns2",
    "-fshort-enums",
    "-fshrink-wrap",
    "-fsigned-zeros",
    "-fsplit-ivs-in-unroller",
    "-fsplit-wide-types",
    "-fssa-phiopt",
    "-fstdarg-opt",
    "-fstrict-aliasing",
    "-fstrict-overflow",
    "-fstrict-volatile-bitfields",
    "-fthread-jumps",
    "-fno-threadsafe-statics",
    "-ftrapping-math",
    "-ftree-bit-ccp",
    "-ftree-builtin-call-dce",
    "-ftree-ccp",
    "-ftree-ch",
    "-ftree-coalesce-vars",
    "-ftree-copy-prop",
    "-ftree-copyrename",
    "-ftree-cselim",
    "-ftree-dce",
    "-ftree-dominator-opts",
    "-ftree-dse",
    "-ftree-forwprop",
    "-ftree-fre",
    "-ftree-loop-distribute-patterns",
    "-ftree-loop-if-convert",
    "-ftree-loop-im",
    "-ftree-loop-ivcanon",
    "-ftree-loop-optimize",
    "-ftree-loop-vectorize",
    "-ftree-partial-pre",
    "-ftree-phiprop",
    "-ftree-pre",
    "-ftree-pta",
    "-ftree-reassoc",
    "-ftree-scev-cprop",
    "-ftree-sink",
    "-ftree-slp-vectorize",
    "-ftree-slsr",
    "-ftree-sra",
    "-ftree-switch-conversion",
    "-ftree-tail-merge",
    "-ftree-ter",
    "-ftree-vrp",
    "-funswitch-loops",
    "-fvar-tracking",
    "-fvar-tracking-assignments",
    "-fweb"});

static const auto &opt_flags_O4 = *new std::vector<std::string>({
    "-faggressive-loop-optimizations",
    "-falign-functions",
    "-falign-jumps",
    "-falign-labels",
    "-falign-loops",
    "-fasynchronous-unwind-tables",
    "-fauto-inc-dec",
    "-fbranch-count-reg",
    "-fcaller-saves",
    "-fcombine-stack-adjustments",
    "-fcompare-elim",
    "-fcprop-registers",
    "-fcrossjumping",
    "-fcse-follow-jumps",
    "-fdce",
    "-fdefer-pop",
    "-fdelete-null-pointer-checks",
    "-fdevirtualize",
    "-fdevirtualize-speculatively",
    "-fdse",
    "-fearly-inlining",
    "-fexpensive-optimizations",
    "-fforward-propagate",
    "-ffunction-cse",
    "-fgcse",
    "-fgcse-after-reload",
    "-fgcse-lm",
    "-fguess-branch-probability",
    "-fhoist-adjacent-loads",
    "-fif-conversion",
    "-fif-conversion2",
    "-findirect-inlining",
    "-finline",
    "-finline-atomics",
    "-finline-functions",
    "-finline-functions-called-once",
    "-finline-small-functions",
    "-fipa-cp",
    "-fipa-cp-alignment",
    "-fipa-cp-clone",
    "-fipa-icf",
    "-fipa-icf-functions",
    "-fipa-profile",
    "-fipa-pure-const",
    "-fipa-ra",
    "-fipa-reference",
    "-fipa-sra",
    "-fira-hoist-pressure",
    "-fira-share-save-slots",
    "-fira-share-spill-slots",
    "-fisolate-erroneous-paths-dereference",
    "-fivopts",
    "-fjump-tables",
    "-flifetime-dse",
    "-flra-remat",
    "-fmath-errno",
    "-fmove-loop-invariants",
    "-foptimize-sibling-calls",
    "-foptimize-strlen",
    "-fpartial-inlining",
    "-fpeephole",
    "-fpeephole2",
    "-fpredictive-commoning",
    "-fprefetch-loop-arrays",
    "-frename-registers",
    "-freorder-blocks",
    "-freorder-blocks-and-partition",
    "-freorder-functions",
    "-frerun-cse-after-loop",
    "-frtti",
    "-fsched-critical-path-heuristic",
    "-fsched-dep-count-heuristic",
    "-fsched-group-heuristic",
    "-fsched-interblock",
    "-fsched-last-insn-heuristic",
    "-fsched-rank-heuristic",
    "-fsched-spec",
    "-fsched-spec-insn-heuristic",
    "-fsched-stalled-insns-dep",
    "-fschedule-fusion",
    "-fschedule-insns2",
    "-fshort-enums",
    "-fshrink-wrap",
    "-fsigned-zeros",
    "-fsplit-ivs-in-unroller",
    "-fsplit-wide-types",
    "-fssa-phiopt",
    "-fstdarg-opt",
    "-fstrict-aliasing",
    "-fstrict-overflow",
    "-fstrict-volatile-bitfields",
    "-fthread-jumps",
    "-fno-threadsafe-statics",
    "-ftrapping-math",
    "-ftree-bit-ccp",
    "-ftree-builtin-call-dce",
    "-ftree-ccp",
    "-ftree-ch",
    "-ftree-coalesce-vars",
    "-ftree-copy-prop",
    "-ftree-copyrename",
    "-ftree-cselim",
    "-ftree-dce",
    "-ftree-dominator-opts",
    "-ftree-dse",
    "-ftree-forwprop",
    "-ftree-fre",
    "-ftree-loop-distribute-patterns",
    "-ftree-loop-if-convert",
    "-ftree-loop-im",
    "-ftree-loop-ivcanon",
    "-ftree-loop-optimize",
    "-ftree-loop-vectorize",
    "-ftree-partial-pre",
    "-ftree-phiprop",
    "-ftree-pre",
    "-ftree-pta",
    "-ftree-reassoc",
    "-ftree-scev-cprop",
    "-ftree-sink",
    "-ftree-slp-vectorize",
    "-ftree-slsr",
    "-ftree-sra",
    "-ftree-switch-conversion",
    "-ftree-tail-merge",
    "-ftree-ter",
    "-ftree-vrp",
    "-funswitch-loops",
    "-fvar-tracking",
    "-fvar-tracking-assignments",
    "-fweb"});

static const auto &opt_flags_Os = *new std::vector<std::string>({
    "-faggressive-loop-optimizations",
    "-falign-functions",
    "-falign-jumps",
    "-falign-labels",
    "-falign-loops",
    "-fasynchronous-unwind-tables",
    "-fauto-inc-dec",
    "-fbranch-count-reg",
    "-fcaller-saves",
    "-fcombine-stack-adjustments",
    "-fcompare-elim",
    "-fcprop-registers",
    "-fcrossjumping",
    "-fcse-follow-jumps",
    "-fdce",
    "-fdefer-pop",
    "-fdelete-null-pointer-checks",
    "-fdevirtualize",
    "-fdevirtualize-speculatively",
    "-fdse",
    "-fearly-inlining",
    "-fexpensive-optimizations",
    "-fforward-propagate",
    "-ffunction-cse",
    "-fgcse",
    "-fgcse-lm",
    "-fguess-branch-probability",
    "-fhoist-adjacent-loads",
    "-fif-conversion",
    "-fif-conversion2",
    "-findirect-inlining",
    "-finline",
    "-finline-atomics",
    "-finline-functions",
    "-finline-functions-called-once",
    "-finline-small-functions",
    "-fipa-cp",
    "-fipa-cp-alignment",
    "-fipa-icf",
    "-fipa-icf-functions",
    "-fipa-profile",
    "-fipa-pure-const",
    "-fipa-ra",
    "-fipa-reference",
    "-fipa-sra",
    "-fira-hoist-pressure",
    "-fira-share-save-slots",
    "-fira-share-spill-slots",
    "-fisolate-erroneous-paths-dereference",
    "-fivopts",
    "-fjump-tables",
    "-flifetime-dse",
    "-flra-remat",
    "-fmath-errno",
    "-fmove-loop-invariants",
    "-foptimize-sibling-calls",
    "-fpartial-inlining",
    "-fpeephole",
    "-fpeephole2",
    "-fprefetch-loop-arrays",
    "-frename-registers",
    "-freorder-blocks",
    "-freorder-blocks-and-partition",
    "-freorder-functions",
    "-frerun-cse-after-loop",
    "-frtti",
    "-fsched-critical-path-heuristic",
    "-fsched-dep-count-heuristic",
    "-fsched-group-heuristic",
    "-fsched-interblock",
    "-fsched-last-insn-heuristic",
    "-fsched-rank-heuristic",
    "-fsched-spec",
    "-fsched-spec-insn-heuristic",
    "-fsched-stalled-insns-dep",
    "-fschedule-fusion",
    "-fschedule-insns2",
    "-fshort-enums",
    "-fshrink-wrap",
    "-fsigned-zeros",
    "-fsplit-ivs-in-unroller",
    "-fsplit-wide-types",
    "-fssa-phiopt",
    "-fstdarg-opt",
    "-fstrict-aliasing",
    "-fstrict-overflow",
    "-fstrict-volatile-bitfields",
    "-fthread-jumps",
    "-fno-threadsafe-statics",
    "-ftrapping-math",
    "-ftree-bit-ccp",
    "-ftree-builtin-call-dce",
    "-ftree-ccp",
    "-ftree-ch",
    "-ftree-coalesce-vars",
    "-ftree-copy-prop",
    "-ftree-copyrename",
    "-ftree-cselim",
    "-ftree-dce",
    "-ftree-dominator-opts",
    "-ftree-dse",
    "-ftree-forwprop",
    "-ftree-fre",
    "-ftree-loop-if-convert",
    "-ftree-loop-im",
    "-ftree-loop-ivcanon",
    "-ftree-loop-optimize",
    "-ftree-phiprop",
    "-ftree-pre",
    "-ftree-pta",
    "-ftree-reassoc",
    "-ftree-scev-cprop",
    "-ftree-sink",
    "-ftree-slsr",
    "-ftree-sra",
    "-ftree-switch-conversion",
    "-ftree-tail-merge",
    "-ftree-ter",
    "-ftree-vrp",
    "-fvar-tracking",
    "-fvar-tracking-assignments",
    "-fweb"});

static const auto &opt_flags_Ofast = *new std::vector<std::string>({
    "-faggressive-loop-optimizations",
    "-falign-functions",
    "-falign-jumps",
    "-falign-labels",
    "-falign-loops",
    "-fassociative-math",
    "-fasynchronous-unwind-tables",
    "-fauto-inc-dec",
    "-fbranch-count-reg",
    "-fcaller-saves",
    "-fcombine-stack-adjustments",
    "-fcompare-elim",
    "-fcprop-registers",
    "-fcrossjumping",
    "-fcse-follow-jumps",
    "-fcx-limited-range",
    "-fdce",
    "-fdefer-pop",
    "-fdelete-null-pointer-checks",
    "-fdevirtualize",
    "-fdevirtualize-speculatively",
    "-fdse",
    "-fearly-inlining",
    "-fexpensive-optimizations",
    "-ffinite-math-only",
    "-fforward-propagate",
    "-ffunction-cse",
    "-fgcse",
    "-fgcse-after-reload",
    "-fgcse-lm",
    "-fguess-branch-probability",
    "-fhoist-adjacent-loads",
    "-fif-conversion",
    "-fif-conversion2",
    "-findirect-inlining",
    "-finline",
    "-finline-atomics",
    "-finline-functions",
    "-finline-functions-called-once",
    "-finline-small-functions",
    "-fipa-cp",
    "-fipa-cp-alignment",
    "-fipa-cp-clone",
    "-fipa-icf",
    "-fipa-icf-functions",
    "-fipa-profile",
    "-fipa-pure-const",
    "-fipa-ra",
    "-fipa-reference",
    "-fipa-sra",
    "-fira-hoist-pressure",
    "-fira-share-save-slots",
    "-fira-share-spill-slots",
    "-fisolate-erroneous-paths-dereference",
    "-fivopts",
    "-fjump-tables",
    "-flifetime-dse",
    "-flra-remat",
    "-fmove-loop-invariants",
    "-foptimize-sibling-calls",
    "-foptimize-strlen",
    "-fpartial-inlining",
    "-fpeephole",
    "-fpeephole2",
    "-fpredictive-commoning",
    "-fprefetch-loop-arrays",
    "-freciprocal-math",
    "-frename-registers",
    "-freorder-blocks",
    "-freorder-blocks-and-partition",
    "-freorder-functions",
    "-frerun-cse-after-loop",
    "-frtti",
    "-fsched-critical-path-heuristic",
    "-fsched-dep-count-heuristic",
    "-fsched-group-heuristic",
    "-fsched-interblock",
    "-fsched-last-insn-heuristic",
    "-fsched-rank-heuristic",
    "-fsched-spec",
    "-fsched-spec-insn-heuristic",
    "-fsched-stalled-insns-dep",
    "-fschedule-fusion",
    "-fschedule-insns2",
    "-fshort-enums",
    "-fshrink-wrap",
    "-fsplit-ivs-in-unroller",
    "-fsplit-wide-types",
    "-fssa-phiopt",
    "-fstdarg-opt",
    "-fstrict-aliasing",
    "-fstrict-overflow",
    "-fstrict-volatile-bitfields",
    "-fthread-jumps",
    "-fno-threadsafe-statics",
    "-ftree-bit-ccp",
    "-ftree-builtin-call-dce",
    "-ftree-ccp",
    "-ftree-ch",
    "-ftree-coalesce-vars",
    "-ftree-copy-prop",
    "-ftree-copyrename",
    "-ftree-cselim",
    "-ftree-dce",
    "-ftree-dominator-opts",
    "-ftree-dse",
    "-ftree-forwprop",
    "-ftree-fre",
    "-ftree-loop-distribute-patterns",
    "-ftree-loop-if-convert",
    "-ftree-loop-im",
    "-ftree-loop-ivcanon",
    "-ftree-loop-optimize",
    "-ftree-loop-vectorize",
    "-ftree-partial-pre",
    "-ftree-phiprop",
    "-ftree-pre",
    "-ftree-pta",
    "-ftree-reassoc",
    "-ftree-scev-cprop",
    "-ftree-sink",
    "-ftree-slp-vectorize",
    "-ftree-slsr",
    "-ftree-sra",
    "-ftree-switch-conversion",
    "-ftree-tail-merge",
    "-ftree-ter",
    "-ftree-vrp",
    "-funsafe-math-optimizations",
    "-funswitch-loops",
    "-fvar-tracking",
    "-fvar-tracking-assignments",
    "-fweb"});

// Heap allocate so the destructor does not need to be called
static const auto &flag_to_parameter = *new std::map<std::string, unsigned>({
  //{"-faggressive-loop-optimizations",      FlagParameterID::kAggressiveLoopOptimizations},
  {"-falign-functions",                    FlagParameterID::kAlignFunctions},
  {"-falign-jumps",                        FlagParameterID::kAlignJumps},
  {"-falign-labels",                       FlagParameterID::kAlignLabels},
  {"-falign-loops",                        FlagParameterID::kAlignLoops},
  {"-fbranch-count-reg",                   FlagParameterID::kBranchCountReg},
  {"-fbranch-target-load-optimize",        FlagParameterID::kBranchTargetLoadOptimize},
  {"-fbranch-target-load-optimize2",       FlagParameterID::kBranchTargetLoadOptimize2},
  {"-fbtr-bb-exclusive",                   FlagParameterID::kBTRBBExclusive},
  {"-fcaller-saves",                       FlagParameterID::kCallerSaves},
  {"-fcombine-stack-adjustments",          FlagParameterID::kCombineStackAdjustments},
  {"-fcompare-elim",                       FlagParameterID::kCompareElim},
  {"-fconserve-stack",                     FlagParameterID::kConserveStack},
  {"-fcprop-registers",                    FlagParameterID::kCPropRegister},
  {"-fcrossjumping",                       FlagParameterID::kCrossJumping},
  {"-fcse-follow-jumps",                   FlagParameterID::kCSEFollowJumps},
  {"-fdce",                                FlagParameterID::kDCE},
  {"-fdefer-pop",                          FlagParameterID::kDeferPop},
  {"-fdelete-null-pointer-checks",         FlagParameterID::kDeleteNullPointerChecks},
  //{"-fdevirtualize",                       FlagParameterID::kDevirtualize},
  {"-fdse",                                FlagParameterID::kDSE},
  {"-fearly-inlining",                     FlagParameterID::kEarlyInlining},
  {"-fexpensive-optimizations",            FlagParameterID::kExpensiveOptimizations},
  {"-fforward-propagate",                  FlagParameterID::kForwardPropagate},
  {"-fgcse",                               FlagParameterID::kGCSE},
  {"-fgcse-after-reload",                  FlagParameterID::kGCSEAfterReload},
  {"-fgcse-las",                           FlagParameterID::kGCSELAS},
  {"-fgcse-lm",                            FlagParameterID::kGCSELM},
  {"-fgcse-sm",                            FlagParameterID::kGCSESM},
  {"-fguess-branch-probability",           FlagParameterID::kGuessBranchProbability},
  //{"-fhoist-adjacent-loads",               FlagParameterID::kHoistAdjacentLoads},
  {"-fif-conversion",                      FlagParameterID::kIfConversion},
  {"-fif-conversion2",                     FlagParameterID::kIfConversion2},
  {"-finline",                             FlagParameterID::kInline},
  {"-finline-atomics",                     FlagParameterID::kInlineAtomics},
  {"-finline-functions",                   FlagParameterID::kInlineFunctions},
  {"-finline-functions-called-once",       FlagParameterID::kInlineFunctionsCalledOnce},
  {"-finline-small-functions",             FlagParameterID::kInlineSmallFunctions},
  {"-fipa-cp",                             FlagParameterID::kIPACP},
  {"-fipa-cp-clone",                       FlagParameterID::kIPACPClone},
  {"-fipa-profile",                        FlagParameterID::kIPAProfile},
  {"-fipa-pta",                            FlagParameterID::kIPAPTA},
  {"-fipa-pure-const",                     FlagParameterID::kIPAPureConst},
  {"-fipa-reference",                      FlagParameterID::kIPAReference},
  {"-fipa-sra",                            FlagParameterID::kIPASRA},
  {"-fira-hoist-pressure",                 FlagParameterID::kIRAHoistPressure},
  {"-fivopts",                             FlagParameterID::kIVOpts},
  {"-fmerge-constants",                    FlagParameterID::kMergeConstants},
  {"-fmodulo-sched",                       FlagParameterID::kModuloSched},
  {"-fmove-loop-invariants",               FlagParameterID::kMoveLoopInvariants},
  {"-fomit-frame-pointer",                 FlagParameterID::kOmitFramePointer},
  {"-foptimize-sibling-calls",             FlagParameterID::kOptimizeSiblingCalls},
  //{"-foptimize-strlen",                    FlagParameterID::kOptimizeStrLen},
  {"-fpeephole",                           FlagParameterID::kPeephole},
  {"-fpeephole2",                          FlagParameterID::kPeephole2},
  {"-fpredictive-commoning",               FlagParameterID::kPredictiveCommoning},
  {"-fprefetch-loop-arrays",               FlagParameterID::kPrefetchLoopArrays},
  {"-fregmove",                            FlagParameterID::kRegMove},
  {"-frename-registers",                   FlagParameterID::kRenameRegisters},
  {"-freorder-blocks",                     FlagParameterID::kReorderBlocks},
  {"-freorder-functions",                  FlagParameterID::kReorderFunctions},
  {"-frerun-cse-after-loop",               FlagParameterID::kRerunCSEAfterLoop},
  {"-freschedule-modulo-scheduled-loops",  FlagParameterID::kRescheduleModuloScheduledLoops},
  {"-fsched-critical-path-heuristic",      FlagParameterID::kSchedCriticalPathHeuristic},
  {"-fsched-dep-count-heuristic",          FlagParameterID::kSchedDepCountHeuristic},
  {"-fsched-group-heuristic",              FlagParameterID::kSchedGroupHeuristic},
  {"-fsched-interblock",                   FlagParameterID::kSchedInterblock},
  {"-fsched-last-insn-heuristic",          FlagParameterID::kSchedLastInsnHeuristic},
  {"-fsched-pressure",                     FlagParameterID::kSchedPressure},
  {"-fsched-rank-heuristic",               FlagParameterID::kSchedRankHeuristic},
  {"-fsched-spec",                         FlagParameterID::kSchedSpec},
  {"-fsched-spec-insn-heuristic",          FlagParameterID::kSchedSpecInsnHeuristic},
  {"-fsched-spec-load",                    FlagParameterID::kSchedSpecLoad},
  {"-fsched-stalled-insns",                FlagParameterID::kSchedStalledInsns},
  {"-fsched-stalled-insns-dep",            FlagParameterID::kSchedStalledInsnsDep},
  {"-fschedule-insns",                     FlagParameterID::kScheduleInsns},
  {"-fschedule-insns2",                    FlagParameterID::kScheduleInsns2},
  //{"-fsection-anchors",                    FlagParameterID::kSectionAnchors},
  {"-fsel-sched-pipelining",               FlagParameterID::kSelSchedPipelining},
  {"-fsel-sched-pipelining-outer-loops",   FlagParameterID::kSelSchedPipeliningOuterLoops},
  {"-fsel-sched-reschedule-pipelined",     FlagParameterID::kSelSchedReschedulePipelined},
  {"-fselective-scheduling",               FlagParameterID::kSelectiveScheduling},
  {"-fselective-scheduling2",              FlagParameterID::kSelectiveScheduling2},
  {"-fshrink-wrap",                        FlagParameterID::kShrinkWrap},
  {"-fsplit-ivs-in-unroller",              FlagParameterID::kSplitIVsInUnroller},
  {"-fsplit-wide-types",                   FlagParameterID::kSplitWideTypes},
  {"-fthread-jumps",                       FlagParameterID::kThreadJumps},
  {"-ftoplevel-reorder",                   FlagParameterID::kTopLevelReorder},
  {"-ftree-bit-ccp",                       FlagParameterID::kTreeBitCCP},
  {"-ftree-builtin-call-dce",              FlagParameterID::kTreeBuiltinCallDCE},
  {"-ftree-ccp",                           FlagParameterID::kTreeCCP},
  {"-ftree-ch",                            FlagParameterID::kTreeCH},
  //{"-ftree-coalesce-inlined-vars",         FlagParameterID::kTreeCoalesceInlinedVars},
  {"-ftree-coalesce-vars",                 FlagParameterID::kTreeCoalesceVars},
  {"-ftree-copy-prop",                     FlagParameterID::kTreeCopyProp},
  {"-ftree-copyrename",                    FlagParameterID::kTreeCopyRename},
  {"-ftree-cselim",                        FlagParameterID::kTreeCSEElim},
  {"-ftree-dce",                           FlagParameterID::kTreeDCE},
  {"-ftree-dominator-opts",                FlagParameterID::kTreeDominatorOpts},
  {"-ftree-dse",                           FlagParameterID::kTreeDSE},
  {"-ftree-forwprop",                      FlagParameterID::kTreeForwProp},
  {"-ftree-fre",                           FlagParameterID::kTreeFRE},
  //{"-ftree-loop-distribute-patterns",      FlagParameterID::kTreeLoopDistributePatterns},
  {"-ftree-loop-distribution",             FlagParameterID::kTreeLoopDistribution},
  {"-ftree-loop-if-convert",               FlagParameterID::kTreeLoopIfConvert},
  {"-ftree-loop-im",                       FlagParameterID::kTreeLoopIM},
  {"-ftree-loop-ivcanon",                  FlagParameterID::kTreeLoopIVCanon},
  {"-ftree-loop-optimize",                 FlagParameterID::kTreeLoopOptimize},
  //{"-ftree-partial-pre",                   FlagParameterID::kTreePartialPre},
  {"-ftree-phiprop",                       FlagParameterID::kTreePhiProp},
  {"-ftree-pre",                           FlagParameterID::kTreePre},
  {"-ftree-pta",                           FlagParameterID::kTreePTA},
  {"-ftree-reassoc",                       FlagParameterID::kTreeReassoc},
  {"-ftree-scev-cprop",                    FlagParameterID::kTreeSCEVCProp},
  {"-ftree-sink",                          FlagParameterID::kTreeSink},
  {"-ftree-slp-vectorize",                 FlagParameterID::kTreeSLPVectorize},
  {"-ftree-slsr",                          FlagParameterID::kTreeSLSR},
  {"-ftree-sra",                           FlagParameterID::kTreeSRA},
  {"-ftree-switch-conversion",             FlagParameterID::kTreeSwitchConversion},
  //{"-ftree-tail-merge",                    FlagParameterID::kTreeTailMerge},
  {"-ftree-ter",                           FlagParameterID::kTreeTER},
  {"-ftree-vect-loop-version",             FlagParameterID::kTreeVectLoopVersion},
  {"-ftree-vectorize",                     FlagParameterID::kTreeVectorize},
  {"-ftree-vrp",                           FlagParameterID::kTreeVRP},
  {"-funroll-all-loops",                   FlagParameterID::kUnrollAllLoops},
  {"-funroll-loops",                       FlagParameterID::kUnrollLoops},
  {"-funswitch-loops",                     FlagParameterID::kUnswitchLoops},
  {"-fvariable-expansion-in-unroller",     FlagParameterID::kVariableExpansionInUnroller},
  {"-fvect-cost-model",                    FlagParameterID::kVectCostModel},
  {"-fweb",                                FlagParameterID::kWeb},
});

struct ParameterToFlag {
  ParameterToFlag() {}
  operator std::map<unsigned, std::string>() {
    static bool is_init = false;
    static auto &parameter_to_flag = *new std::map<unsigned, std::string>();

    if (!is_init) {
      for (auto entry : flag_to_parameter)
        parameter_to_flag[entry.second] = entry.first;
      is_init = true;
    }
    return parameter_to_flag;
  }
};
static const std::map<unsigned, std::string> &parameter_to_flag =
    *new ParameterToFlag();

// Split a string on a provided character
static std::vector<std::string> splitString(std::string str, char c) {
  std::vector<std::string> res;

  std::string buf;
  for (auto I = str.begin(); I != str.end(); ++I) {
    if (*I != c) {
      buf.push_back(*I);
    } else {
      res.push_back(buf);
      buf.clear();
    }
  }
  res.push_back(buf);
  return res;
}

// Print the version of this driver.
static void printVersion() {
  mageec::util::out() << MAGEEC_PREFIX "Driver version: "
                      << static_cast<std::string>(gcc_driver_version) << '\n';
}

// Print the version of the database that was specified for use
static int printDatabaseVersion(mageec::Framework &framework,
                                const std::string &db_path) {
  std::unique_ptr<mageec::Database> db = framework.getDatabase(db_path, false);
  if (!db) {
    MAGEEC_ERR("Error retrieving database. The database may not exists, or "
               "you may not have sufficient permissions to read it");
    return -1;
  }
  mageec::util::out() << MAGEEC_PREFIX "Database version: "
                      << static_cast<std::string>(db->getVersion()) << '\n';
  return 0;
}

// Print the version of the framework that the driver was compiled with.
static void printFrameworkVersion(mageec::Framework &framework) {
  mageec::util::out() << MAGEEC_PREFIX "Framework version: "
                      << static_cast<std::string>(framework.getVersion())
                      << '\n';
}


struct FeatureIDEntry {
  std::string            name;
  mageec::FeatureSetID   id;
  mageec::FeatureClass   feature_class;

  bool operator<(const FeatureIDEntry &other) const {
    if (name < other.name)
      return true;
    return false;
  }
};

struct FileFeatureIDs {
  mageec::util::Option<FeatureIDEntry> module;
  std::set<FeatureIDEntry>             functions;
};

static mageec::util::Option<std::map<std::string, FileFeatureIDs>>
loadFeatureIDs(std::string features_path) {
  std::map<std::string, FileFeatureIDs> file_to_features;

  std::ifstream features_file(features_path);
  if (!features_file.is_open()) {
    MAGEEC_ERR("Error opening features file. The file may not exist, or you "
               "may not have sufficient permissions to read and write it");
    return nullptr;
  }

  std::string line;
  while (std::getline(features_file, line)) {
    std::vector<std::string> values = splitString(line, ',');
    if (values.size() != 7)
      continue;
    if ((values[1] != "module") && (values[1] != "function"))
      continue;
    if (values[3] != "features")
      continue;
    if (values[5] != "feature_class")
      continue;
    if (!values[0].size() || !values[2].size() || !values[4].size() ||
        !values[6].size()) {
      continue;
    }

    std::stringstream feat_id_str(values[4]);
    uint64_t feat_id;
    feat_id_str >> feat_id;
    if (feat_id_str.fail()) {
      MAGEEC_ERR("Malformed line in features file");
      return nullptr;
    }

    std::stringstream feat_class_str(values[6]);
    unsigned feat_class;
    feat_class_str >> feat_class;
    if (feat_class_str.fail()) {
      MAGEEC_ERR("Malformed line in features file");
      return nullptr;
    }

    // Entry to be inserted into the map
    FeatureIDEntry entry = { values[2],
                             static_cast<mageec::FeatureSetID>(feat_id),
                             static_cast<mageec::FeatureClass>(feat_class) };

    FileFeatureIDs &file_entry = file_to_features[values[0]];
    if (values[1] == "module") {
      if (file_entry.module) {
        FeatureIDEntry old_entry = file_entry.module.get();
        if (old_entry.id != entry.id
            || old_entry.feature_class != entry.feature_class) {
          MAGEEC_WARN("Multiple entries for module: " << entry.name
                      << " with different feature sets");
        }
      }
      file_entry.module = entry;
    } else {
      assert(values[1] == "function");
      if (file_entry.functions.count(entry)) {
        FeatureIDEntry old_entry = *file_entry.functions.find(entry);
        if (old_entry.id != entry.id
            || old_entry.feature_class != entry.feature_class) {
          MAGEEC_WARN("Multiple entries for function: " << entry.name
                      << " with different feature sets");
        }
      }
      file_entry.functions.insert(entry);
    }
  }
  return file_to_features;
}

// Print the help output string
static void printHelp() {
  mageec::util::out() <<
"Wrapper around gcc which can interact with the mageec framework\n"
"\n"
"Basic options:\n"
"  -fmageec-help               Print this help information\n"
"  -fmageec-version            Print out the version of this driver\n"
"  -fmageec-database-version   Print the version of the provided database\n"
"  -fmageec-framework-version  Print the version of the MAGEEC framework\n"
"  -fmageec-debug              Enable debug output\n"
"  -fmageec-gcc=<command>      Command to invoke gcc\n"
"  -fmageec-g++=<command>      Command to invoke g++\n"
"  -fmageec-gfortran=<command> Command to invoke gfortran\n"
"  -fmageec-mode=<mode>        Mode of the driver, valid values are\n"
"                              gather and optimize\n"
"  -fmageec-database=<file>    Database to record to\n"
"  -fmageec-features=<file>    File containing feature group identifiers\n"
"  -fmageec-out=<file>         File to output compilation ids into\n"
"  -fmageec-ml=<id>            string identifier or shared object identifying\n"
"                              the machine learner to be used\n"
"  -fmageec-metric=<name>      Metric to optimize for\n";
}

static bool endsWith(std::string str, std::string ending) {
  if (str.size() >= ending.size())
    return str.compare(str.size() - ending.size(), ending.size(), ending) == 0;
  else
    return false;
}

int main(int argc, const char *argv[]) {
  DriverMode mode = DriverMode::kNone;

  // The gcc command to use
  std::string gcc_command;
  // The g++ command to use
  std::string gxx_command;
  // The gfortran command to use
  std::string gfortran_command;
  // The database to gather into, or use when optimizing
  std::string db_str;
  // File holding the features for input programs
  std::string features_path;
  // File into which the compilation ids for the program should be output
  std::string out_path;
  // Params which this compilation will be compiled with
  std::vector<std::string> param_list;
  // The machine learner to optimize with
  std::string ml_str;
  // The metric to use when optimizing
  std::string metric_str;

  bool with_help              = false;
  bool with_version           = false;
  bool with_db_version        = false;
  bool with_framework_version = false;
  bool with_debug             = false;
  bool with_gcc_command       = false;
  bool with_gxx_command       = false;
  bool with_gfortran_command  = false;
  bool with_db                = false;
  bool with_features          = false;
  bool with_out               = false;
  bool with_ml                = false;
  bool with_metric            = false;

  // Handle arguments controlling mageec, accumulate the arguments which
  // aren't controlling this driver
  std::vector<std::string> cmd_args;
  for (int i = 0; i < argc; ++i) {
    std::string arg = argv[i];

    if (arg.compare(0, strlen("-fmageec-"), "-fmageec-") != 0) {
      cmd_args.push_back(arg);
      continue;
    }
    arg = std::string(arg.begin() + strlen("-fmageec-"), arg.end());

    // Common flags
    bool handled = false;
    if (arg == "help") {
      handled = with_help = true;
    } else if (arg == "version") {
      handled = with_version = true;
    } else if (arg == "database-version") {
      handled = with_db_version = true;
    } else if (arg == "framework-version") {
      handled = with_framework_version = true;
    } else if (arg == "debug") {
      handled = with_debug = true;
    }
    if (handled)
      continue;

    // Flags with values
    if (arg.compare(0, strlen("gcc="), "gcc=") == 0) {
      gcc_command = std::string(arg.begin() + strlen("gcc="), arg.end());
      if (gcc_command == "") {
        MAGEEC_ERR("No gcc command provided");
        return -1;
      }
      with_gcc_command = true;
    } else if (arg.compare(0, strlen("g++="), "g++=") == 0) {
      gxx_command = std::string(arg.begin() + strlen("g++="), arg.end());
      if (gcc_command == "") {
        MAGEEC_ERR("No gcc command provided");
        return -1;
      }
      with_gxx_command = true;
    } else if (arg.compare(0, strlen("gfortran="), "gfortran=") == 0) {
      gfortran_command =
          std::string(arg.begin() + strlen("gfortran="), arg.end());
      if (gfortran_command == "") {
        MAGEEC_ERR("No gfortran command provided");
        return -1;
      }
      with_gfortran_command = true;
    } else if (arg.compare(0, strlen("mode="), "mode=") == 0) {
      std::string mode_str(arg.begin() + strlen("mode="), arg.end());

      if (mode_str == "gather") {
        mode = DriverMode::kGather;
      } else if (mode_str == "optimize") {
        mode = DriverMode::kOptimize;
      } else {
        MAGEEC_ERR("Unknown mode: '" << mode_str << "'");
        return -1;
      }
    } else if (arg.compare(0, strlen("database="), "database=") == 0) {
      db_str = std::string(arg.begin() + strlen("database="), arg.end());
      if (db_str == "") {
        MAGEEC_ERR("No database path provided");
        return -1;
      }
      with_db = true;
    } else if (arg.compare(0, strlen("features="), "features=") == 0) {
      features_path = std::string(arg.begin() + strlen("features="), arg.end());
      if (features_path.size() == 0) {
        MAGEEC_ERR("No feature path provided");
        return -1;
      }
      with_features = true;
    } else if (arg.compare(0, strlen("out="), "out=") == 0) {
      out_path = std::string(arg.begin() + strlen("out="), arg.end());
      if (out_path.size() == 0) {
        MAGEEC_ERR("No config file path provided");
        return -1;
      }
      with_out = true;
    } else if (arg.compare(0, strlen("ml="), "ml=") == 0) {
      ml_str = std::string(arg.begin() + strlen("ml="), arg.end());
      if (ml_str.size() == 0) {
        MAGEEC_ERR("No machine learner provided");
        return -1;
      }
      with_ml = true;
    } else if (arg.compare(0, strlen("metric="), "metric=") == 0) {
      metric_str = std::string(arg.begin() + strlen("metric="), arg.end());
      if (metric_str.size() == 0) {
        MAGEEC_ERR("No metric value provided");
        return -1;
      }
      with_metric = true;
    } else {
      MAGEEC_ERR("Unknown argument -fmageec-" << arg);
      return -1;
    }
  }

  // Errors
  bool have_error = false;
  if (mode == DriverMode::kOptimize) {
    if (!with_db) {
      MAGEEC_ERR("Optimize mode specified without a database");
      have_error = true;
    }
    if (!with_features) {
      MAGEEC_ERR("Optimize mode specified without a features file");
      have_error = true;
    }
    if (!with_out) {
      MAGEEC_ERR("Optimize mode specified without an output file");
      have_error = true;
    }
    if (!with_metric) {
      MAGEEC_ERR("Optimize mode specified without any metric to optimize for");
      have_error = true;
    }
    if (!with_ml) {
      MAGEEC_ERR("Optimize mode specified without a machine learner to use");
      have_error = true;
    }
  } else if (mode == DriverMode::kGather) {
    if (!with_db) {
      MAGEEC_ERR("Gather mode specified without a database");
      have_error = true;
    }
    if (!with_features) {
      MAGEEC_ERR("Gather mode specified without a features file ");
      have_error = true;
    }
    if (!with_out) {
      MAGEEC_ERR("Gather mode specified without an output file");
      have_error = true;
    }
  }
  if (have_error)
    return -1;

  // Warnings
  if (mode == DriverMode::kGather) {
    if (with_ml)
      MAGEEC_WARN("-fmageec-ml argument will be ignored");
    if (with_metric)
      MAGEEC_WARN("-fmageec-metric argument will be ignored");
  }

  // Initialize the framework, and register some builtin machine learners so
  // they can be selected by name by the user.
  mageec::Framework framework(with_debug);

  // C5 classifier
  MAGEEC_DEBUG("Registering C5.0 machine learner interface");
  std::unique_ptr<mageec::IMachineLearner> c5_ml(new mageec::C5Driver());
  framework.registerMachineLearner(std::move(c5_ml));

  // Select the machine learner chosen by the user. This may be the name of
  // an already register machine learner, or a path to a shared object which
  // needs to be loaded and registered.
  mageec::IMachineLearner *ml = nullptr;
  if (with_ml) {
    MAGEEC_DEBUG("Selecting machine learner: " << ml_str)

    // Try to select the machine learner assuming that it has already been
    // loaded
    std::set<mageec::IMachineLearner *> mls = framework.getMachineLearners();
    for (auto *ml_interface : mls) {
      if (ml_interface->getName() == ml_str) {
        ml = ml_interface;
        break;
      }
    }
    if (!ml) {
      MAGEEC_DEBUG(ml_str << " not a registered machine learner... attempting "
                   << "to load as a plugin");
      auto ml_name = framework.loadMachineLearner(ml_str);
      if (ml_name == "") {
        MAGEEC_ERR("Could not load user machine learner" << ml_str);
        return -1;
      } else {
        MAGEEC_DEBUG("Loaded machine learner plugin: " << ml_name);
      }
      std::set<mageec::IMachineLearner *> mls = framework.getMachineLearners();
      for (auto *ml_interface : mls) {
        if (ml_interface->getName() == ml_name) {
          ml = ml_interface;
          break;
        }
      }
      assert(ml);
    }
  }

  // Handle basic options
  if (with_help)
    printHelp();

  if (with_version)
    printVersion();

  if (with_db_version) {
    int res = printDatabaseVersion(framework, db_str);
    if (res != 0)
      return res;
  }
  if (with_framework_version)
    printFrameworkVersion(framework);

  // Arguments controlling mageec have been handled, now parse the underlying
  // gcc command line to find the input files and optimization flags used.
  bool to_obj = false;
  for (unsigned i = 0; i < cmd_args.size(); ++i) {
    if (cmd_args[i] == "-c")
      to_obj = true;

    // Check if the output file has a .o extension
    if (cmd_args[i] == "-o") {
      ++i;
      if (i < cmd_args.size()) {
        auto arg = cmd_args[i];
        if (arg.size() > strlen(".o")) {
          if (arg.compare(arg.size() - strlen(".o"), strlen(".o"), ".o") == 0)
            to_obj = true;
        }
      }
    }
    // A subsequent -S or -E override a preceding -c flag
    if (cmd_args[i] == "-S" || cmd_args[i] == "-E") {
      to_obj = false;
    }
  }

  // Replace the command word depending on the underlying wrapper being used.
  //
  // For the 'mageec-gcc' wrapper, use the argument from '-fmageec-gcc', and
  // for 'mageec-gfortran' and 'mageec-g++', use the '-fmageec-gfortran' and
  // '-fmageec-g++' arguments respectively.
  if (endsWith(cmd_args[0], "mageec-gcc")) {
    if (with_gcc_command)
      cmd_args[0] = gcc_command;
    else
      cmd_args[0] = "gcc";
  } else if (endsWith(cmd_args[0], "mageec-g++")) {
    if (with_gxx_command)
      cmd_args[0] = gxx_command;
    else
      cmd_args[0] = "g++";
  } else {
    assert(endsWith(cmd_args[0], "mageec-gfortran"));
    if (with_gfortran_command)
      cmd_args[0] = gfortran_command;
    else
      cmd_args[0] = "gfortran";
  }

  // If we are not in 'gather' or 'optimize' modes, or if we're not compiling
  // to an object file, then just run the original command
  if (!to_obj || (mode == DriverMode::kNone)) {
    std::stringstream command;
    for (unsigned i = 0; i < cmd_args.size(); ++i) {
      if (i == 0) {
        command << cmd_args[i];
      } else {
        command << " " << cmd_args[i];
      }
    }
    if (!to_obj && with_debug) {
      MAGEEC_WARN("MAGEEC driver called, but not compiling to an object file, "
                  "calling the original command");
    }
    if (with_debug) {
      MAGEEC_DEBUG("Executing command: " + command.str());
    }
    // FIXME: Windows?
    return system(command.str().c_str());
  }

  // Names of all of the input files involved in the compilation
  std::vector<std::string> src_files;

  // Find the input files and strip them from the command line (they will be
  // added back individually later).
  //
  // If an input file has an extension that we can't handle, then warn, but
  // pass it on to be compiled anyway.
  //
  // It's is assumed that anything without a preceding '-' is a filename,
  // excluding the command word, and the filename after the '-o' option.
  std::vector<std::string> new_cmd_args = {cmd_args[0]};
  auto arg_iter = std::next(cmd_args.begin());

  for (; arg_iter != cmd_args.end(); ++arg_iter) {
    auto arg = *arg_iter;

    // skip over the file after a -o argument
    if (!arg.compare(arg.size() - strlen("-o"), strlen("-o"), "-o")) {
      new_cmd_args.push_back(arg);
      if (std::next(arg_iter) != cmd_args.end()) {
        arg = *(++arg_iter);
        new_cmd_args.push_back(arg);
      }
      continue;
    }
    // ignore any other arguments beginning with '-', since they can't be
    // filenames (hopefully).
    if (arg[0] == '-') {
      new_cmd_args.push_back(arg);
      continue;
    }

    // Check the extension to see if it's a filename that can be handled
    // by the framework. If not, pass it on anyway, but warn in case it's
    // a case we haven't handled.
    const char *src_file_exts[] = {
      // C input file extensions
      ".c", ".i",
      // C++ input file extensions
      ".ii", ".cc", ".cp", ".cxx", ".cpp", ".CPP", ".c++", ".C",
      // Fortran input file extensions
      ".f", ".for", ".ftn", ".F", ".FOR", ".fpp", ".FPP", ".FTN", ".f90",
      ".f95", ".f03", ".f08", ".F90", ".F95", ".F03", ".F08",
      // Assembly code
      ".s", ".S", ".sx"
    };
    bool found_ext = false;
    for (auto ext : src_file_exts)
      found_ext |= arg.compare(arg.size() - strlen(ext), strlen(ext), ext) == 0;
    if (!found_ext)
      MAGEEC_WARN("Unrecognized extension on input file '" + arg + "'");

    if (with_debug)
      MAGEEC_DEBUG("Found input file '" + arg + "'");
    src_files.push_back(arg);
  };
  cmd_args = new_cmd_args;

  // Load the database
  assert((mode == DriverMode::kOptimize) || (mode == DriverMode::kGather));
  std::unique_ptr<mageec::Database> db = framework.getDatabase(db_str, false);
  if (!db) {
    MAGEEC_ERR("Error retrieving database. The database may not exists, or "
               "you may not have sufficient permissions to read it");
    return -1;
  }

  // Load the features file to get the feature groups
  auto feature_groups = loadFeatureIDs(features_path);
  if (!feature_groups) {
    MAGEEC_ERR("Failed to retrieve feature groups from features file");
    return -1;
  }

  // Extract the parameters that were provided to the compiler on the command
  // line. Use this to build up a parameter set.
  std::set<unsigned> orig_params;

  // A copy of the command with the optimization flags stripped out
  std::vector<std::string> stripped_cmd_args;

  // First set the parameter set according to the basic optimization level
  // The rightmost optimization flag takes precedence.
  std::string opt_level = "-O0";
  for (auto arg : cmd_args) {
    if (arg == "-O0" || arg == "-O"  || arg == "-O1" || arg == "-O2" ||
        arg == "-O3" || arg == "-O4" || arg == "-Os" || arg == "-Ofast") {
      opt_level = arg;
    }
  }
  std::vector<std::string> flags;
  if (opt_level == "-O0")
    flags = opt_flags_O0;
  if (opt_level == "-O" || opt_level == "-O1")
    flags = opt_flags_O1;
  if (opt_level == "-O2")
    flags = opt_flags_O2;
  if (opt_level == "-O3")
    flags = opt_flags_O3;
  if (opt_level == "-O4")
    flags = opt_flags_O4;
  if (opt_level == "-Os")
    flags = opt_flags_Os;
  if (opt_level == "-Ofast")
    flags = opt_flags_Ofast;

  // flag -> parameter
  for (auto f : flags) {
    auto p = flag_to_parameter.find(f);
    if (p != flag_to_parameter.end())
      orig_params.insert(p->second);
  }

  // Now, toggle individual flags, modifying the base set of flags. The
  // rightmost flag takes precedence.
  //
  // Also build a stripped command, with all of the recognized optimization
  // flags omitted.
  for (auto arg : cmd_args) {
    if (arg == "-O0" || arg == "-O"  || arg == "-O1" || arg == "-O2" ||
        arg == "-O3" || arg == "-O4" || arg == "-Os" || arg == "-Ofast") {
      continue;
    }

    // Individual flags may enable the parameter, or disable it if it has a
    // "-fno-" prefix. First check if the flag enables a parameter, then if
    // it disables one.
    auto p = flag_to_parameter.find(arg);
    if (p != flag_to_parameter.end()) {
      orig_params.insert(p->second);
      continue;
    } else {
      if (arg.size() > strlen("-fno-") &&
          !arg.compare(0, strlen("-fno-"), "-fno-")) {
        arg = std::string("-f") +
              std::string(arg.begin() + strlen("-fno-"), arg.end());

        p = flag_to_parameter.find(arg);
        if (p != flag_to_parameter.end()) {
          orig_params.erase(p->second);
          continue;
        }
      }
    }
    // If this isn't an -O flag, or a parameter flag we recognize, add it to
    // the stripped command arguments
    stripped_cmd_args.push_back(arg);
  }

  auto src_file_feature_set_ids = feature_groups.get();
  std::map<std::string, std::set<unsigned>> src_file_parameters;
  std::map<std::string, mageec::ParameterSetID> src_file_parameter_set_ids;

  if (mode == DriverMode::kGather) {
    // When in 'gather' mode, the parameters used for each file are based on
    // the flags originally provided on the command line
    mageec::ParameterSet param_set;
    for (unsigned i = FlagParameterID::kFIRST_FLAG_PARAMETER;
         i <= FlagParameterID::kLAST_FLAG_PARAMETER; ++i) {
      auto param_flag = parameter_to_flag.at(i);
      param_set.add(std::make_shared<mageec::BoolParameter>(i,
                                                            orig_params.count(i),
                                                            param_flag));
    }
    // Add the set of parameters to the database
    auto param_set_id = db->newParameterSet(param_set);

    // Use the same parameters for every input file
    for (auto file_arg : src_files) {
      auto src_file_path = mageec::util::getFullPath(file_arg);
      src_file_parameters[src_file_path] = orig_params;
      src_file_parameter_set_ids[src_file_path] = param_set_id;
    }
  } else {
    // When in 'optimize' mode, the parameters used for each file are based on
    // flags generated from the features
    assert(mode == DriverMode::kOptimize);

    // Find the selected machine learner trained for the specified metric
    auto trained_mls = db->getTrainedMachineLearners();
    mageec::TrainedML *chosen_ml = nullptr;

    bool found_ml = false;
    for (auto &trained_ml : trained_mls) {
      if (trained_ml.getName() == ml->getName()) {
        found_ml = true;

        // Check that the found machine learner is trained for the desired
        // metric and class of features.
        // TODO: Only module features can be handled here
        if (trained_ml.getMetric() == metric_str &&
            trained_ml.getFeatureClass() == mageec::FeatureClass::kModule) {
          chosen_ml = &trained_ml;
          break;
        }
      }
    }
    if (!found_ml) {
      MAGEEC_ERR("Could not find training data for specified machine learner "
                 "and metric");
      return -1;
    }

    // For each input file, use the set of features for the file and the
    // user-specified machine learner to generate flags for the compilation
    for (auto file_arg : src_files) {
      auto src_file_path = mageec::util::getFullPath(file_arg);

      // Ignore any files which don't have associated features, these will
      // be built with the original command line
      auto feature_set_ids = src_file_feature_set_ids.find(src_file_path);
      if (feature_set_ids == src_file_feature_set_ids.end())
        continue;

      // Generate a parameter set based on the features of the module for the
      // file
      //
      // Use the original parameter configuration provided on the command line
      // as the base set of flags. If a parameter value is not overridden by
      // mageec then this will form the 'native' decision
      assert(feature_set_ids->second.module);
      auto feature_set_id = feature_set_ids->second.module.get().id;
      auto features = db->getFeatureSetFeatures(feature_set_id);
      assert(features.size() != 0);

      std::set<unsigned> params;
      mageec::ParameterSet param_set;
      for (unsigned i = FlagParameterID::kFIRST_FLAG_PARAMETER;
           i <= FlagParameterID::kLAST_FLAG_PARAMETER; ++i) {
        assert(chosen_ml);

        mageec::BoolDecisionRequest req(i);
        auto res = chosen_ml->makeDecision(req, features);

        bool enabled = false;
        if (res->getType() == mageec::DecisionType::kNative) {
          enabled = orig_params.count(i);
        } else {
          auto *decision = static_cast<mageec::BoolDecision*>(res.get());
          enabled = decision->getValue();
        }

        auto param_flag = parameter_to_flag.at(i);
        param_set.add(std::make_shared<mageec::BoolParameter>(i, enabled,
                                                              param_flag));
        params.insert(i);
      }
      // Add the set of parameters to the database
      auto param_set_id = db->newParameterSet(param_set);

      src_file_parameters[src_file_path] = params;
      src_file_parameter_set_ids[src_file_path] = param_set_id;
    }
  }

  // Mapping from an input filename, to a command string to compile that file
  // With the appropriate set of parameters.
  std::map<std::string, std::string> src_file_commands;

  for (auto file_arg : src_files) {
    auto src_file_path = mageec::util::getFullPath(file_arg);

    // If this file doesn't have any features, then it cannot be affected by
    // mageec. Just use the original command with the input filename appended
    if (src_file_feature_set_ids.count(src_file_path) == 0) {
      std::stringstream command;
      for (unsigned i = 0; i < cmd_args.size(); ++i) {
        if (i == 0)
          command << cmd_args[i];
        else
          command << " " << cmd_args[i];
      }
      // Add in the input file
      command << " " << file_arg;

      src_file_commands[src_file_path] = command.str();
      continue;
    }

    // Otherwise, this file has features, and therefore will also have
    // associated parameters. Use these parameters to build up a new
    // command line
    auto params = src_file_parameters[src_file_path];

    // Build the command line for this file based on the 'stripped' command
    // line derived earlier, with the specified optimization flags enabled
    // or disabled.
    auto cmd_iter = stripped_cmd_args.begin();
    std::vector<std::string> file_cmd;

    // Add the original command word
    file_cmd.push_back(*cmd_iter);
    ++cmd_iter;

    // In both 'gather' and 'optimize' -O3 is used as a base set of flags.
    // This gives us a common configuration for both gather and optimizing, and
    // by running at -O3 we are also able to toggle the whole range of
    // optimizations passes
    file_cmd.push_back("-O3");

    // Add the optimization flags to the command. This adds *all* optimization
    // flags which the driver knows about, either in -fflag-name or
    // -fno-flag-name form depending on whether the flag is enabled or
    // disabled
    for (unsigned i = FlagParameterID::kFIRST_FLAG_PARAMETER;
         i <= FlagParameterID::kLAST_FLAG_PARAMETER; ++i) {
      std::string flag = parameter_to_flag.at(i);

      if (params.count(i)) {
        file_cmd.push_back(flag);
      } else {
        assert(!flag.compare(0, strlen("-f"), "-f"));
        flag = std::string("-fno-") +
               std::string(flag.begin() + strlen("-f"), flag.end());
        file_cmd.push_back(flag);
      }
    }

    // Add the remaining flags from the original command
    for (; cmd_iter != stripped_cmd_args.end(); ++cmd_iter)
      file_cmd.push_back(*cmd_iter);

    // Add the input filename
    file_cmd.push_back(file_arg);

    // Build a command appropriate to compile each of the files
    std::stringstream command;
    for (unsigned i = 0; i < file_cmd.size(); ++i) {
      if (i == 0)
        command << file_cmd[i];
      else
        command << " " << file_cmd[i];
    }
    src_file_commands[src_file_path] = command.str();
  }

  // Compile each of the files in turn, if any fail, error out early
  for (auto file_arg : src_files) {
    auto src_file_path = mageec::util::getFullPath(file_arg);
    auto command = src_file_commands[src_file_path];

    // FIXME: Windows?
    MAGEEC_DEBUG("Executing command: " << command);
    int res = system(command.c_str());
    if (res) {
      MAGEEC_ERR("Compilation failed\ncommand: " << command);
      return res;
    }
  }

  // If all of the file compiled successfully, generated compilation ids for
  // them and output these ids into the output file
  std::ofstream out_file(out_path, std::ios::app);
  if (!out_file.is_open()) {
    MAGEEC_ERR("Error opening output file. The file may not exist, or you "
               "may not have sufficient permissions to read and write it");
    return -1;
  }
  for (auto file_arg : src_files) {
    std::string src_file_path = mageec::util::getFullPath(file_arg);
    auto feature_set_ids = src_file_feature_set_ids.find(src_file_path);
    auto param_set_id = src_file_parameter_set_ids.find(src_file_path);

    // If there were no features for this file, then parameters would not have
    // been derived and there will be no compilation id
    if (feature_set_ids == src_file_feature_set_ids.end())
      continue;
    assert(param_set_id != src_file_parameter_set_ids.end());

    // Generate a compilation id for the module
    // Append the generated compilation ids to the output file
    assert(feature_set_ids->second.module);
    auto module_entry = feature_set_ids->second.module.get();
    auto module_compilation = db->newCompilation(module_entry.name, "module",
                                                 module_entry.id,
                                                 mageec::FeatureClass::kModule,
                                                 param_set_id->second,
                                                 src_file_commands[src_file_path],
                                                 nullptr);

    // TODO: Avoid static_cast here
    uint64_t tmp = static_cast<uint64_t>(module_compilation);
    out_file << src_file_path << ",module," << module_entry.name
                               << ",compilation," << tmp << "\n";

    // Generate a compilation id for each of the functions in the module.
    for (auto function_entry : feature_set_ids->second.functions) {
      auto function_compilation =
          db->newCompilation(function_entry.name, "function",
                             function_entry.id, mageec::FeatureClass::kFunction,
                             param_set_id->second,
                             src_file_commands[src_file_path],
                             module_compilation);

      // TODO: Avoid static cast here
      tmp = static_cast<uint64_t>(function_compilation);
      out_file << src_file_path << ",function," << function_entry.name
                                 << ",compilation," << tmp << "\n";
    }
  }
  return 0;
}
