#include <iostream>

#include "mageec/Database.h"
#include "mageec/Framework.h"
#include "mageec/Util.h"


int main(void)
{
  mageec::Framework framework;
  mageec::Database db = framework.getDatabase("tmp.db", true);

#ifdef MAGEEC_DEBUG
  std::cout << std::string(db.getVersion()) << std::endl;
#endif // MAGEEC_DEBUG

  return 0;
}
