#ifndef CAREER_SPONSORS_HPP
#define CAREER_SPONSORS_HPP

#include <string>

#include "../../data/careerdata.hpp"
#include "career_common.hpp"

namespace blunted {
namespace CareerSponsors {

// Populates the list of available sponsor offers for the current reputation.
void GenerateSponsorOffers(CareerSave& save);

// Accepts an available offer (reputation permitting); returns whether it went
// through.
bool AcceptSponsorDeal(CareerSave& save, CareerCommon::CareerEvents& events, int dealIndex);

void TerminateSponsorDeal(CareerSave& save, CareerCommon::CareerEvents& events,
                          const std::string& sponsorName);

}  // namespace CareerSponsors
}  // namespace blunted

#endif  // CAREER_SPONSORS_HPP
