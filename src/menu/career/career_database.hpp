#ifndef CAREER_DATABASE_HPP
#define CAREER_DATABASE_HPP

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "../../data/careerdata.hpp"
#include "career_common.hpp"
#include "career_persistence.hpp"
#include "career_sim.hpp"

namespace blunted {

using CareerSave = ::CareerSave;
using CareerEvent = ::CareerEvent;
using TransferTarget = ::TransferTarget;
using TransferBid = ::TransferBid;
using StaffMember = ::StaffMember;
using StadiumUpgrade = ::StadiumUpgrade;
using SponsorDeal = ::SponsorDeal;
using SimulatedMatch = ::SimulatedMatch;
using FixtureResult = ::FixtureResult;
using PlayerCareerState = ::PlayerCareerState;

struct CareerPendingFixture {
  bool hasFixture = false;
  bool isHome = true;
  int season = 0;
  int week = 0;
  int userTeamDBID = 0;
  int opponentTeamDBID = 0;
  std::string opponentName;
};

class CareerDatabase : public CareerCommon::CareerEvents {
public:
  CareerDatabase();
  ~CareerDatabase() override;

  static CareerDatabase& GetInstance() {
    static CareerDatabase instance;
    return instance;
  }

  bool SetPendingFixture(bool isHome, int userTeamDBID, int opponentTeamDBID,
                         const std::string& opponentName, int expectedSeason = -1,
                         int expectedWeek = -1);
  bool HasPendingFixture() const;
  const CareerPendingFixture& GetPendingFixture() const;
  void ClearPendingFixture();
  // Applies a finished 3D match to the pending fixture. Scorer lists hold the
  // database IDs of each team's goal scorers (MatchData team order).
  bool ConsumePlayedFixture(int matchGoals0, int matchGoals1,
                            const std::vector<int>& scorerIDs0 = {},
                            const std::vector<int>& scorerIDs1 = {});

  bool Initialize(const std::string& saveDir);
  bool HasSaveFile() const;
  bool HasSaveSlot(int slotIndex) const;
  bool LoadCareerSave(const std::string& saveName);
  bool LoadCareerSlot(int slotIndex);
  bool CreateNewCareer(const std::string& careerName, const std::string& mode,
                       const std::string& managerName);
  // Saves to the active slot: the one the career was loaded from or last saved
  // to. Lifecycle commits (matches, season rollover) write there too.
  bool SaveCareerData();
  // Saves to the given slot, which becomes the active slot (0-3; -1 is the
  // shared autosave and does not change the active slot).
  bool SaveCareerSlot(int slotIndex);
  int GetActiveSlot() const { return m_activeSlot; }
  bool DeleteCareerSlot(int slotIndex);
  bool AutoSave();
  bool GetSlotSummary(int slotIndex, CareerPersistence::CareerSaveSummary& outSummary) const;
  std::string GetSlotPath(int slotIndex) const;

  CareerSave* GetActiveSave() { return m_activeSave.get(); }

  void AddEvent(const std::string& eventType, const std::string& description, int reputationDelta,
                bool isMajor) override;

  void RecruitFreeAgent(const std::string& playerName,
                        CareerActionSource source = CareerActionSource::USER);
  bool TrainSquad();
  bool SetHandsOnManagement(bool enabled);
  bool SetTrainingPlan(CareerTrainingPlan plan,
                       CareerActionSource source = CareerActionSource::USER);
  bool TrainFocus(const std::string& focusArea);
  bool MotivatePlayer(const std::string& playerName,
                      CareerActionSource source = CareerActionSource::USER);
  bool DrillPlayer(const std::string& playerName,
                   CareerActionSource source = CareerActionSource::USER);
  void SetStrategy(const std::string& strategy,
                   CareerActionSource source = CareerActionSource::USER);

  void ScoutYouthPlayer(CareerActionSource source = CareerActionSource::USER);
  void PromoteYouthPlayer(const std::string& playerName,
                          CareerActionSource source = CareerActionSource::USER);

  void ModifyBudget(long long transferDelta, long long wageDelta,
                    CareerActionSource source = CareerActionSource::USER);
  void ModifyBoardConfidence(int delta) override;

