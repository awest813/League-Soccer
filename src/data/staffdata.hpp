#pragma once

#include <string>
#include <vector>

enum class StaffRole {
  ASSISTANT_COACH,
  FITNESS_COACH,
  MEDICAL_STAFF,
  HEAD_SCOUT,
  GOALKEEPING_COACH
};

struct StaffMember {
  int staffID = 0;
  std::string name;
  StaffRole role = StaffRole::ASSISTANT_COACH;
  int skill = 50;   // 0–100
  int salary = 0;
  int teamID = 0;
};

class StaffManager {
 public:
  StaffManager() = default;

  int HireStaff(const StaffMember& member);
  void FireStaff(int staffID);
  StaffMember* GetStaff(int staffID);

  const std::vector<StaffMember>& GetAllStaff() const { return m_staff; }
  std::vector<StaffMember> GetStaffForTeam(int teamID) const;

  // Returns a bonus multiplier [1.0, 1.2] based on the best staff skill in
  // the given role for the team.  Defaults to 1.0 if no matching staff.
  float GetStaffBonus(int teamID, StaffRole role) const;

  int GetTotalWages(int teamID) const;

 private:
  std::vector<StaffMember> m_staff;
  int m_nextStaffID = 1;
};
