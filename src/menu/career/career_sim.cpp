#include "career_sim.hpp"

#include <algorithm>
#include <functional>
#include <random>

#include "career_common.hpp"

namespace blunted {
namespace CareerSim {

namespace {
using CareerCommon::ClampInt;
using CareerCommon::RandomInt;
using CareerCommon::SafeStoi;
}  // namespace

int EstimateLeaguePosition(int wins, int draws, int losses) {
  const int played = wins + draws + losses;
  if (played <= 0)
    return 10;
  const int points = wins * 3 + draws;
  // Project points onto a 38-match season, then map onto finish bands.
  const float projected = (static_cast<float>(points) / static_cast<float>(played)) * 38.0f;
  if (projected >= 90.0f)
    return 1;
  if (projected >= 78.0f)
    return 2;
  if (projected >= 70.0f)
    return 4;
  if (projected >= 60.0f)
    return 7;
  if (projected >= 52.0f)
    return 10;
  if (projected >= 45.0f)
    return 12;
  if (projected >= 38.0f)
    return 15;
  if (projected >= 30.0f)
    return 17;
  return 19;
}

void ProcessPlayerGrowth(PlayerCareerState& player, const CareerSave* save) {
  int growthPoints = 0;

  int facilityBonus = 0;
  if (save != nullptr) {
    for (const auto& upgrade : save->stadium.upgrades) {
      if (upgrade.name == "Training Complex")
        facilityBonus += 15;
      if (upgrade.name == "Youth Academy")
        facilityBonus += 25;
    }
  }

  if (player.ovr < player.pot) {
    int formBonus = (player.matchForm >= 80) ? 20 : ((player.matchForm >= 60) ? 5 : 0);
    int playBonus = std::min(30, player.matchesPlayed);  // up to 30% bonus for playing games

    int growthChance = 0;
    if (player.age <= 21)
      growthChance = 45 + formBonus + playBonus + facilityBonus;
    else if (player.age <= 25)
      growthChance = 25 + formBonus + playBonus + (facilityBonus / 2);
    else if (player.age <= 29)
      growthChance = 5 + formBonus + (playBonus / 2);

    // Roll multiple times to allow breakout seasons
    for (int i = 0; i < 3; i++) {
      if (RandomInt(1, 100) <= growthChance) {
        growthPoints++;
        growthChance /= 2;  // diminishing returns
      }
    }
  } else if (player.age >= 30) {
    // Decline logic
    int declineChance = (player.age - 29) * 15;  // 30=15%, 33=60%
    if (player.fitness < 70)
      declineChance += 10;
    if (player.fitness < 50)
      declineChance += 15;  // bad fitness accelerates decline
    if (player.matchesPlayed == 0)
      declineChance += 10;

    // Better facilities slightly stave off decline
    declineChance -= (facilityBonus / 4);
    declineChance = std::max(5, declineChance);

    for (int i = 0; i < 2; i++) {
      if (RandomInt(1, 100) <= declineChance) {
        growthPoints--;
        declineChance /= 2;
      }
    }
  }

  player.ovr = std::min(99, std::max(1, player.ovr + growthPoints));
  player.morale = std::min(100, std::max(0, player.morale + RandomInt(-10, 10)));
  player.fitness = 100;   // Reset fitness for new season
  player.matchForm = 50;  // Reset form
}

void UpdatePlayerValue(PlayerCareerState& player) {
  long long ageModifier = 120;
  if (player.age >= 30)
    ageModifier = 85;
  else if (player.age <= 21)
    ageModifier = 135;

  long long formModifier = 80 + ClampInt(player.matchForm, 0, 100) / 5;
  long long potentialModifier = 100 + std::max(0, player.pot - player.ovr);
  long long baseValue = static_cast<long long>(player.ovr) * player.ovr * 4000;
  player.value =
      std::max(50000LL, (baseValue * ageModifier * formModifier * potentialModifier) / 1200000LL);
  player.wage = std::max(500LL, player.value / 1200LL);
}

void RecordMatchStats(CareerSave& save, const std::string& playerName, int goals, int assists) {
  auto it = std::find_if(
      save.roster.begin(), save.roster.end(),
      [&playerName](const PlayerCareerState& player) { return player.name == playerName; });
  if (it == save.roster.end())
    return;

  it->matchesPlayed++;
  it->careerGoals += std::max(0, goals);
  it->careerAssists += std::max(0, assists);
  // Keep per-match form gains modest so a hot streak cannot snowball the whole
  // squad's simulated strength across a 38-game season.
  it->matchForm = ClampInt(it->matchForm + goals * 3 + assists * 2 + 1, 0, 100);
  it->morale = ClampInt(it->morale + goals * 2 + assists * 1 + 1, 0, 100);
  UpdatePlayerValue(*it);
}

SimulatedMatch SimulateMatchResult(CareerSave& save, const std::string& opponentName,
                                   const std::string& opponentTeamDBID, bool isHome) {
  SimulatedMatch result;
  result.opponentName = opponentName;

  int teamOVR = 65;
  int opponentOVR = 65;
  int teamMorale = 70;
  int teamForm = 50;
  std::string strategy = save.activeStrategy;

  int ovrSum = 0;
  int moraleSum = 0;
  int formSum = 0;
  int count = 0;
  for (const auto& p : save.roster) {
    ovrSum += p.ovr;
    moraleSum += p.morale;
    formSum += p.matchForm;
    count++;
  }
  if (count > 0) {
    teamOVR = ovrSum / count;
    teamMorale = moraleSum / count;
    teamForm = formSum / count;
  }

  // Wider, identity-stable opponent pool (roughly 45-88) so weak and elite
  // clubs both meet realistic resistance across a season.
  if (!opponentName.empty()) {
    int seed = static_cast<int>(std::hash<std::string>{}(opponentName) % 1000);
    opponentOVR = 45 + (seed % 44);
  } else if (!opponentTeamDBID.empty()) {
    int idValue = SafeStoi(opponentTeamDBID);
    opponentOVR = 45 + ((idValue % 44) + 44) % 44;
  } else {
    opponentOVR = 55 + RandomInt(0, 30);
  }

  int baseAttack = teamOVR + (teamForm - 50) / 8 + (teamMorale - 50) / 12;
  int baseDefense = teamOVR + (teamForm - 50) / 10 + (teamMorale - 50) / 15;
  int oppAttack = opponentOVR + RandomInt(-2, 4);
  int oppDefense = opponentOVR + RandomInt(-2, 3);

  // Inject a bit more variance to allow for upsets
  baseAttack += RandomInt(-4, 5);
  baseDefense += RandomInt(-4, 5);
  oppAttack += RandomInt(-4, 5);
  oppDefense += RandomInt(-4, 5);

  int stratPossessionDelta = 0;
  if (strategy == "Attacking") {
    baseAttack += 4;
    baseDefense -= 3;
    stratPossessionDelta = 5;
  } else if (strategy == "Defensive") {
    baseAttack -= 3;
    baseDefense += 4;
    stratPossessionDelta = -5;
  } else if (strategy == "High Pressing") {
    baseAttack += 5;
    baseDefense -= 1;
    stratPossessionDelta = 8;
  } else if (strategy == "Counter Attack") {
    baseAttack += 4;
    baseDefense += 2;
    stratPossessionDelta = -8;
  } else if (strategy == "Possession") {
    baseAttack += 2;
    baseDefense += 3;
    stratPossessionDelta = 10;
  }

  // homeGoals/awayGoals mean "us" / "them" for ApplyMatchResult bookkeeping.
  // Expected goals are primarily OVR-gap driven so club tiers separate cleanly.
  float venueAttack = isHome ? 1.08f : 0.92f;
  float ourXG = 1.05f * venueAttack + 0.06f * static_cast<float>(baseAttack - oppDefense);
  float theirXG = 1.05f / venueAttack + 0.06f * static_cast<float>(oppAttack - baseDefense);
  ourXG = std::max(0.2f, std::min(3.8f, ourXG));
  theirXG = std::max(0.2f, std::min(3.8f, theirXG));

  std::poisson_distribution<int> ourDist(ourXG);
  std::poisson_distribution<int> theirDist(theirXG);
  int expectedHomeGoals = ourDist(CareerCommon::Rng());
  int expectedAwayGoals = theirDist(CareerCommon::Rng());

  result.homeGoals = ClampInt(expectedHomeGoals, 0, 9);
  result.awayGoals = ClampInt(expectedAwayGoals, 0, 7);
  result.homeShots = result.homeGoals + RandomInt(2, 8);
  result.awayShots = result.awayGoals + RandomInt(2, 8);
  result.homePossession = ClampInt(
      50 + (teamOVR - opponentOVR) + RandomInt(-5, 5) + stratPossessionDelta + (isHome ? 3 : -3),
      30, 70);
  result.played = true;

  int rosterSize = static_cast<int>(save.roster.size());
  if (rosterSize <= 0)
    return result;

  // Calculate weights for goalscorers based on position
  std::vector<int> weights(rosterSize, 1);
  int totalWeight = 0;
  for (int i = 0; i < rosterSize; i++) {
    const std::string& pos = save.roster[i].preferredPosition;
    if (pos == "ST" || pos == "CF")
      weights[i] = 25;
    else if (pos == "AM" || pos == "LW" || pos == "RW" || pos == "LM" || pos == "RM")
      weights[i] = 10;
    else if (pos == "CM" || pos == "WM")
      weights[i] = 4;
    else if (pos == "DM" || pos == "LB" || pos == "RB" || pos == "CB")
      weights[i] = 1;
    else if (pos == "GK")
      weights[i] = 0;

    // Boost based on form/OVR relative to squad
    if (save.roster[i].ovr >= teamOVR + 3)
      weights[i] += 3;
    if (save.roster[i].matchForm >= 80)
      weights[i] += 2;

    totalWeight += weights[i];
  }

  for (int g = 0; g < result.homeGoals; g++) {
    if (totalWeight <= 0) {
      result.scorers.push_back(save.roster[0].name);  // fallback
      continue;
    }

    int r = RandomInt(0, totalWeight - 1);
    int currentWeight = 0;
    int selectedIdx = 0;
    for (int i = 0; i < rosterSize; i++) {
      currentWeight += weights[i];
      if (r < currentWeight) {
        selectedIdx = i;
        break;
      }
    }
    result.scorers.push_back(save.roster[selectedIdx].name);
  }

  return result;
}

void ApplyMatchResult(CareerSave& save, CareerCommon::CareerEvents& events, int homeGoals,
                      int awayGoals, const std::string& opponentLabel,
                      const std::vector<std::string>& scorers) {
  const bool isWin = homeGoals > awayGoals;
  const bool isDraw = homeGoals == awayGoals;
  save.seasonWins += isWin ? 1 : 0;
  save.seasonDraws += isDraw ? 1 : 0;
  save.seasonLosses += (!isWin && !isDraw) ? 1 : 0;
  save.seasonGoalsFor += std::max(0, homeGoals);
  save.seasonGoalsAgainst += std::max(0, awayGoals);

  std::string summary =
      save.name + " " + std::to_string(homeGoals) + " - " + std::to_string(awayGoals);
  if (!opponentLabel.empty())
    summary += " " + opponentLabel;
  // Draws are reputation-neutral; only decisive results move the needle.
  // Match results are not major legacy events — season advance / transfers are.
  const int reputationDelta = isWin ? 1 : (isDraw ? 0 : -1);
  events.AddEvent("matchday", summary, reputationDelta, false);
  events.ModifyBoardConfidence(reputationDelta);

  // Squad-wide form/fitness regression after every match prevents mid-season
  // inflation from turning average clubs into perpetual title winners.
  for (auto& player : save.roster) {
    player.matchForm = ClampInt(player.matchForm - RandomInt(1, 3), 25, 100);
    player.fitness = ClampInt(player.fitness - RandomInt(0, 2), 55, 100);
  }

  for (const auto& scorerName : scorers) {
    RecordMatchStats(save, scorerName, 1, 0);
  }
}

void Process3DMatchResult(CareerSave& save, CareerCommon::CareerEvents& events, int homeGoals,
                          int awayGoals) {
  ApplyMatchResult(save, events, homeGoals, awayGoals, "(3D match)");
  // A completed 3D match is one league fixture, matching the simulated path
  // (where CareerMatchdayPage::GoBack advances the week). Without this the
  // calendar drifted between simulated and played matches.
  save.season.currentWeek++;
}

long long CalculateSeasonPrizeMoney(int leaguePosition) {
  switch (leaguePosition) {
    case 1:
      return 35000000LL;
    case 2:
      return 28000000LL;
    case 3:
      return 22000000LL;
    case 4:
      return 18000000LL;
    case 5:
      return 15000000LL;
    case 6:
      return 14000000LL;
    case 7:
      return 13000000LL;
    case 8:
      return 12000000LL;
    case 9:
      return 11000000LL;
    case 10:
      return 10000000LL;
    case 11:
      return 9000000LL;
    case 12:
      return 8000000LL;
    case 13:
      return 7000000LL;
    case 14:
      return 6500000LL;
    case 15:
      return 6000000LL;
    case 16:
      return 5500000LL;
    case 17:
      return 5000000LL;
    case 18:
      return 4000000LL;
    case 19:
      return 3500000LL;
    default:
      return 3000000LL;
  }
}

static std::vector<std::pair<int, std::string>> GetDefaultLeagueClubs(
    const std::string& leagueName) {
  if (leagueName == "Premier League") {
    return {{101, "Manchester City"},
            {102, "Arsenal"},
            {103, "Liverpool"},
            {104, "Aston Villa"},
            {105, "Tottenham"},
            {106, "Chelsea"},
            {107, "Newcastle"},
            {108, "Manchester United"},
            {109, "West Ham"},
            {110, "Brighton"},
            {111, "Wolves"},
            {112, "Fulham"},
            {113, "Bournemouth"},
            {114, "Crystal Palace"},
            {115, "Brentford"},
            {116, "Everton"},
            {117, "Nottingham Forest"},
            {118, "Luton Town"},
            {119, "Burnley"},
            {120, "Sheffield United"}};
  } else if (leagueName == "La Liga") {
    return {{201, "Real Madrid"},     {202, "FC Barcelona"},    {203, "Girona"},
            {204, "Atletico Madrid"}, {205, "Athletic Bilbao"}, {206, "Real Sociedad"},
            {207, "Real Betis"},      {208, "Villarreal"},      {209, "Valencia"},
            {210, "Getafe"},          {211, "Osasuna"},         {212, "Sevilla"},
            {213, "Mallorca"},        {214, "Las Palmas"},      {215, "Alaves"},
            {216, "Rayo Vallecano"},  {217, "Celta Vigo"},      {218, "Cadiz"},
            {219, "Granada"},         {220, "Almeria"}};
  } else if (leagueName == "Bundesliga") {
    return {{301, "Bayer Leverkusen"},
            {302, "Bayern Munich"},
            {303, "VfB Stuttgart"},
            {304, "Borussia Dortmund"},
            {305, "RB Leipzig"},
            {306, "Eintracht Frankfurt"},
            {307, "Hoffenheim"},
            {308, "Freiburg"},
            {309, "Heidenheim"},
            {310, "Werder Bremen"},
            {311, "Augsburg"},
            {312, "Wolfsburg"},
            {313, "Mainz"},
            {314, "Borussia Monchengladbach"},
            {315, "Union Berlin"},
            {316, "Bochum"},
            {317, "Koln"},
            {318, "Darmstadt"}};
  } else if (leagueName == "Eredivisie") {
    return {{401, "PSV Eindhoven"},
            {402, "Feyenoord"},
            {403, "Twente"},
            {404, "AZ Alkmaar"},
            {405, "Ajax"},
            {406, "NEC Nijmegen"},
            {407, "Utrecht"},
            {408, "Sparta Rotterdam"},
            {409, "Go Ahead Eagles"},
            {410, "Fortuna Sittard"},
            {411, "Heerenveen"},
            {412, "PEC Zwolle"},
            {413, "Almere City"},
            {414, "Heracles"},
            {415, "Excelsior"},
            {416, "RKC Waalwijk"},
            {417, "Volendam"},
            {418, "Vitesse"}};
  }
  // Default balanced 20-club European League
  return {{501, "Real Madrid"},
          {502, "Manchester City"},
          {503, "Bayern Munich"},
          {504, "FC Barcelona"},
          {505, "Arsenal"},
          {506, "Paris Saint-Germain"},
          {507, "Liverpool"},
          {508, "Inter Milan"},
          {509, "Bayer Leverkusen"},
          {510, "Atletico Madrid"},
          {511, "Borussia Dortmund"},
          {512, "Juventus"},
          {513, "AC Milan"},
          {514, "Napoli"},
          {515, "Chelsea"},
          {516, "Manchester United"},
          {517, "Ajax"},
          {518, "Benfica"},
          {519, "Sporting CP"},
          {520, "FC Porto"}};
}

std::vector<CareerLeagueTableRow> GenerateLeagueStandings(
    const CareerSave& save, const std::vector<std::pair<int, std::string>>& leagueClubs) {
  std::vector<std::pair<int, std::string>> clubs = leagueClubs;
  if (clubs.empty()) {
    clubs = GetDefaultLeagueClubs(save.club.leagueName);
  }

  // Ensure user team is present in clubs list
  bool userFound = false;
  for (const auto& c : clubs) {
    if (c.first == save.club.clubID || c.second == save.name || c.second == save.club.clubName) {
      userFound = true;
      break;
    }
  }
  if (!userFound) {
    if (!clubs.empty()) {
      clubs.back() = {save.club.clubID > 0 ? save.club.clubID : 999, save.name};
    } else {
      clubs.push_back({save.club.clubID > 0 ? save.club.clubID : 999, save.name});
    }
  }

  std::vector<CareerLeagueTableRow> table;
  table.reserve(clubs.size());

  // Current games played across league
  const int userPlayed = save.seasonWins + save.seasonDraws + save.seasonLosses;
  const int leaguePlayed = std::min(38, std::max(userPlayed, save.season.currentWeek - 1));

  for (const auto& c : clubs) {
    const bool isUser =
        (c.first == save.club.clubID || c.second == save.name || c.second == save.club.clubName);
    CareerLeagueTableRow row;
    row.teamID = c.first;
    row.name = c.second;
    row.isUserTeam = isUser;

    if (isUser) {
      row.played = userPlayed;
      row.wins = save.seasonWins;
      row.draws = save.seasonDraws;
      row.losses = save.seasonLosses;
      row.goalsFor = save.seasonGoalsFor;
      row.goalsAgainst = save.seasonGoalsAgainst;
      row.goalDiff = row.goalsFor - row.goalsAgainst;
      row.points = row.wins * 3 + row.draws;

      // Build form guide from recent win/draw/loss ratio
      std::string formStr;
      int remWins = row.wins;
      int remDraws = row.draws;
      int remLosses = row.losses;
      for (int f = 0; f < std::min(5, row.played); ++f) {
        if (remWins > 0) {
          formStr += "W";
          remWins--;
        } else if (remDraws > 0) {
          formStr += "D";
          remDraws--;
        } else if (remLosses > 0) {
          formStr += "L";
          remLosses--;
        }
      }
      row.form = formStr.empty() ? "-" : formStr;
    } else {
      row.played = leaguePlayed;
      if (leaguePlayed == 0) {
        row.wins = 0;
        row.draws = 0;
        row.losses = 0;
        row.goalsFor = 0;
        row.goalsAgainst = 0;
        row.goalDiff = 0;
        row.points = 0;
        row.form = "-";
      } else {
        // Deterministic rating based on club name and season
        uint32_t seed = 0;
        for (char ch : c.second)
          seed = seed * 31 + static_cast<unsigned char>(ch);
        seed += static_cast<uint32_t>(save.season.currentSeason * 1009);

        // Rating roughly 55 to 88
        int rating = 55 + (seed % 34);

        // Win probability from rating: top ~62%, mid ~38%, weak ~20%
        float winProb = 0.20f + (rating - 55) * (0.42f / 33.0f);
        float drawProb = 0.26f - (rating - 55) * (0.06f / 33.0f);

        int simWins = 0;
        int simDraws = 0;
        int simLosses = 0;
        std::string formStr;

        std::mt19937 rng(seed + static_cast<uint32_t>(leaguePlayed * 97));
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);

        for (int m = 0; m < leaguePlayed; ++m) {
          float roll = dist(rng);
          char res = 'L';
          if (roll < winProb) {
            simWins++;
            res = 'W';
          } else if (roll < winProb + drawProb) {
            simDraws++;
            res = 'D';
          } else {
            simLosses++;
            res = 'L';
          }
          if (m >= leaguePlayed - 5) {
            formStr += res;
          }
        }

        row.wins = simWins;
        row.draws = simDraws;
        row.losses = simLosses;
        row.points = row.wins * 3 + row.draws;

        // Goals estimation: top teams score ~1.8-2.3, concede ~0.8-1.1
        float gfPerGame = 0.8f + (rating - 55) * (1.4f / 33.0f);
        float gaPerGame = 1.9f - (rating - 55) * (1.1f / 33.0f);
        row.goalsFor = std::max(
            0, static_cast<int>(gfPerGame * leaguePlayed + (static_cast<int>(seed % 5) - 2)));
        row.goalsAgainst = std::max(
            0, static_cast<int>(gaPerGame * leaguePlayed + (static_cast<int>((seed / 7) % 5) - 2)));
        row.goalDiff = row.goalsFor - row.goalsAgainst;
        row.form = formStr.empty() ? "-" : formStr;
      }
    }
    table.push_back(row);
  }

