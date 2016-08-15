#include "mageec/Database.h"
#include "mageec/Framework.h"
#include "mageec/ML/C5.h"
#include "mageec/Util.h"
#include "Parameters.h"

#include <map>
#include <memory>
#include <string>


#if !defined(GCC_DRIVER_VERSION_MAJOR) || \
		!defined(GCC_DRIVER_VERSION_MINOR) || \
    !defined(GCC_DRIVER_VERSION_PATCH)
#error "MAGEEC gcc driver version not defined by the build system"
#endif
static_assert(GCC_DRIVER_VERSION_MAJOR == 1 &&
						  GCC_DRIVER_VERSION_MINOR == 0 &&
						  GCC_DRIVER_VERSION_PATCH == 0,
							"MAGEEC gcc driver version does not match");
static const mageec::util::Version gcc_driver_version(GCC_DRIVER_VERSION_MAJOR,
                                                      GCC_DRIVER_VERSION_MINOR,
                                                      GCC_DRIVER_VERSION_PATCH);

enum class DriverMode { kNone, kGather, kOptimize };

static const std::map<std::string, unsigned> flag_to_parameter = {
  {"-faggressive-loop-optimizations",      FlagParameterID::kAggressiveLoopOptimizations},
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
  {"-fcommon",                             FlagParameterID::kCommon},
  {"-fcompare-elim",                       FlagParameterID::kCompareElim},
  {"-fconserve-stack",                     FlagParameterID::kConserveStack},
  {"-fcprop-registers",                    FlagParameterID::kCPropRegister},
  {"-fcrossjumping",                       FlagParameterID::kCrossJumping},
  {"-fcse-follow-jumps",                   FlagParameterID::kCSEFollowJumps},
  {"-fdata-sections",                      FlagParameterID::kDataSections},
  {"-fdce",                                FlagParameterID::kDCE},
  {"-fdefer-pop",                          FlagParameterID::kDeferPop},
  {"-fdelete-null-pointer-checks",         FlagParameterID::kDeleteNullPointerChecks},
  {"-fdevirtualize",                       FlagParameterID::kDevirtualize},
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
  {"-fhoist-adjacent-loads",               FlagParameterID::kHoistAdjacentLoads},
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
  {"-foptimize-strlen",                    FlagParameterID::kOptimizeStrLen},
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
  {"-fsection-anchors",                    FlagParameterID::kSectionAnchors},
  {"-fsel-sched-pipelining",               FlagParameterID::kSelSchedPipelining},
  {"-fsel-sched-pipelining-outer-loops",   FlagParameterID::kSelSchedPipeliningOuterLoops},
  {"-fsel-sched-reschedule-pipelined",     FlagParameterID::kSelSchedReschedulePipelined},
  {"-fselective-scheduling",               FlagParameterID::kSelectiveScheduling},
  {"-fselective-scheduling2",              FlagParameterID::kSelectiveScheduling2},
  {"-fshrink-wrap",                        FlagParameterID::kShrinkWrap},
  {"-fsplit-ivs-in-unroller",              FlagParameterID::kSplitIVsInUnroller},
  {"-fsplit-wide-types",                   FlagParameterID::kSplitWideTypes},
  {"-fstrict-aliasing",                    FlagParameterID::kStrictAliasing},
  {"-fthread-jumps",                       FlagParameterID::kThreadJumps},
  {"-ftoplevel-reorder",                   FlagParameterID::kTopLevelReorder},
  {"-ftree-bit-ccp",                       FlagParameterID::kTreeBitCCP},
  {"-ftree-builtin-call-dce",              FlagParameterID::kTreeBuiltinCallDCE},
  {"-ftree-ccp",                           FlagParameterID::kTreeCCP},
  {"-ftree-ch",                            FlagParameterID::kTreeCH},
  {"-ftree-coalesce-inlined-vars",         FlagParameterID::kTreeCoalesceInlinedVars},
  {"-ftree-coalesce-vars",                 FlagParameterID::kTreeCoalesceVars},
  {"-ftree-copy-prop",                     FlagParameterID::kTreeCopyProp},
  {"-ftree-copyrename",                    FlagParameterID::kTreeCopyRename},
  {"-ftree-cselim",                        FlagParameterID::kTreeCSEElim},
  {"-ftree-dce",                           FlagParameterID::kTreeDCE},
  {"-ftree-dominator-opts",                FlagParameterID::kTreeDominatorOpts},
  {"-ftree-dse",                           FlagParameterID::kTreeDSE},
  {"-ftree-forwprop",                      FlagParameterID::kTreeForwProp},
  {"-ftree-fre",                           FlagParameterID::kTreeFRE},
  {"-ftree-loop-distribute-patterns",      FlagParameterID::kTreeLoopDistributePatterns},
  {"-ftree-loop-distribution",             FlagParameterID::kTreeLoopDistribution},
  {"-ftree-loop-if-convert",               FlagParameterID::kTreeLoopIfConvert},
  {"-ftree-loop-im",                       FlagParameterID::kTreeLoopIM},
  {"-ftree-loop-ivcanon",                  FlagParameterID::kTreeLoopIVCanon},
  {"-ftree-loop-optimize",                 FlagParameterID::kTreeLoopOptimize},
  {"-ftree-partial-pre",                   FlagParameterID::kTreePartialPre},
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
  {"-ftree-tail-merge",                    FlagParameterID::kTreeTailMerge},
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
};


