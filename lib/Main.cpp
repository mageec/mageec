#include <iostream>

#include "mageec/Database.h"
#include "mageec/Feature.h"
#include "mageec/Framework.h"
#include "mageec/Util.h"


int main(void)
{
  mageec::Framework framework;
  mageec::Database db = framework.getDatabase("tmp", true);

#ifdef MAGEEC_DEBUG
  std::cout << std::string(db.getVersion()) << std::endl;
#endif // MAGEEC_DEBUG

  std::vector<mageec::FeatureBase*> fn_features;
  fn_features.push_back(new mageec::IntFeature(111, 8, "banana"));
  fn_features.push_back(new mageec::IntFeature(312, 15, "cabbage"));

  std::vector<mageec::FeatureBase*> mod_features;
  mod_features.push_back(new mageec::IntFeature(12, 1, "toffee"));
  mod_features.push_back(new mageec::IntFeature(11, 91, "albatross"));

  mageec::FeatureSetID fn_feature_set = db.newFeatureSet(fn_features);
  mageec::FeatureSetID mod_feature_set = db.newFeatureSet(mod_features);

  db.newFeatureGroup({fn_feature_set, mod_feature_set});

  std::vector<mageec::ParameterBase*> params;
  params.push_back(new mageec::RangeParameter(12, 91, "food"));
  params.push_back(new mageec::BoolParameter(43, 18, "potato"));
  db.newParameterSet(params);

  return 0;
}