  // Sort by Points DESC, GoalDiff DESC, GoalsFor DESC, Name ASC
  std::sort(table.begin(), table.end(),
            [](const CareerLeagueTableRow& a, const CareerLeagueTableRow& b) {
              if (a.points != b.points)
                return a.points > b.points;
              if (a.goalDiff != b.goalDiff)
                return a.goalDiff > b.goalDiff;
              if (a.goalsFor != b.goalsFor)
                return a.goalsFor > b.goalsFor;
              return a.name < b.name;
            });

  return table;
}

std::vector<CareerTopScorer> GetTopScorers(const CareerSave& save) {
  std::vector<CareerTopScorer> scorers;

  // Add user squad scorers
  for (const auto& p : save.roster) {
    if (p.careerGoals > 0) {
      scorers.push_back({p.name, save.name, p.careerGoals, true});
    }
  }

  // Add simulated stars based on league games played
  const int userPlayed = save.seasonWins + save.seasonDraws + save.seasonLosses;
  const int played = std::min(38, std::max(userPlayed, save.season.currentWeek - 1));

  static const std::vector<std::pair<std::string, std::string>> starPlayers = {
      {"Erling Haaland", "Manchester City"}, {"Kylian Mbappe", "Real Madrid"},
      {"Harry Kane", "Bayern Munich"},       {"Vinicius Junior", "Real Madrid"},
      {"Mohamed Salah", "Liverpool"},        {"Robert Lewandowski", "FC Barcelona"},
      {"Lautaro Martinez", "Inter Milan"},   {"Bukayo Saka", "Arsenal"},
      {"Phil Foden", "Manchester City"},     {"Jude Bellingham", "Real Madrid"}};

  for (size_t i = 0; i < starPlayers.size(); ++i) {
    uint32_t seed = static_cast<uint32_t>(i * 101 + save.season.currentSeason * 37);
    int goals = 0;
    if (played > 0) {
      float goalRate = 0.55f - static_cast<float>(i) * 0.035f;
      goals = std::max(0, static_cast<int>(goalRate * played + (static_cast<int>(seed % 5) - 2)));
    }
    scorers.push_back({starPlayers[i].first, starPlayers[i].second, goals, false});
  }

  std::sort(scorers.begin(), scorers.end(), [](const CareerTopScorer& a, const CareerTopScorer& b) {
    if (a.goals != b.goals)
      return a.goals > b.goals;
    return a.playerName < b.playerName;
  });

  if (scorers.size() > 10) {
    scorers.resize(10);
  }

  return scorers;
}

void AdvanceSeason(CareerSave& save, CareerCommon::CareerEvents& events,
                   std::vector<TransferBid>& bids, std::vector<TransferTarget>& targets) {
  int userPos = EstimateLeaguePosition(save.seasonWins, save.seasonDraws, save.seasonLosses);

  SeasonRecord record;
  record.season = save.season.currentSeason;
  record.teamID = save.club.clubID;
  record.wins = save.seasonWins;
  record.draws = save.seasonDraws;
  record.losses = save.seasonLosses;
  record.goalsFor = save.seasonGoalsFor;
  record.goalsAgainst = save.seasonGoalsAgainst;
  record.leaguePosition = userPos;
  record.wonTitle = (userPos == 1);
  save.history.push_back(record);

  // Prize money award
  long long prizeMoney = CalculateSeasonPrizeMoney(userPos);
  save.transferBudget += prizeMoney;
  events.AddEvent("finance",
                  "Season " + std::to_string(save.season.currentSeason) +
                      " prize money awarded: EUR " + std::to_string(prizeMoney) +
                      " for finishing #" + std::to_string(userPos),
                  2, true);

  // Title achievement and board feedback
  if (userPos == 1) {
    save.legacyStats["titles"]++;
    events.AddEvent("trophy",
                    "🏆 LEAGUE CHAMPIONS: " + save.name + " won the title in Season " +
                        std::to_string(save.season.currentSeason) + "!",
                    5, true);
    events.ModifyBoardConfidence(15);
  } else if (userPos <= 4) {
    events.AddEvent("champions",
                    "Qualified for Continental Cup with a #" + std::to_string(userPos) + " finish!",
                    3, true);
    events.ModifyBoardConfidence(8);
  } else if (userPos >= 18) {
    events.AddEvent("warning",
                    "Finished in relegation danger zone (#" + std::to_string(userPos) +
                        "). Board pressure is mounting!",
                    -3, true);
    events.ModifyBoardConfidence(-15);
  }

  // Contract expirations & renewals
  std::vector<std::string> departedPlayers;
  for (auto it = save.roster.begin(); it != save.roster.end();) {
    it->age++;
    if (it->contract.yearsRemaining > 0)
      it->contract.yearsRemaining--;
    ProcessPlayerGrowth(*it, &save);
    UpdatePlayerValue(*it);
    it->matchesPlayed = 0;

    if (it->contract.yearsRemaining <= 0) {
      if (save.roster.size() > 14) {
        departedPlayers.push_back(it->name);
        PlayerCareerState fa = *it;
        fa.contract.yearsRemaining = 0;
        save.freeAgents.push_back(fa);
        it = save.roster.erase(it);
        continue;
      } else {
        // Automatic emergency extension if squad would fall below minimum 14
        it->contract.yearsRemaining = 1;
        it->wage = it->wage * 105 / 100;
      }
    }
    ++it;
  }
  for (const auto& name : departedPlayers) {
    events.AddEvent("contract", "Contract expired: " + name + " departed on free transfer.", -1,
                    false);
  }

  for (auto& member : save.staff) {
    if (member.contractYearsRemaining > 0)
      member.contractYearsRemaining--;
  }
  save.staff.erase(
      std::remove_if(save.staff.begin(), save.staff.end(),
                     [](const StaffMember& member) { return member.contractYearsRemaining <= 0; }),
      save.staff.end());

  for (auto& sponsor : save.activeSponsors) {
    if (sponsor.yearsRemaining > 0)
      sponsor.yearsRemaining--;
  }
  save.activeSponsors.erase(
      std::remove_if(save.activeSponsors.begin(), save.activeSponsors.end(),
                     [](const SponsorDeal& sponsor) { return sponsor.yearsRemaining <= 0; }),
      save.activeSponsors.end());

  for (auto& upgrade : save.stadium.upgrades) {
    if (upgrade.seasonsRemaining > 0) {
      upgrade.seasonsRemaining--;
      if (upgrade.seasonsRemaining == 0) {
        save.stadium.capacity += upgrade.capacityIncrease;
        save.stadium.matchDayRevenue += upgrade.revenueBonus;
        events.AddEvent("stadium", "Completed stadium upgrade: " + upgrade.name, 1, false);
      }
    }
  }

  save.season.currentSeason++;
  save.currentSeason = save.season.currentSeason;
  save.season.currentWeek = 1;
  save.season.inPreseason = true;
  save.season.transferWindowOpen = true;
  save.trainingPoints = (save.mode == CareerMode::COACH) ? 20 : 10;
  save.availableSponsorOffers.clear();
  bids.clear();
  targets.clear();
  save.seasonWins = 0;
  save.seasonDraws = 0;
  save.seasonLosses = 0;
  save.seasonGoalsFor = 0;
  save.seasonGoalsAgainst = 0;
  events.AddEvent("season", "Advanced to season " + std::to_string(save.season.currentSeason), 1,
                  true);
}

}  // namespace CareerSim
}  // namespace blunted