// Print the help output string
static void printHelp()
{
  mageec::util::out() <<
"Usage: mageec-gcc [options]\n"
"       mageec-gcc gather database.db [options] --command gcc_command\n"
"       mageec-gcc optimize database.db [options] --command gcc_command\n"
"\n"
"MAGEEC driver for gcc. This is used to run a compilation which can interact\n"
"with the MAGEEC framework. This includes running training configurations of\n"
"the compiler, and using mageec to optimize an input program\n"
"\n"
"This is used after the feature extractor has already run over the input\n"
"files. The process of feature extraction produces a file containing an\n"
"identifier in the database for a set of features for each unit of the\n"
"compilation (functions/modules). This file is used as an input for this\n"
"driver.\n"
"\n"
"When gathering training data, a set of parameters are provided to the\n"
"driver. These are used to drive gcc, and when associated with the previously\n"
"extracted features, produce an identifier for the complete compilation of\n"
"each unit of the program. This identifier is output, and later on results\n"
"will be associated with it. Once that is done it forms a piece of complete\n"
"training data consisting of a set of features, a configuration of the\n"
"compiler used to transform the unit of the program, and a result value\n"
"indicating the effectiveness of the configuration.\n"
"\n"
"When optimizing, the previously extracted features are used with the\n"
"MAGEEC framework and a specified machine learner in order to materialize\n"
"a set of parameters to be provided to the compiler. The compiler is then\n"
"compiled with this configuration to produce a more optimal result.\n"
"\n"
"Basic options:\n"
"  --help                  Print this help information\n"
"  --version               Print out the version of this driver\n"
"  --database-version      Print the version of the provided database\n"
"  --framework-version     Print the version of the MAGEEC framework\n"
"  --debug                 Enable debug output\n"
"  --print-params          Print the parameters which may be specified with\n"
"                          this driver\n"
"  --config <file>         File containing feature group identifiers, and\n"
"                          to output compilation ids to\n"
"  --command <gcc_command> Specifies the compilation command to be run\n"
"\n"
"Gather options:\n"
"\n"
"  --params <param_list>   Comma seperated list of parameters specifying the\n"
"                          configuration of the compiler. These are just gcc\n"
"                          optimization flags\n"
"\n"
"Optimization options:\n"
"\n"
"  --ml <machine_learner>  UUID or shared object identifying the machine\n"
"                          learner to be used\n"
"  --metric <name>         Metric to optimize for\n"
"\n"
"Examples:\n"
"  mageec-gcc --help --version\n"
"  mageec-gcc gather foo.db\n"
"             --config config.csv\n"
"             --params -falign-loops,-fpeephole\n"
"             --command gcc --std=c99 foo.c bar.c\n"
"  mageec-gcc optimize foo.db\n"
"             --ml deadbeef-ca75-4096-a935-15cabba9e5\n"
"             --metric code-size\n"
"             --command gcc --std=c99 foo.c bar.c\n";
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
  mageec::util::out() << MAGEEC_PREFIX "Database version: " <<
      static_cast<std::string>(db->getVersion()) << '\n';
  return 0;
}

