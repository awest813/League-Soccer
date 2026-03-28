#include "careerdata.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

bool CareerDatabase::Load(const std::string& /*path*/) {
  // Future: deserialise from JSON/SQLite; currently a no-op stub
  return true;
}

bool CareerDatabase::Save(const std::string& /*path*/) const {
  // Future: serialise to JSON/SQLite; currently a no-op stub
  return true;
}

int CareerDatabase::CreateSave(const CareerSave& save) {
  CareerSave s = save;
  s.saveID = m_nextSaveID++;
  m_saves.push_back(s);
  return s.saveID;
}

CareerSave* CareerDatabase::GetSave(int saveID) {
  for (auto& s : m_saves) {
    if (s.saveID == saveID) return &s;
  }
  return nullptr;
}

void CareerDatabase::DeleteSave(int saveID) {
  m_saves.erase(std::remove_if(m_saves.begin(), m_saves.end(),
                               [saveID](const CareerSave& s) { return s.saveID == saveID; }),
                m_saves.end());
}

void CareerDatabase::RecordSeason(int saveID, const SeasonRecord& record) {
  CareerSave* s = GetSave(saveID);
  if (s) s->history.push_back(record);
}

void CareerDatabase::AdvanceSeason(int saveID) {
  CareerSave* s = GetSave(saveID);
  if (s) s->currentSeason++;
}
