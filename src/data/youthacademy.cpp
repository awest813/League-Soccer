#include "youthacademy.hpp"

#include <algorithm>

int YouthAcademy::AddProspect(const YouthPlayer& player) {
  YouthPlayer p = player;
  p.youthID = m_nextYouthID++;
  m_prospects.push_back(p);
  return p.youthID;
}

void YouthAcademy::RemoveProspect(int youthID) {
  m_prospects.erase(
      std::remove_if(m_prospects.begin(), m_prospects.end(),
                     [youthID](const YouthPlayer& p) { return p.youthID == youthID; }),
      m_prospects.end());
}

YouthPlayer* YouthAcademy::GetProspect(int youthID) {
  for (auto& p : m_prospects) {
    if (p.youthID == youthID) return &p;
  }
  return nullptr;
}

void YouthAcademy::DevelopPlayers() {
  std::vector<int> toRemove;
  for (auto& p : m_prospects) {
    p.age++;
    // Each season, rating grows towards potential proportional to developmentRate
    float gap = static_cast<float>(p.rawPotential - p.currentRating);
    float improvement = gap * 0.15f * p.developmentRate;
    p.currentRating =
        static_cast<int>(Clamp(static_cast<float>(p.currentRating) + improvement, 0.0f, 100.0f));

    // Players aged 21+ are released if not promoted
    if (p.age > 20) toRemove.push_back(p.youthID);
  }
  for (int id : toRemove) RemoveProspect(id);
}

int YouthAcademy::PromoteToFirstTeam(int youthID) {
  YouthPlayer* p = GetProspect(youthID);
  if (!p) return -1;
  // Return current rating as a placeholder new-player ID
  int promotedID = p->youthID;
  RemoveProspect(youthID);
  return promotedID;
}
