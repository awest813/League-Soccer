#pragma once

#include <string>
#include <vector>

enum class ScoutGrade { A, B, C, D, F };

struct DraftProspect {
  int prospectID = 0;
  std::string firstName;
  std::string lastName;
  int age = 18;
  std::string position;
  ScoutGrade scoutGrade = ScoutGrade::C;
  int projectedPick = 0;      // estimated draft position
  int actualRating = 0;       // hidden until picked
};

struct DraftPick {
  int round = 0;
  int pickNumber = 0;
  int owningTeamID = 0;
  int originalTeamID = 0;
  bool isTraded = false;
};

class DraftSystem {
 public:
  DraftSystem() = default;

  // Generate a fresh pool of prospects for the given year
  void GenerateProspects(int year, int count = 60);

  // Conduct a full draft round; returns picks in order
  std::vector<std::pair<int /*teamID*/, int /*prospectID*/>> ConductDraft(
      const std::vector<int>& teamOrder);

  bool TradePick(int fromTeamID, int toTeamID, int round, int pickNumber);

  const std::vector<DraftProspect>& GetProspects() const { return m_prospects; }
  const std::vector<DraftPick>& GetPicks() const { return m_picks; }

  void SetDraftOrder(const std::vector<DraftPick>& picks) { m_picks = picks; }

 private:
  std::vector<DraftProspect> m_prospects;
  std::vector<DraftPick> m_picks;
  int m_nextProspectID = 1;

  static ScoutGrade GradeFromRating(int rating);
};
