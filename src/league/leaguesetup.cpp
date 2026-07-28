#include "leaguesetup.hpp"

#include "base/utils.hpp"

namespace {

static const char* kLeagueNames[] = {
  "Premier Division",
  "Championship",
  "League One",
  "League Two"
};

static const char* kTeamNames[4][8] = {
  {
    "Thunder FC", "Storm United", "Iron City", "Royal Athletic",
    "Crestwood Town", "Harbor Rovers", "Summit FC", "Valley Stars"
  },
  {
    "Oakwood FC", "Riverside United", "Granite City", "Phoenix Athletic",
    "Northgate Town", "Seabird Rovers", "Highland FC", "Meadow Stars"
  },
  {
    "Ashford FC", "Brookside United", "Cliff City", "Dunmore Athletic",
    "Eastfield Town", "Fairview Rovers", "Glenwood FC", "Hawthorn Stars"
  },
  {
    "Ivybridge FC", "Kingsley United", "Larkspur City", "Millbrook Athletic",
    "Newbridge Town", "Orchard Rovers", "Parkside FC", "Redwood Stars"
  }
};

static const char* kShortNames[4][8] = {
  { "THU", "STO", "IRO", "ROY", "CRE", "HAR", "SUM", "VAL" },
  { "OAK", "RIV", "GRA", "PHO", "NOR", "SEA", "HIG", "MEA" },
  { "ASH", "BRO", "CLI", "DUN", "EAS", "FAI", "GLE", "HAW" },
  { "IVY", "KIN", "LAR", "MIL", "NEW", "ORC", "PAR", "RED" }
};

static const char* kColors1[4][8] = {
  { "#FF0000", "#0000FF", "#008000", "#800080", "#FFA500", "#008080", "#800000", "#000080" },
  { "#800080", "#FFA500", "#008080", "#800000", "#000080", "#FF0000", "#0000FF", "#008000" },
  { "#000080", "#800000", "#008080", "#FFA500", "#800080", "#008000", "#0000FF", "#FF0000" },
  { "#008000", "#0000FF", "#FF0000", "#800080", "#000080", "#FFA500", "#800000", "#008080" }
};

static const char* kColors2[4][8] = {
  { "#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF" },
  { "#000000", "#000000", "#000000", "#000000", "#000000", "#000000", "#000000", "#000000" },
  { "#FFFF00", "#FFFF00", "#FFFF00", "#FFFF00", "#FFFF00", "#FFFF00", "#FFFF00", "#FFFF00" },
  { "#000000", "#FFFFFF", "#000000", "#FFFFFF", "#FFFF00", "#000000", "#FFFFFF", "#FFFF00" }
};

static const char* kFirstNames[] = {
  "Marcus", "Youssef", "Liam", "Jorge", "Tom", "Henrik", "Raj", "Pierre",
  "Diego", "Aaron", "Viktor", "Ali", "Brandon", "Erik", "Sergio", "Niko",
  "Julian", "Kwame", "Rafael", "Lukas", "Dante", "Callum", "Oscar", "Enzo"
};

static const char* kLastNames[] = {
  "Thompson", "Al-Rashid", "Novak", "Cruz", "Mueller", "Patel", "Laurent",
  "Sanchez", "Chang", "Kowalski", "Bakker", "Moreau", "Lindberg", "Hayes",
  "Petrov", "Fernandez", "Kim", "Dubois", "Torres", "Osei", "Berg", "Sato"
};

static const char* kRoles[] = {
  "GK", "GK", "LB", "RB", "CB", "CB", "CB", "DM", "DM", "LM", "RM", "CM", "CM", "CM", "AM", "AM", "CF", "CF", "CF", "ST"
};

void GeneratePlayersForTeam(Database* db, int teamID) {
  std::string defaultStats = "<defense>0.6</defense><bodybalance>0.6</bodybalance><acceleration>0.6</acceleration><velocity>0.6</velocity><agility>0.6</agility><reaction>0.6</reaction><ballcontrol>0.6</ballcontrol><dribblevelocity>0.6</dribblevelocity><shotpower>0.6</shotpower><passtechnique>0.6</passtechnique><tactics>0.6</tactics><condition>1.0</condition>";
  
  for (int i = 0; i < 20; i++) {
    std::string fname = kFirstNames[rand() % 24];
    std::string lname = kLastNames[rand() % 23];
    std::string role = kRoles[i];
    int age = 18 + (rand() % 15);
    float baseStat = 0.5f + (static_cast<float>(rand() % 40) / 100.0f);

    db->Query(
        "INSERT INTO players (team_id, nationalteam_id, firstname, lastname, role, age, base_stat, profile_xml, skincolor, hairstyle, haircolor, height, weight, formationorder) "
        "VALUES (" + int_to_str(teamID) + ", NULL, '" + fname + "', '" + lname + "', '" + role + "', " + int_to_str(age) + ", " + real_to_str(baseStat) + ", '" + defaultStats + "', " + int_to_str(rand() % 4) + ", 'short', 'brown', 1.80, 75.0, " + int_to_str(i) + ")");
  }
}

}

