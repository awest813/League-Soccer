#pragma once

#include <string>
#include <vector>

enum class FacilityType {
  TRAINING_GROUND,
  STADIUM,
  MEDICAL_CENTER,
  YOUTH_ACADEMY_FACILITY,
  SCOUTING_CENTER
};

struct Facility {
  FacilityType type = FacilityType::TRAINING_GROUND;
  int level = 1;              // 1–5
  int upgradeCost = 0;        // cost to reach next level
  int weeklyMaintenance = 0;
  std::string bonusDescription;
};

class FacilityManager {
 public:
  FacilityManager();

  bool Upgrade(FacilityType type);
  int GetLevel(FacilityType type) const;

  // Returns a bonus multiplier [1.0, 1.5] proportional to facility level
  float GetBonus(FacilityType type) const;

  int GetTotalWeeklyMaintenance() const;

  const std::vector<Facility>& GetFacilities() const { return m_facilities; }

 private:
  std::vector<Facility> m_facilities;

  Facility* Find(FacilityType type);
  const Facility* Find(FacilityType type) const;

  static int UpgradeCostForLevel(int currentLevel);
  static int MaintenanceForLevel(int level);
};
