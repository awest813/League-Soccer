#ifndef CAREER_DATABASE_HPP
#define CAREER_DATABASE_HPP

#include <memory>
#include <string>
#include <vector>

#include "../../data/careerdata.hpp"

namespace blunted {

using CareerSave = ::CareerSave;
using CareerEvent = ::CareerEvent;
using TransferTarget = ::TransferTarget;
using TransferBid = ::TransferBid;
using StaffMember = ::StaffMember;
using StadiumUpgrade = ::StadiumUpgrade;
using SponsorDeal = ::SponsorDeal;
using SimulatedMatch = ::SimulatedMatch;

class CareerDatabase {
public:
  CareerDatabase();
  ~CareerDatabase();

  static CareerDatabase& GetInstance() {
    static CareerDatabase instance;
    return instance;
  }

  bool Initialize(const std::string& saveDir);
  bool LoadCareerSave(const std::string& saveName);
  bool CreateNewCareer(const std::string& careerName, const std::string& mode,
                       const std::string& managerName);
  bool SaveCareerData();

  CareerSave* GetActiveSave() { return m_activeSave.get(); }

  void AddEvent(const std::string& eventType, const std::string& description, int reputationDelta,
                bool isMajor);

  void RecruitFreeAgent(const std::string& playerName);
  bool TrainSquad();
  bool TrainFocus(const std::string& focusArea);
  void SetStrategy(const std::string& strategy);

  void ScoutYouthPlayer();
  void PromoteYouthPlayer(const std::string& playerName);

  void ModifyBudget(long long transferDelta, long long wageDelta);
  void ModifyBoardConfidence(int delta);

  void AdvanceSeason();
  void ProcessPlayerGrowth(PlayerCareerState& player);
  void UpdatePlayerValue(PlayerCareerState& player);
  void ReleasePlayer(const std::string& playerName);
  void RecordMatchStats(const std::string& playerName, int goals, int assists);

  void PopulateTransferMarket();
  std::vector<TransferTarget> GetTransferTargets() const;
  TransferBid PlaceBid(const std::string& playerName, long long bidAmount, int offeredWage,
                       int contractYears);
  std::vector<TransferBid>& GetActiveBids() { return m_activeBids; }
  void WithdrawBid(const std::string& playerName);
  void ProcessPendingBids();
  std::string GetBidStatusString(BidStatus status) const;
  int GetNextBidID() { return ++m_nextBidID; }
  bool CompleteTransfer(const std::string& playerName);

  void InitializeOwnerData();
  void UpgradeStadium(int upgradeIndex);
  void RenameStadium(const std::string& newName);
  void RepairStadium(int amount);
  void SetTicketPrice(int price);

  void HireStaff(const StaffMember& member);
  void FireStaff(const std::string& staffName);
  void GenerateStaffCandidates(std::vector<StaffMember>& candidates);

  void GenerateSponsorOffers();
  bool AcceptSponsorDeal(int dealIndex);
  void TerminateSponsorDeal(const std::string& sponsorName);

  void ProcessSeasonFinances();
  long long GetSeasonProfit() const;
  std::string GetFinancialHealthString() const;

  void GenerateBoardObjectives();
  void EvaluateBoardObjectives();

  void InvestInFanBase(long long amount);
  void InvestInPrestige(long long amount);

  SimulatedMatch SimulateMatchResult(const std::string& opponentName,
                                     const std::string& opponentTeamDBID, bool isHome = true);
  // Apply a finished match to season W/D/L, goals, board confidence, reputation,
  // and optional scorer bookkeeping. Shared by sim and 3D result paths.
  void ApplyMatchResult(int homeGoals, int awayGoals, const std::string& opponentLabel,
                        const std::vector<std::string>& scorers = {});
  void Process3DMatchResult(int homeGoals, int awayGoals);

  // Estimate a 20-team league finish from a W/D/L record (deterministic).
  static int EstimateLeaguePosition(int wins, int draws, int losses);

  // Reseed the simulation RNG so match results are deterministic and
  // reproducible (used by tests; harmless in normal play).
  void SeedRng(unsigned int seed);

  int GetReputation() const;
  std::string GetReputationStatus() const;
  std::string GetMoraleString(int morale) const;
  std::string GetFormString(int form) const;
  int GetLegacyStat(const std::string& statName) const;
  std::vector<CareerEvent> GetRecentEvents(int limit = 5) const;

private:
  std::unique_ptr<CareerSave> m_activeSave;
  std::string m_saveDirectory;
  std::vector<TransferTarget> m_transferTargets;
  std::vector<TransferBid> m_activeBids;
  int m_nextBidID = 0;

  bool SaveToFile(const std::string& path) const;
  bool LoadFromFile(const std::string& path);
};

}  // namespace blunted

#endif  // CAREER_DATABASE_HPP
