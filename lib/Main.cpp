#include <iostream>

#include "mageec/Database.h"
#include "mageec/Feature.h"
#include "mageec/Framework.h"
#include "mageec/Types.h"
#include "mageec/Util.h"


int main(void)
{
  mageec::Framework framework;
  mageec::Database db = framework.getDatabase("tmp", true);

#ifdef MAGEEC_DEBUG
  std::cout << std::string(db.getVersion()) << std::endl;
#endif // MAGEEC_DEBUG

  // Emulate some feature extraction
  std::vector<mageec::FeatureBase*> func_features = {
    new mageec::IntFeature(1, 15, "function basic block count"),
    new mageec::IntFeature(2, 26, "function instruction count"),
    new mageec::BoolFeature(3, true, "returns void")
  };

  std::vector<mageec::FeatureBase*> module_features = {
    new mageec::IntFeature(11, 2, "module function count"),
    new mageec::IntFeature(12, 5, "number of globals in module")
  };

  mageec::FeatureSetID func_feature_set = db.newFeatureSet(func_features);
  mageec::FeatureSetID module_feature_set = db.newFeatureSet(module_features);

  // Setup feature groups. The module has its feature group, which is also
  // inherited by the function.
  mageec::FeatureGroupID func_feature_group =
    db.newFeatureGroup({module_feature_set, func_feature_set});
  mageec::FeatureGroupID module_feature_group =
    db.newFeatureGroup({module_feature_set});

  // Setup the global parameters used for the compilation
  // These are shared by the module and function compilations
  std::vector<mageec::ParameterBase*> params = {
    new mageec::BoolParameter(1, true, "debug enabled"),
    new mageec::RangeParameter(2, 2, "optimization level"),
    new mageec::RangeParameter(3, 5, "global inline threshold")
  };
  mageec::ParameterSetID parameter_set = db.newParameterSet(params);

  // Create a single pass sequence shared by the module and function
  mageec::PassSequenceID pass_sequence = db.newPassSequence();

  // Create compilations for the module and function
  mageec::CompilationID module_compilation = db.newCompilation(
    "hello.c", "module", module_feature_group, parameter_set,
    mageec::util::Option<mageec::PassSequenceID>(pass_sequence),
    mageec::util::Option<mageec::CompilationID>()
  );

  mageec::CompilationID func_compilation = db.newCompilation(
    "foo()", "function", func_feature_group, parameter_set,
    mageec::util::Option<mageec::PassSequenceID>(pass_sequence),
    module_compilation
  );

  // Create a parameter set for the inline threshold
  std::vector<mageec::ParameterBase*> inline_params = {
    new mageec::RangeParameter(5, 1, "pass inline threshold")
  };
  mageec::ParameterSetID inline_param_set = db.newParameterSet(inline_params);

  // Add a few passes to the pass sequence
  mageec::PassID dce = db.addPass("Dead code elim", pass_sequence);
  db.addPass("Common subexpression elim", pass_sequence);
  db.addPass("Register resurrection", pass_sequence);
  db.addPass("Inliner", pass_sequence,
    mageec::util::Option<mageec::ParameterSetID>(inline_param_set));

  // Module features were unchanged after dead code elimination
  db.addFeaturesAfterPass(module_feature_group, module_compilation, dce);

  // Add results for the now completed compilation
  db.addResult(func_compilation, mageec::Metric::kCodeSize, 12345);

  return 0;
}
