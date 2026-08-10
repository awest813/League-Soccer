#ifndef CAREER_STAFF_HPP
#define CAREER_STAFF_HPP

#include <vector>

#include "../../data/careerdata.hpp"
#include "career_common.hpp"

namespace blunted {
namespace CareerStaff {

void HireStaff(CareerSave& save, const StaffMember& member);
void FireStaff(CareerSave& save, CareerCommon::CareerEvents& events, const std::string& staffName);
void GenerateStaffCandidates(std::vector<StaffMember>& candidates);

}  // namespace CareerStaff
}  // namespace blunted

#endif  // CAREER_STAFF_HPP
