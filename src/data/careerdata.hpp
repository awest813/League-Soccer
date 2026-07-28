#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

enum class ClubRole { STAR, STARTER, ROTATION, PROSPECT, BENCH, RESERVE };
enum class MoraleState { EXCELLENT, GOOD, NEUTRAL, LOW, UNHAPPY };
enum class InjuryStatus { HEALTHY, DAY_TO_DAY, OUT_SHORT_TERM, OUT_LONG_TERM };
enum class TrainingFocus { FITNESS, SHARPNESS, ATTACKING, DEFENDING, SET_PIECES, YOUTH };
enum class TransferStatus { NONE, LISTED, WANTED, REJECTED_MOVE, ON_SHORTLIST };

enum class InboxItemType {
  BOARD_OBJECTIVE,
  TRANSFER_OFFER,
  PLAYER_COMPLAINT,
  SCOUT_REPORT,
  INJURY_UPDATE,
  FINANCE_UPDATE,
  YOUTH_PROMOTION,
  PRESS_SNIPPET,
  DERBY_HYPE
};

enum class CareerMode { PLAYER, MANAGER, GM, COACH, OWNER };

enum class BidStatus { PENDING, ACCEPTED, REJECTED, WITHDRAWN };

struct SeasonRecord {
  int season = 0;
  int teamID = 0;
  int wins = 0;
  int draws = 0;
  int losses = 0;
  int goalsFor = 0;
  int goalsAgainst = 0;
  int leaguePosition = 0;
  bool wonTitle = false;
};

struct ContractInfo {
  int yearsRemaining = 0;
  long long wage = 0;
  long long releaseClause = 0;
  bool loanListed = false;
  bool transferListed = false;
};

struct PlayerCareerState {
  int playerID = 0;
  int teamID = 0;
  std::string name;
  std::string position;
  int age = 0;
  long long value = 0;
  long long wage = 0;
  int ovr = 0;
  int pot = 0;
  int form = 50;
  int morale = 50;
  int fitness = 100;
  int matchForm = 50;
  ClubRole role = ClubRole::ROTATION;
  InjuryStatus injury = InjuryStatus::HEALTHY;
  TransferStatus transferStatus = TransferStatus::NONE;
  ContractInfo contract;
  bool isYouth = false;
  bool isPromotedFromAcademy = false;
  std::string preferredPosition;
  int careerGoals = 0;
  int careerAssists = 0;
  int matchesPlayed = 0;
  int databaseID = 0;
};

struct SquadState {
  std::vector<PlayerCareerState> roster;
  std::vector<int> startingXIPlayerIDs;
  std::vector<int> benchPlayerIDs;
  std::vector<int> reservesPlayerIDs;
  TrainingFocus weeklyTrainingFocus = TrainingFocus::SHARPNESS;
  int chemistry = 50;
};

struct FinanceState {
  long long cash = 0;
  long long transferBudget = 0;
  long long wageBudget = 0;
  long long weeklyWageSpend = 0;
  long long sponsorIncomePerSeason = 0;
  long long prizeMoney = 0;
  bool wageBudgetExceeded = false;
};

struct BoardObjective {
  std::string description;
  int targetValue = 0;
  int currentValue = 0;
  bool optional = false;
  bool completed = false;
  int penaltyIfFailed = 0;
};

struct BoardState {
  std::vector<BoardObjective> objectives;
  int confidence = 50;
  int prestige = 50;
  bool jobSecurityAtRisk = false;
};

struct ScoutProspect {
  std::string name;
  std::string position;
  int age = 0;
  int ovrEstimateLow = 0;
  int ovrEstimateHigh = 0;
  int potEstimateLow = 0;
  int potEstimateHigh = 0;
  int askingPrice = 0;
  int monthlyWageEstimate = 0;
  std::string region;
  int reportQuality = 0;
};

struct ScoutingState {
  std::string activeRegion;
  int monthsRemaining = 0;
  std::vector<ScoutProspect> shortlist;
  std::vector<ScoutProspect> discoveredProspects;
};

struct YouthAcademyState {
  std::string regionFocus;
  int monthlyIntakeSize = 0;
  std::vector<PlayerCareerState> prospects;
  std::vector<PlayerCareerState> promotedPlayers;
};