  bool ExtendContract(const std::string& playerName);
  bool ToggleTransferList(const std::string& playerName);
  bool CanAdvanceSeason() const;
  bool AdvanceSeason(int expectedSeason = -1);
  bool CompleteFixture(int season, int week, bool isHome, int userTeamID, int opponentTeamID,
                       const std::string& opponentName, int userGoals, int opponentGoals,
                       const std::vector<std::string>& scorers = {},
                       const std::vector<std::string>& assisters = {});
  void ProcessPlayerGrowth(PlayerCareerState& player);
  void UpdatePlayerValue(PlayerCareerState& player);
  void ReleasePlayer(const std::string& playerName,
                     CareerActionSource source = CareerActionSource::USER);
  void RecordMatchStats(const std::string& playerName, int goals, int assists);

  void PopulateTransferMarket();
  std::vector<TransferTarget> GetTransferTargets() const;
  TransferBid PlaceBid(const std::string& playerName, long long bidAmount, int offeredWage,
                       int contractYears, CareerActionSource source = CareerActionSource::USER);
  std::vector<TransferBid>& GetActiveBids() { return m_activeBids; }
  void WithdrawBid(const std::string& playerName,
                   CareerActionSource source = CareerActionSource::USER);
  void ProcessPendingBids();
  std::string GetBidStatusString(BidStatus status) const;
  bool CompleteTransfer(const std::string& playerName,
                        CareerActionSource source = CareerActionSource::USER);

  void InitializeOwnerData();
  void UpgradeStadium(int upgradeIndex, CareerActionSource source = CareerActionSource::USER);
  void RenameStadium(const std::string& newName,
                     CareerActionSource source = CareerActionSource::USER);
  void RepairStadium(int amount, CareerActionSource source = CareerActionSource::USER);
  void SetTicketPrice(int price, CareerActionSource source = CareerActionSource::USER);

  void HireStaff(const StaffMember& member, CareerActionSource source = CareerActionSource::USER);
  void FireStaff(const std::string& staffName,
                 CareerActionSource source = CareerActionSource::USER);
  void GenerateStaffCandidates(std::vector<StaffMember>& candidates);

  void GenerateSponsorOffers();
  bool AcceptSponsorDeal(int dealIndex, CareerActionSource source = CareerActionSource::USER);
  void TerminateSponsorDeal(const std::string& sponsorName,
                            CareerActionSource source = CareerActionSource::USER);

  long long GetSeasonProfit() const;
  std::string GetFinancialHealthString() const;

  void GenerateBoardObjectives();

  void InvestInFanBase(long long amount, CareerActionSource source = CareerActionSource::USER);
  void InvestInPrestige(long long amount, CareerActionSource source = CareerActionSource::USER);

  SimulatedMatch SimulateMatchResult(const std::string& opponentName,
                                     const std::string& opponentTeamDBID, bool isHome = true);
  // Standings and top scorers
  std::vector<CareerSim::CareerLeagueTableRow> GetLeagueStandings(
      const std::vector<std::pair<int, std::string>>& leagueClubs = {}) const;
  std::vector<CareerSim::CareerTopScorer> GetTopScorers() const;

  // Estimate a 20-team league finish from a W/D/L record (deterministic).
  static int EstimateLeaguePosition(int wins, int draws, int losses);

  // Reseed the simulation RNG so match results are deterministic and
  // reproducible (used by tests; harmless in normal play).
  void SeedRng(unsigned int seed);

  int GetReputation() const;
  std::string GetReputationStatus() const;
  std::string GetMoraleString(int morale) const;
  std::string GetFormString(int form) const;
  std::string GetFormGuideString(int count = 5) const;
  int GetLegacyStat(const std::string& statName) const;
  std::vector<CareerEvent> GetRecentEvents(int limit = 5) const;

private:
  bool CommitLifecycle(const std::function<void()>& mutate);
  bool m_inLifecycle = false;
  std::unique_ptr<CareerSave> m_activeSave;
  std::string m_saveDirectory;
  std::vector<TransferTarget> m_transferTargets;
  std::vector<TransferBid> m_activeBids;
  CareerPendingFixture m_pendingFixture;
  int m_activeSlot = 0;
};

}  // namespace blunted

#endif  // CAREER_DATABASE_HPP
