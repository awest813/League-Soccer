#ifndef CAREER_TRANSFERS_HPP
#define CAREER_TRANSFERS_HPP

#include <string>
#include <vector>

#include "../../data/careerdata.hpp"
#include "career_common.hpp"

namespace blunted {
namespace CareerTransfers {

// Signs a player from the free-agent pool into the roster (budget permitting).
void RecruitFreeAgent(CareerSave& save, CareerCommon::CareerEvents& events,
                      const std::string& playerName);

// Age- and potential-aware market valuation used for transfer targets: young
// high-ceiling players carry a premium, veterans are discounted. Mirrors
// CareerSim::UpdatePlayerValue so market and roster values stay consistent.
long long ComputeMarketValue(int overallRating, int potentialRating, int age);

// Releases a roster player into the free-agent pool.
void ReleasePlayer(CareerSave& save, CareerCommon::CareerEvents& events,
                   const std::string& playerName);

// Generates the transfer-market targets (no-op if already populated).
void PopulateTransferMarket(std::vector<TransferTarget>& targets);

// Places a bid on a listed target; returns the resulting bid object.
TransferBid PlaceBid(CareerSave& save, CareerCommon::CareerEvents& events,
                     std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids,
                     const std::string& playerName, long long bidAmount, int offeredWage,
                     int contractYears);

void WithdrawBid(std::vector<TransferBid>& bids, const std::string& playerName);

// Resolves all pending bids against their targets' asking prices.
void ProcessPendingBids(CareerSave& save, CareerCommon::CareerEvents& events,
                        std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids);

std::string GetBidStatusString(BidStatus status);

// Completes an accepted bid: moves the target into the roster and settles
// budgets. Returns false if no accepted bid / target matches.
bool CompleteTransfer(CareerSave& save, CareerCommon::CareerEvents& events,
                      std::vector<TransferTarget>& targets, std::vector<TransferBid>& bids,
                      const std::string& playerName);

}  // namespace CareerTransfers
}  // namespace blunted

#endif  // CAREER_TRANSFERS_HPP
