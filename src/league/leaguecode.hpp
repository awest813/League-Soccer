#ifndef _HPP_LEAGUECODE
#define _HPP_LEAGUECODE

#include <string>

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

// A single calendar fixture, enriched with the names the UI and the match
// simulator need. team1 is always the home side, mirroring the calendar table.
struct LeagueFixtureInfo {
  int calendarID = 0;
  int homeTeamID = 0;
  int awayTeamID = 0;
  int competitionID = 0;
  std::string homeTeamName;
  std::string awayTeamName;
  std::string competitionName;
  std::string date;  // YYYY-MM-DD
};

// The fixture the user has kicked off in the 3D match engine and that is
// waiting for its result to be written back after full-time. Only one can be
// armed at a time; it is cleared on forfeit, on returning to the main menu,
// and when its result is recorded.
bool LeagueHasPendingFixture();
const LeagueFixtureInfo& LeagueGetPendingFixture();
void LeagueSetPendingFixture(const LeagueFixtureInfo& fixture);
void LeagueClearPendingFixture();

bool LeagueGetUserTeamID(int& teamID);
// Resolves a team's logo file path against the active save directory
// (mirroring TeamData). Returns an empty string when the team has no logo on
// disk; callers should fall back to a placeholder image.
std::string LeagueResolveLogoPath(int teamID);
// The user's earliest unplayed fixture, or false when the season is done.
bool LeagueGetUserNextFixture(LeagueFixtureInfo& out);
// Loads any fixture by its calendar row id.
bool LeagueGetFixtureByCalendarID(int calendarID, LeagueFixtureInfo& out);
// Earliest date that still has unplayed fixtures.
bool LeagueGetNextMatchdayDate(std::string& outDate);

// Writes the score into match_results (replacing any previous entry for the
// fixture) and files an inbox message. Does not touch the clock.
void LeagueRecordResult(const LeagueFixtureInfo& fixture, int homeGoals, int awayGoals);

// Hash-based strength model shared by every simulated fixture.
void LeagueSimulateFixture(const LeagueFixtureInfo& fixture, int& homeGoals, int& awayGoals);

// Simulates every unplayed fixture scheduled on or before the given date,
// moves the current date to it and remembers it as the last resolved matchday.
// Returns the number of fixtures simulated.
int LeagueResolveMatchday(const std::string& date);
std::string LeagueGetLastMatchdayDate();

// True when the calendar has no unplayed fixtures left.
bool LeagueSeasonComplete();
// Rolls into the next season: bumps the season year, resets the clock to
// June 1st, archives the old results and regenerates the calendar.
bool LeagueAdvanceSeason();

// Named save slots: snapshots of the live autosave database, stored inside
// the save directory as slot_<n>.sqlite.
bool LeagueSaveToSlot(int slotIndex);
bool LeagueLoadSlot(int slotIndex);
int LeagueGetSlotCount();
const int kLeagueMaxSaveSlots = 5;

// Squad size rules for the transfer/release UI.
const int kLeagueSquadMinSize = 13;
const int kLeagueSquadMaxSize = 26;

// Records the 3D match's final score for the pending fixture, resolves the
// rest of that matchday and clears the pending state. Returns false (leaving
// the pending fixture untouched) when no fixture is pending or it has already
// been played.
bool LeagueConsumePlayedFixture(int homeGoals, int awayGoals);

#endif
