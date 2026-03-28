#include "facilitydata.hpp"

#include <numeric>

FacilityManager::FacilityManager() {
  auto make = [](FacilityType t, const std::string& bonus) {
    Facility f;
    f.type = t;
    f.level = 1;
    f.upgradeCost = UpgradeCostForLevel(1);
    f.weeklyMaintenance = MaintenanceForLevel(1);
    f.bonusDescription = bonus;
    return f;
  };
  m_facilities = {
      make(FacilityType::TRAINING_GROUND, "Improves player attribute growth rate"),
      make(FacilityType::STADIUM, "Increases match day revenue and crowd atmosphere"),
      make(FacilityType::MEDICAL_CENTER, "Reduces injury duration for players"),
      make(FacilityType::YOUTH_ACADEMY_FACILITY, "Increases youth prospect quality"),
      make(FacilityType::SCOUTING_CENTER, "Improves scout report accuracy"),
  };
}

Facility* FacilityManager::Find(FacilityType type) {
  for (auto& f : m_facilities) {
    if (f.type == type) return &f;
  }
  return nullptr;
}

const Facility* FacilityManager::Find(FacilityType type) const {
  for (const auto& f : m_facilities) {
    if (f.type == type) return &f;
  }
  return nullptr;
}

bool FacilityManager::Upgrade(FacilityType type) {
  Facility* f = Find(type);
  if (!f || f->level >= 5) return false;
  f->level++;
  f->upgradeCost = UpgradeCostForLevel(f->level);
  f->weeklyMaintenance = MaintenanceForLevel(f->level);
  return true;
}

int FacilityManager::GetLevel(FacilityType type) const {
  const Facility* f = Find(type);
  return f ? f->level : 1;
}

float FacilityManager::GetBonus(FacilityType type) const {
  int level = GetLevel(type);
  // level 1 -> 1.0x, level 5 -> 1.5x, linear
  return 1.0f + (static_cast<float>(level - 1) / 4.0f) * 0.50f;
}

int FacilityManager::GetTotalWeeklyMaintenance() const {
  int total = 0;
  for (const auto& f : m_facilities) total += f.weeklyMaintenance;
  return total;
}

int FacilityManager::UpgradeCostForLevel(int currentLevel) {
  // Escalating costs: L1->L2=500k, L2->L3=1M, L3->L4=2M, L4->L5=4M
  static const int costs[] = {0, 500000, 1000000, 2000000, 4000000};
  if (currentLevel < 1 || currentLevel > 4) return 0;
  return costs[currentLevel];
}

int FacilityManager::MaintenanceForLevel(int level) {
  // 5k per level per week
  return level * 5000;
}