// Print the version of the framework that the driver was compiled with.
static void printFrameworkVersion(mageec::Framework &framework) {
  mageec::util::out() << MAGEEC_PREFIX "Framework version: " <<
      static_cast<std::string>(framework.getVersion()) << '\n';
}

// Print a list of all of the available parameters that can be used with
// this driver.
static void printParameters() {
  mageec::util::out() << MAGEEC_PREFIX "Available parameters:\n";
  for (const auto &param : flag_to_parameter)
    mageec::util::out() << param.first << "\n";
}

// Compile a set of input source files using the provided set of parameters.
// Generate a identifier for the compilation of each unit of the program
// (module/function), which associates the features previous extracted with
// the configuration of the compiler specified by the given parameters.
static int gather(mageec::Framework &framework,
                  std::string db_str,
                  std::string config_path,
                  std::vector<std::string> param_list,
                  std::string command_str) {
  (void)framework;
  (void)db_str;
  (void)config_path;
  (void)param_list;
  (void)command_str;
  return 0;
}

// Optimize the compilation of the input program. Use the set of features
// extracted in a previous step, along with the provided machine learner and
// the mageec framework, to make a prediction for a good configuration for
// each unit of source of the compilation.
static int optimize(mageec::Framework &framework,
                    std::string db_str,
                    std::string config_path,
                    std::string ml_str,
                    std::string metric_str,
                    std::string command_str) {
  (void)framework;
  (void)db_str;
  (void)config_path;
  (void)ml_str;
  (void)metric_str;
  (void)command_str;
  return 0;
}


static mageec::util::Option<std::vector<std::string>>
parseParameterList(std::string arg) {
  std::vector<std::string> param_list;

  {
    std::string param;
    for (unsigned i = 0; i < arg.size(); ++i) {

      char c = arg[i];
      if (c == ',') {
        param_list.push_back(param);
        param.clear();
      } else {
        param.push_back(c);
      }
    }
    param_list.push_back(param);
  }

  for (auto param : param_list) {
    if (flag_to_parameter.count(param) == 0) {
      MAGEEC_ERR("Unknown parameter in parameter list '" << param << "'\n");
      return nullptr;
    }
  }
  return param_list;
}