struct InboxItem {
  int id = 0;
  InboxItemType type = InboxItemType::BOARD_OBJECTIVE;
  std::string subject;
  std::string body;
  int weekCreated = 0;
  bool read = false;
  int relatedPlayerID = 0;
  int relatedTeamID = 0;
};

struct FixtureResult {
  int fixtureID = 0;
  int homeTeamID = 0;
  int awayTeamID = 0;
  int homeGoals = 0;
  int awayGoals = 0;
  bool played = false;
};

struct SeasonState {
  int currentSeason = 1;
  int currentWeek = 1;
  int maxWeeks = 38;
  bool inPreseason = true;
  bool transferWindowOpen = false;
  bool deadlineWeek = false;
  std::vector<FixtureResult> fixtures;
  std::vector<std::vector<int>> standingsByDivision;
  std::vector<std::string> seasonSummaries;
};

struct ClubIdentity {
  int clubID = 0;
  std::string clubName;
  std::string leagueName;
  std::string stadiumName;
  std::string kitColorPrimary;
  std::string kitColorSecondary;
  int leagueDivision = 0;
  int reputation = 50;
};

struct CareerEvent {
  std::string type;
  std::string description;
  int reputationImpact = 0;
  int64_t timestamp = 0;
  bool isMajor = false;

  CareerEvent() = default;
  CareerEvent(const std::string& t, const std::string& desc, int rep, int64_t time,
              bool major = false)
      : type(t), description(desc), reputationImpact(rep), timestamp(time), isMajor(major) {}
};

struct TransferTarget {
  std::string name;
  std::string preferredPosition;
  int overallRating = 0;
  int potentialRating = 0;
  int age = 0;
  long long value = 0;
  long long wage = 0;
  long long askingPrice = 0;
  int teamID = -1;
  bool isListed = true;
};

struct TransferBid {
  std::string playerName;
  long long bidAmount = 0;
  int offeredWage = 0;
  int contractYears = 3;
  long long agentFee = 0;
  BidStatus status = BidStatus::PENDING;
  int negotiationRounds = 0;
};

struct StadiumUpgrade {
  std::string name;
  std::string description;
  long long cost = 0;
  int buildTimeSeasons = 0;
  int seasonsRemaining = 0;
  int capacityIncrease = 0;
  int revenueBonus = 0;
  bool isComplete() const { return seasonsRemaining <= 0; }
};

struct Stadium {
  std::string name;
  int capacity = 30000;
  int condition = 70;
  int fanSatisfaction = 60;
  long long maintenanceCost = 500000;
  long long matchDayRevenue = 800000;
  std::vector<StadiumUpgrade> upgrades;
  std::vector<StadiumUpgrade> availableUpgrades;
};

struct StaffMember {
  std::string name;
  std::string role;
  int skill = 50;
  long long salary = 100000;
  int contractYearsRemaining = 2;
  int morale = 70;

  StaffMember() = default;
  StaffMember(const std::string& n, const std::string& r, int s, long long sal, int yrs)
      : name(n), role(r), skill(s), salary(sal), contractYearsRemaining(yrs), morale(70) {}
};

struct SponsorDeal {
  std::string sponsorName;
  std::string type;
  long long annualRevenue = 0;
  int yearsRemaining = 0;
  int reputationRequirement = 0;

  SponsorDeal() = default;
  SponsorDeal(const std::string& name, const std::string& t, long long rev, int yrs, int repReq)
      : sponsorName(name),
        type(t),
        annualRevenue(rev),
        yearsRemaining(yrs),
        reputationRequirement(repReq) {}
};

struct ClubFinances {
  long long totalRevenue = 0;
  long long totalExpenses = 0;
  long long netWorth = 50000000;
  long long matchDayIncome = 0;
  long long sponsorIncome = 0;
  long long merchandiseIncome = 0;
  long long tvRevenue = 5000000;
  long long playerWages = 0;
  long long staffWages = 0;
  long long stadiumCosts = 0;
  long long transferSpending = 0;
  long long transferIncome = 0;
  int ticketPrice = 40;
  int seasonTicketHolders = 15000;
  int debtLevel = 0;
};

