#include "career_staff.hpp"

#include <algorithm>

#include "career_common.hpp"

namespace blunted {
namespace CareerStaff {

namespace {
using CareerCommon::RandomInt;
}  // namespace

void HireStaff(CareerSave& save, const StaffMember& member) {
  if (member.salary > save.finances.netWorth)
    return;
  save.staff.push_back(member);
  save.finances.netWorth -= member.salary;
  save.finances.staffWages += member.salary;
}

void FireStaff(CareerSave& save, CareerCommon::CareerEvents& events, const std::string& staffName) {
  auto it =
      std::find_if(save.staff.begin(), save.staff.end(),
                   [&staffName](const StaffMember& member) { return member.name == staffName; });
  if (it == save.staff.end())
    return;
  save.finances.staffWages = std::max(0LL, save.finances.staffWages - it->salary);
  save.staff.erase(it);
  events.ModifyBoardConfidence(-1);
}

void GenerateStaffCandidates(std::vector<StaffMember>& candidates) {
  candidates.clear();
  static const std::vector<std::string> firstNames = {
      "Jordan", "Sofia",  "Callum", "Kei",   "Marta",  "Henrik", "Lena",
      "Omar",   "Priya",  "Diego",  "Alina", "Samuel", "Yuki",   "Fabio",
      "Rosa",   "Thomas", "Aisha",  "Liam",  "Nadia",  "Andre",  "Clara"};
  static const std::vector<std::string> lastNames = {
      "Blake",  "Marin",     "Hart",    "Tanaka",    "Costa",  "Lindqvist", "Petrov", "Ali",
      "Sharma", "Fernandez", "Novak",   "Eriksson",  "Rossi",  "Muller",    "Chen",   "Dubois",
      "Park",   "Santos",    "Fischer", "Johansson", "Moreau", "Torres"};
  static const std::vector<std::string> roles = {
      "Assistant Coach", "Head Scout",       "Fitness Coach", "Goalkeeping Coach",
      "Physio",          "Tactical Analyst", "Youth Coach",   "Set Piece Specialist"};

  std::vector<int> usedIndices;
  for (int i = 0; i < 5; ++i) {
    int nameIdx;
    do {
      nameIdx = RandomInt(0, static_cast<int>(firstNames.size()) - 1);
    } while (std::find(usedIndices.begin(), usedIndices.end(), nameIdx) != usedIndices.end());
    usedIndices.push_back(nameIdx);
    int roleIdx = RandomInt(0, static_cast<int>(roles.size()) - 1);
    candidates.push_back(StaffMember(firstNames[nameIdx] + " " + lastNames[nameIdx], roles[roleIdx],
                                     RandomInt(58, 88), RandomInt(500000, 1500000),
                                     RandomInt(2, 4)));
  }
}

}  // namespace CareerStaff
}  // namespace blunted
