#pragma once

#include <string>
#include <vector>

struct YouthPlayer {
  int youthID = 0;
  std::string firstName;
  std::string lastName;
  int age = 16;           // 14–18
  std::string position;
  int rawPotential = 0;   // 0–100, hidden until promoted
  int currentRating = 0;  // 0–100
  float developmentRate = 1.0f;
};

class YouthAcademy {
 public:
  explicit YouthAcademy(int teamID) : m_teamID(teamID) {}

  int TeamID() const { return m_teamID; }

  int AddProspect(const YouthPlayer& player);
  void RemoveProspect(int youthID);
  YouthPlayer* GetProspect(int youthID);

  const std::vector<YouthPlayer>& GetProspects() const { return m_prospects; }

  // Call at end of each season: age players by 1, improve ratings, remove those too old
  void DevelopPlayers();

  // Promote a youth player to the first team; returns a new PlayerData ID placeholder
  int PromoteToFirstTeam(int youthID);

 private:
  int m_teamID = 0;
  std::vector<YouthPlayer> m_prospects;
  int m_nextYouthID = 1;

  static float Clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
  }
};
