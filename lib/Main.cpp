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

  // Create compilations for the module and function
  mageec::CompilationID module_compilation = db.newCompilation(
    "hello.c", "module", module_feature_group, parameter_set,
    mageec::util::Option<mageec::PassSequenceID>(),
    mageec::util::Option<mageec::CompilationID>()
  );

  db.newCompilation(
    "foo()", "function", func_feature_group, parameter_set,
    mageec::util::Option<mageec::PassSequenceID>(),
    module_compilation
  );

  return 0;
}
