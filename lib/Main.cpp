#include <iostream>
#include <memory>

#include "mageec/Database.h"
#include "mageec/Feature.h"
#include "mageec/Framework.h"
#include "mageec/Types.h"
#include "mageec/Util.h"


using namespace mageec;


int main(void)
{
  Framework framework;
  std::unique_ptr<Database> db = framework.getDatabase("tmp", true);

#ifdef MAGEEC_DEBUG
  std::cout << std::string(db->getVersion()) << '\n';
#endif // MAGEEC_DEBUG

  // Emulate some feature extraction
  FeatureSet func_features = {
    std::make_shared<IntFeature>(1, 15, "function basic block count"),
    std::make_shared<IntFeature>(2, 26, "function instruction count"),
    std::make_shared<BoolFeature>(3, true, "returns void")
  };

  FeatureSet module_features = {
    std::make_shared<IntFeature>(11, 2, "module function count"),
    std::make_shared<IntFeature>(12, 5, "number of globals in module")
  };

  FeatureSetID func_feature_set = db->newFeatureSet(func_features);
  FeatureSetID module_feature_set = db->newFeatureSet(module_features);

  // Setup feature groups. The module has its feature group, which is also
  // inherited by the function.
  FeatureGroupID func_feature_group =
    db->newFeatureGroup({module_feature_set, func_feature_set});
  FeatureGroupID module_feature_group =
    db->newFeatureGroup({module_feature_set});

  // Setup the global parameters used for the compilation
  // These are shared by the module and function compilations
  ParameterSet params = {
    std::make_shared<BoolParameter>(1, true, "debug enabled"),
    std::make_shared<RangeParameter>(2, 2, "optimization level"),
    std::make_shared<RangeParameter>(3, 5, "global inline threshold")
  };
  ParameterSetID parameter_set = db->newParameterSet(params);

  // Create a single pass sequence shared by the module and function
  PassSequenceID pass_sequence = db->newPassSequence();

  // Create compilations for the module and function
  CompilationID module_compilation = db->newCompilation(
    "hello.c", "module", module_feature_group, parameter_set,
    util::Option<PassSequenceID>(pass_sequence),
    util::Option<CompilationID>()
  );

  CompilationID func_compilation = db->newCompilation(
    "foo()", "function", func_feature_group, parameter_set,
    util::Option<PassSequenceID>(pass_sequence),
    module_compilation
  );

  // Create a parameter set for the inline threshold
  ParameterSet inline_params = {
    std::make_shared<RangeParameter>(5, 1, "pass inline threshold")
  };
  ParameterSetID inline_param_set = db->newParameterSet(inline_params);

  // Add a few passes to the pass sequence
  PassID dce = db->addPass("Dead code elim", pass_sequence);
  db->addPass("Common subexpression elim", pass_sequence);
  db->addPass("Register resurrection", pass_sequence);
  db->addPass("Inliner", pass_sequence,
    util::Option<ParameterSetID>(inline_param_set));

  // Module features were unchanged after dead code elimination
  db->addFeaturesAfterPass(module_feature_group, module_compilation, dce);

  // Add results for the now completed compilation
  db->addResult(func_compilation, Metric::kCodeSize, 12345);

  return 0;
}
