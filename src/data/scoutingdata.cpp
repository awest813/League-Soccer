#include "scoutingdata.hpp"

#include <algorithm>
#include <cstdlib>

int ScoutingNetwork::HireScout(const Scout& scout) {
  Scout s = scout;
  s.scoutID = m_nextScoutID++;
  m_scouts.push_back(s);
  return s.scoutID;
}

void ScoutingNetwork::FireScout(int scoutID) {
  m_scouts.erase(
      std::remove_if(m_scouts.begin(), m_scouts.end(),
                     [scoutID](const Scout& s) { return s.scoutID == scoutID; }),
      m_scouts.end());
}

Scout* ScoutingNetwork::GetScout(int scoutID) {
  for (auto& s : m_scouts) {
    if (s.scoutID == scoutID) return &s;
  }
  return nullptr;
}

ScoutReport ScoutingNetwork::GenerateReport(int scoutID, int playerID, int season, int week) {
  ScoutReport report;
  report.reportID = m_nextReportID++;
  report.scoutID = scoutID;
  report.playerID = playerID;
  report.reportDate = season * 1000 + week;

  Scout* scout = GetScout(scoutID);
  int skill = scout ? scout->skill : 50;

  // Uncertainty is inversely proportional to scout skill: skill 100 = ±5, skill 0 = ±30
  int baseUncertainty = 30 - static_cast<int>(skill * 25.0f / 100.0f);

  static const char* attrs[] = {"pace", "shooting", "passing", "dribbling", "defending",
                                 "physical", nullptr};
  for (int i = 0; attrs[i]; ++i) {
    // Placeholder attribute values; in a full implementation these would come
    // from the player database
    report.knownAttributes[attrs[i]] = 50 + (std::rand() % 30 - 15);
    report.attributeUncertainty[attrs[i]] = baseUncertainty;
  }

  report.potential = 40 + (std::rand() % 60);
  report.recommendBid = report.potential >= 70;

  m_reports.push_back(report);
  return report;
}

void ScoutingNetwork::RevealAttribute(ScoutReport& report, const std::string& attr) {
  report.attributeUncertainty[attr] = 0;
}
