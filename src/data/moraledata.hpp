#pragma once

#include <algorithm>
#include <string>
#include <vector>

struct PlayerMorale {
  int playerID = 0;
  float moraleScore = 50.0f;  // 0–100
  std::string reason;
};

struct TeamChemistry {
  int teamID = 0;
  float chemistryScore = 50.0f;  // 0–100, derived from player morales
};

class MoraleManager {
 public:
  MoraleManager() = default;

  void SetMorale(int playerID, float score, const std::string& reason = "");
  float GetMorale(int playerID) const;

  // Returns a performance multiplier in [0.85, 1.15]:
  //   morale 0  -> 0.85x
  //   morale 50 -> 1.00x
  //   morale 100-> 1.15x
  float GetPerformanceMultiplier(int playerID) const;

  void UpdateAfterMatch(int playerID, bool won, bool drew);
  void UpdateAfterTransfer(int playerID, bool playerRequested);

  TeamChemistry ComputeTeamChemistry(int teamID,
                                     const std::vector<int>& playerIDs) const;

  const std::vector<PlayerMorale>& GetAllMorales() const { return m_morales; }

 private:
  std::vector<PlayerMorale> m_morales;

  PlayerMorale* FindOrCreate(int playerID);
  const PlayerMorale* Find(int playerID) const;

  static float Clamp(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
  }
};