int main(int argc, const char *argv[]) {
  DriverMode mode = DriverMode::kNone;

  // The database to gather into, or optimize using
  std::string db_str;
  // Config file holding the features for the input program, and which will
  // hold the compilation ids for the compiled program.
  std::string config_path;
  // Params which this compilation will be compiled with
  std::vector<std::string> param_list;
  // The machine learner to optimize with
  std::string ml_str;
  // The metric to use when optimizing
  std::string metric_str;
  // The command which will be executed in order to compile all of the
  // source files
  std::string command_str;

  bool with_db                = false;
  bool with_help              = false;
  bool with_version           = false;
  bool with_db_version        = false;
  bool with_framework_version = false;
  bool with_debug             = false;
  bool with_print_params      = false;
  bool with_config            = false;
  bool with_params            = false;
  bool with_ml                = false;
  bool with_metric            = false;
  bool with_command           = false;

  for (int i = 0; i < argc; ++i) {
    if (i == 0)
      continue;
    std::string arg = argv[i];

    // The first argument may specify the mode if it's not a flag
    if (i == 1 && arg[0] != '-') {
      if (arg == "gather") {
        mode = DriverMode::kGather;
        continue;
      } else if (arg == "optimize") {
        mode = DriverMode::kOptimize;
        continue;
      } else {
        MAGEEC_ERR("Unknown mode: '" << arg << "'");
        return -1;
      }
    }

    // If we have a mode, the second flag should specify the database
    if (mode != DriverMode::kNone && i == 2) {
      if (arg[0] != '-') {
        db_str = arg;
        with_db = true;
      } else {
        MAGEEC_ERR("Mode specified, but no database provided");
        return -1;
      }
    }

    // Common flags
    if (arg == "--help") {
      with_help = true;
    } else if (arg == "--version") {
      with_version = true;
    } else if (arg == "--framework-version") {
      with_framework_version = true;
    } else if (arg == "--debug") {
      with_debug = true;
    } else if (arg == "--print-params") {
      with_print_params = true;
    }

    else if (arg == "--config") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--config' file provided");
        return -1;
      }
      config_path = arg;
      with_config = true;
    } else if (arg == "--params") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--params' parameter list provided");
        return -1;
      }
      auto tmp_param_list = parseParameterList(arg);
      if (!tmp_param_list) {
        MAGEEC_ERR("Invalid parameter in argument to '--params'");
        return -1;
      }
      param_list = tmp_param_list.get();
      with_params = true;
    } else if (arg == "--ml") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--ml' value provided");
        return -1;
      }
      ml_str = arg;
      with_ml = true;
    } else if (arg == "--metric") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--metric' value provided");
        return -1;
      }
      metric_str = arg;
      with_metric = true;
    } else if (arg == "--command") {
      ++i;
      if (i >= argc) {
        MAGEEC_ERR("No '--command' gcc command provided");
        return -1;
      }

      // accumulate the remaining args into the command buffer.
      std::string command_acc = argv[i];
      for (++i; i < argc; ++i)
        command_acc += argv[i];

      command_str = command_acc;
      with_command = true;
    } else {
      MAGEEC_ERR("Unrecognized argument: '" << arg << "'");
      return -1;
    }
  }

  // Errors
  bool have_error = false;
  if (mode == DriverMode::kOptimize) {
    assert(with_db); // this was checked earlier

    if (!with_config) {
      MAGEEC_ERR("Optimize mode specified without a configuration");
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
    if (!with_command) {
      MAGEEC_ERR("Optimize mode specified without a command to run");
      have_error = true;
    }
  } else if (mode == DriverMode::kGather) {
    assert(with_db);

    if (!with_config) {
      MAGEEC_ERR("Gather mode specified without a configuration");
      have_error = true;
    }
    if (!with_command) {
      MAGEEC_ERR("Gather mode specified without a command");
      have_error = true;
    }
  }
  if (have_error)
    return -1;

  // Warnings
  if (mode == DriverMode::kOptimize) {
    if (with_params)
      MAGEEC_WARN("--params argument will be ignored for the current mode");
  }
  if (mode == DriverMode::kGather) {
    if (with_ml)
      MAGEEC_WARN("--ml argument will be ignored for the current mode");
    if (with_metric)
      MAGEEC_WARN("--metric argument will be ignored for the current mode");
  }

  // Initialize the framework, and register some builtin machine learners so
  // they can be selected by UUID by the user
  mageec::Framework framework(with_debug);

  // C5 classifier
  MAGEEC_DEBUG("Registering C5.0 machine learner interface");
  std::unique_ptr<mageec::IMachineLearner> c5_ml(new mageec::C5Driver());
  framework.registerMachineLearner(std::move(c5_ml));

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
  if (with_print_params)
    printParameters();

  // Handle modes
  switch (mode) {
  case DriverMode::kNone:
    return 0;
  case DriverMode::kGather:
    return gather(framework, db_str, config_path, param_list, command_str);
  case DriverMode::kOptimize:
    return optimize(framework, db_str, config_path, ml_str, metric_str,
                    command_str);
  }
  return 0;
}
