#include "staffdata.hpp"

#include <algorithm>
#include <numeric>

int StaffManager::HireStaff(const StaffMember& member) {
  StaffMember m = member;
  m.staffID = m_nextStaffID++;
  m_staff.push_back(m);
  return m.staffID;
}

void StaffManager::FireStaff(int staffID) {
  m_staff.erase(std::remove_if(m_staff.begin(), m_staff.end(),
                               [staffID](const StaffMember& m) { return m.staffID == staffID; }),
                m_staff.end());
}

StaffMember* StaffManager::GetStaff(int staffID) {
  for (auto& m : m_staff) {
    if (m.staffID == staffID) return &m;
  }
  return nullptr;
}

std::vector<StaffMember> StaffManager::GetStaffForTeam(int teamID) const {
  std::vector<StaffMember> result;
  for (const auto& m : m_staff) {
    if (m.teamID == teamID) result.push_back(m);
  }
  return result;
}

float StaffManager::GetStaffBonus(int teamID, StaffRole role) const {
  int bestSkill = 0;
  for (const auto& m : m_staff) {
    if (m.teamID == teamID && m.role == role && m.skill > bestSkill) bestSkill = m.skill;
  }
  // skill 0 -> 1.0x, skill 100 -> 1.2x
  return 1.0f + (static_cast<float>(bestSkill) / 100.0f) * 0.20f;
}

int StaffManager::GetTotalWages(int teamID) const {
  int total = 0;
  for (const auto& m : m_staff) {
    if (m.teamID == teamID) total += m.salary;
  }
  return total;
}
