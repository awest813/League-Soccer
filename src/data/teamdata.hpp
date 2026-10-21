#ifndef _HPP_TEAMDATA
#define _HPP_TEAMDATA

#include <memory>
#include <string>
#include <vector>

#include "../gamedefines.hpp"
#include "base/properties.hpp"
#include "defines.hpp"
#include "playerdata.hpp"

struct TeamTactics {
  TeamTactics() {}

  Properties factoryProperties;
  Properties userProperties;

  Properties humanReadableNames;
  Properties descriptions;
};

// A squad that replaces a team's database squad for the next match (a career
// club fields its career roster). Database players are loaded as usual; the
// others are built from the career's stats.
struct SquadOverridePlayer {
  int databaseID = 0;  // unique within the squad; may be absent from the database
  std::string firstName;
  std::string lastName;
  std::string role = "CM";  // GK, CB, LB, RB, DM, CM, LM, RM, AM or CF
  float baseStat = 0.6f;    // 0-1
  int age = 25;
};

struct SquadOverride {
  int teamDatabaseID = 0;
  std::vector<SquadOverridePlayer> players;  // starting eleven first, then the bench
  // Keeps the team's human controller on this player (Player-mode careers).
  int lockedPlayerDatabaseID = 0;
};

// Applies to every TeamData built for the team until cleared.
void SetSquadOverride(const SquadOverride& squad);
void ClearSquadOverride();

class TeamData {
public:
  TeamData(int teamDatabaseID);
  virtual ~TeamData();

  std::string GetName() const { return name; }
  std::string GetShortName() const { return shortName; }
  std::string GetLogoUrl() const { return logo_url; }
  std::string GetKitUrl() const { return kit_url; }
  Vector3 GetColor1() const { return color1; }
  Vector3 GetColor2() const { return color2; }

  int GetDatabaseID() const { return databaseID; }

  const TeamTactics& GetTactics() const { return tactics; }
  TeamTactics& GetTacticsWritable() { return tactics; }

  FormationEntry GetFormationEntry(int num);
  void SetFormationEntry(int num, FormationEntry entry);

  void SwitchPlayers(int databaseID1, int databaseID2);

  // vector index# is entry in formation[index#]
  const std::vector<std::unique_ptr<PlayerData>>& GetPlayerData() { return playerData; }
  int GetPlayerNum() { return static_cast<int>(playerData.size()); }
  PlayerData* GetPlayerData(int num) { return playerData.at(num).get(); }
  PlayerData* GetPlayerDataByDatabaseID(int id);
  // Database ID of the player the human controller is locked to, or 0.
  int GetLockedPlayerDatabaseID() const { return lockedPlayerDatabaseID; }

  void SaveLineup();
  void SaveTactics();
  void Save();

protected:
  int databaseID;

  std::string name;
  std::string shortName;
  std::string logo_url;
  std::string kit_url;
  Vector3 color1, color2;

  TeamTactics tactics;

  FormationEntry formation[playerNum];

  std::vector<std::unique_ptr<PlayerData>> playerData;
  int lockedPlayerDatabaseID = 0;

private:
  bool LoadSquadOverride();
};

#endif