enum class OwnerObjectiveType {
  PROMOTION,
  AVOID_RELEGATION,
  WIN_TITLE,
  FINANCIAL_STABILITY,
  GROW_FANBASE
};

struct SimulatedMatch {
  std::string opponentName;
  int homeGoals = 0;
  int awayGoals = 0;
  int homeShots = 0;
  int awayShots = 0;
  int homePossession = 50;
  std::vector<std::string> scorers;
  bool played = false;
};

struct OwnerBoardObjective {
  OwnerObjectiveType type = OwnerObjectiveType::FINANCIAL_STABILITY;
  std::string description;
  bool completed = false;
  int reputationReward = 5;
  int confidencePenalty = -10;
};

struct PressConferenceQuestion {
  std::string question;
  std::vector<std::string> answers;
  std::vector<int> reputationDeltas;
};

struct DivisionConfig {
  std::string name;
  int numTeams = 20;
  int promotionSpots = 3;
  int relegationSpots = 3;
  int playOffSpots = 0;
};

struct LeagueExpansionSettings {
  bool enabled = true;
  std::vector<DivisionConfig> divisions;
};

struct CustomLeagueConfig {
  std::string leagueName;
  int numDivisions = 1;
  std::vector<DivisionConfig> divisions;
  bool cupCompetition = false;
  std::string cupName;
};

struct CareerSave {
  int saveID = 0;
  CareerMode mode = CareerMode::MANAGER;
  std::string name;
  std::string managerName;
  ClubIdentity club;
  int controlledEntityID = 0;
  int currentSeason = 1;
  int budget = 0;
  int reputation = 50;
  std::string objective;
  long long transferBudget = 0;
  long long wageBudget = 0;
  int boardConfidence = 50;
  std::vector<PlayerCareerState> roster;
  std::vector<PlayerCareerState> freeAgents;
  std::vector<PlayerCareerState> youthAcademy;
  int trainingPoints = 10;
  int scoutingNetworkLevel = 1;
  std::string activeStrategy = "Balanced";
  std::map<std::string, int> legacyStats;
  std::vector<CareerEvent> recentEvents;
  SeasonState season;
  SquadState squad;
  FinanceState finance;
  BoardState board;
  ScoutingState scouting;
  YouthAcademyState youth;
  std::vector<InboxItem> inbox;
  std::vector<SeasonRecord> history;

  Stadium stadium;
  ClubFinances finances;
  std::vector<StaffMember> staff;
  std::vector<SponsorDeal> activeSponsors;
  std::vector<SponsorDeal> availableSponsorOffers;
  std::vector<OwnerBoardObjective> boardObjectives;
  int fanBase = 50;
  int clubPrestige = 50;
  bool isSeasonActive = true;
  int seasonWins = 0;
  int seasonDraws = 0;
  int seasonLosses = 0;
  int seasonGoalsFor = 0;
  int seasonGoalsAgainst = 0;

  LeagueExpansionSettings leagueSettings;
  CustomLeagueConfig customLeague;
};

// Standalone multi-save registry (indexed by saveID) with promotion/relegation
// helpers. This is a distinct concept from blunted::CareerDatabase (the
// per-session active-save singleton used by the career menu pages) and is
// intentionally named differently to avoid an ambiguous-symbol clash between
// the two when both headers are visible in the same translation unit.
class CareerSaveRegistry {
public:
  CareerSaveRegistry() = default;

  bool Load(const std::string& path);
  bool Save(const std::string& path) const;

  int CreateSave(const CareerSave& save);
  CareerSave* GetSave(int saveID);
  void DeleteSave(int saveID);
  const std::vector<CareerSave>& GetAllSaves() const { return m_saves; }

  void RecordSeason(int saveID, const SeasonRecord& record);
  void AdvanceSeason(int saveID);

  void ApplyReputationDelta(int saveID, int delta);

  void SetLeagueExpansionSettings(int saveID, const LeagueExpansionSettings& settings);

  static std::vector<std::pair<int, int>> ComputePromotionRelegation(
      const LeagueExpansionSettings& settings, const std::vector<std::vector<int>>& standings);

  void SetCustomLeague(int saveID, const CustomLeagueConfig& config);

private:
  std::vector<CareerSave> m_saves;
  int m_nextSaveID = 1;
};
