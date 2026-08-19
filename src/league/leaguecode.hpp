#ifndef _HPP_LEAGUECODE
#define _HPP_LEAGUECODE

#include "utils/database.hpp"

using namespace blunted;

// Declared (and, for the real game, defined) in main.hpp. Forward-declared
// here instead of including the full main.hpp, which transitively pulls in
// SDL2/OpenGL headers (via gametask.hpp/menutask.hpp/graphics_system.hpp)
// that headless/test-only builds don't have available.
Database* GetDB();
std::string GetActiveSaveDirectory();
void SetActiveSaveDirectory(const std::string& dir);

int CreateNewLeagueSave(const std::string& srcDbName, const std::string& saveName);
bool PrepareDatabaseForLeague();
bool SaveAutosaveToDatabase();
bool SaveDatabaseToAutosave();
bool LoadLeague();
void GenerateSeasonCalendars();
bool StepLeagueTime();

#endif
