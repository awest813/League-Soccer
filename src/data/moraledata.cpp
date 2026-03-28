#include "moraledata.hpp"

#include <numeric>

PlayerMorale* MoraleManager::FindOrCreate(int playerID) {
  for (auto& m : m_morales) {
    if (m.playerID == playerID) return &m;
  }
  m_morales.push_back({playerID, 50.0f, ""});
  return &m_morales.back();
}

const PlayerMorale* MoraleManager::Find(int playerID) const {
  for (const auto& m : m_morales) {
    if (m.playerID == playerID) return &m;
  }
  return nullptr;
}

void MoraleManager::SetMorale(int playerID, float score, const std::string& reason) {
  PlayerMorale* m = FindOrCreate(playerID);
  m->moraleScore = Clamp(score, 0.0f, 100.0f);
  if (!reason.empty()) m->reason = reason;
}

float MoraleManager::GetMorale(int playerID) const {
  const PlayerMorale* m = Find(playerID);
  return m ? m->moraleScore : 50.0f;
}

float MoraleManager::GetPerformanceMultiplier(int playerID) const {
  float morale = GetMorale(playerID);  // 0–100
  return 0.85f + (morale / 100.0f) * 0.30f;
}

void MoraleManager::UpdateAfterMatch(int playerID, bool won, bool drew) {
  float delta = won ? 5.0f : (drew ? 0.0f : -5.0f);
  PlayerMorale* m = FindOrCreate(playerID);
  m->moraleScore = Clamp(m->moraleScore + delta, 0.0f, 100.0f);
  m->reason = won ? "Won match" : (drew ? "Drew match" : "Lost match");
}

void MoraleManager::UpdateAfterTransfer(int playerID, bool playerRequested) {
  PlayerMorale* m = FindOrCreate(playerID);
  float delta = playerRequested ? -10.0f : 5.0f;
  m->moraleScore = Clamp(m->moraleScore + delta, 0.0f, 100.0f);
  m->reason = playerRequested ? "Unhappy with transfer" : "Happy with move";
}

TeamChemistry MoraleManager::ComputeTeamChemistry(int teamID,
                                                   const std::vector<int>& playerIDs) const {
  TeamChemistry tc;
  tc.teamID = teamID;
  if (playerIDs.empty()) {
    tc.chemistryScore = 50.0f;
    return tc;
  }
  float sum = 0.0f;
  for (int id : playerIDs) sum += GetMorale(id);
  tc.chemistryScore = sum / static_cast<float>(playerIDs.size());
  return tc;
}
