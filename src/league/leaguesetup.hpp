#ifndef _HPP_LEAGUESETUP
#define _HPP_LEAGUESETUP

#include "utils/database.hpp"

using namespace blunted;

struct LeagueSetupEntry {
  const char* leagueName;
  const char* teams[8];
  const char* shortNames[8];
  const char* color1[8];
  const char* color2[8];
};

void SetupFourLeagues(Database* db);

#endif
