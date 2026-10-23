#include "career_sponsors.hpp"

#include <algorithm>

#include "career_common.hpp"
#include "utils/localization.hpp"

namespace blunted {
namespace CareerSponsors {

void GenerateSponsorOffers(CareerSave& save) {
  save.availableSponsorOffers.clear();
  const SponsorDeal offers[] = {SponsorDeal("Vertex", "Shirt Sponsor", 4500000, 2, 40),
                                SponsorDeal("Apex Air", "Sleeve Sponsor", 2500000, 3, 55),
                                SponsorDeal("Northbank", "Training Kit", 1800000, 2, 30)};
  for (const SponsorDeal& offer : offers) {
    // A sponsor already under contract does not offer a second, stacking deal.
    const bool active = std::any_of(
        save.activeSponsors.begin(), save.activeSponsors.end(),
        [&offer](const SponsorDeal& sponsor) { return sponsor.sponsorName == offer.sponsorName; });
    if (!active)
      save.availableSponsorOffers.push_back(offer);
  }
}

bool AcceptSponsorDeal(CareerSave& save, CareerCommon::CareerEvents& events, int dealIndex) {
  if (dealIndex < 0 || dealIndex >= static_cast<int>(save.availableSponsorOffers.size()))
    return false;

  const SponsorDeal deal = save.availableSponsorOffers[dealIndex];
  if (save.reputation < deal.reputationRequirement)
    return false;

  // Revenue is paid once per season by ProcessSeasonFinances; paying it here
  // as well would credit the first year twice.
  save.activeSponsors.push_back(deal);
  save.finances.sponsorIncome += deal.annualRevenue;
  save.availableSponsorOffers.erase(save.availableSponsorOffers.begin() + dealIndex);
  events.AddEvent("commercial", TRF("career_event_sponsor", {deal.sponsorName}), 1, false);
  return true;
}

void TerminateSponsorDeal(CareerSave& save, CareerCommon::CareerEvents& events,
                          const std::string& sponsorName) {
  auto it = std::find_if(
      save.activeSponsors.begin(), save.activeSponsors.end(),
      [&sponsorName](const SponsorDeal& sponsor) { return sponsor.sponsorName == sponsorName; });
  if (it == save.activeSponsors.end())
    return;

  save.finances.sponsorIncome = std::max(0LL, save.finances.sponsorIncome - it->annualRevenue);
  save.activeSponsors.erase(it);
  events.ModifyBoardConfidence(-2);
}

}  // namespace CareerSponsors
}  // namespace blunted