void SetupFourLeagues(Database* db) {
  if (!db) return;

  db->Query(
      "CREATE TABLE IF NOT EXISTS leagues(id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "country_id INTEGER, name VARCHAR(64), logo_url VARCHAR(512));");
  db->Query(
      "CREATE TABLE IF NOT EXISTS teams(id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "league_id INTEGER, name VARCHAR(64), logo_url VARCHAR(512), kit_url VARCHAR(512), "
      "formation_xml TEXT, formation_factory_xml TEXT, tactics_xml TEXT, "
      "tactics_factory_xml TEXT, shortname VARCHAR(3), color1 VARCHAR(16), color2 VARCHAR(16));");
  db->Query(
      "CREATE TABLE IF NOT EXISTS players(id INTEGER PRIMARY KEY AUTOINCREMENT, "
      "team_id INTEGER, nationalteam_id INTEGER, firstname VARCHAR(64), lastname VARCHAR(64), "
      "role VARCHAR(32), age INTEGER, base_stat FLOAT, profile_xml TEXT, skincolor INTEGER, "
      "hairstyle VARCHAR(64), haircolor VARCHAR(64), height FLOAT, weight FLOAT, "
      "formationorder INTEGER, nationalteamformationorder INTEGER);");

  std::string defaultFormation = "<p1><position>-1,0</position><role>GK</role></p1><p2><position>-0.6,0.6</position><role>LB</role></p2><p3><position>-0.7,0.2</position><role>CB</role></p3><p4><position>-0.7,-0.2</position><role>CB</role></p4><p5><position>-0.6,-0.6</position><role>RB</role></p5><p6><position>0.0,0.8</position><role>LM</role></p6><p7><position>0.1,0.0</position><role>AM</role></p7><p8><position>0.0,-0.8</position><role>RM</role></p8><p9><position>0.7,0.5</position><role>CF</role></p9><p10><position>0.8,0.0</position><role>CF</role></p10><p11><position>0.7,-0.5</position><role>CF</role></p11>";
  std::string defaultTactics = "<dribble_centermagnet>0.5</dribble_centermagnet><dribble_offensiveness>0.5</dribble_offensiveness>";

  db->Query("DELETE FROM players");
  db->Query("DELETE FROM teams");
  db->Query("DELETE FROM leagues");

  for (int leagueIdx = 0; leagueIdx < 4; leagueIdx++) {
    db->Query(
        "INSERT INTO leagues (country_id, name, logo_url) "
        "VALUES (NULL, '" + std::string(kLeagueNames[leagueIdx]) + "', '')");

    auto leagueResult = db->Query("SELECT last_insert_rowid()");
    if (leagueResult->data.empty()) continue;
    int leagueID = atoi(leagueResult->data.at(0).at(0).c_str());

    for (int teamIdx = 0; teamIdx < 8; teamIdx++) {
      db->Query(
          "INSERT INTO teams (league_id, name, logo_url, kit_url, formation_xml, "
          "formation_factory_xml, tactics_xml, tactics_factory_xml, shortname, color1, color2) "
          "VALUES (" + int_to_str(leagueID) + ", "
          "'" + std::string(kTeamNames[leagueIdx][teamIdx]) + "', '', '', '" + defaultFormation + "', '" + defaultFormation + "', '" + defaultTactics + "', '" + defaultTactics + "', "
          "'" + std::string(kShortNames[leagueIdx][teamIdx]) + "', "
          "'" + std::string(kColors1[leagueIdx][teamIdx]) + "', "
          "'" + std::string(kColors2[leagueIdx][teamIdx]) + "')");
      
      auto teamResult = db->Query("SELECT last_insert_rowid()");
      if (!teamResult->data.empty()) {
        int teamID = atoi(teamResult->data.at(0).at(0).c_str());
        GeneratePlayersForTeam(db, teamID);
      }
    }
  }
}
