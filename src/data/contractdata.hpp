#pragma once

#include <string>

enum class ContractStatus { ACTIVE, OFFERED, EXPIRED, RELEASED };

enum class NegotiationStatus { PENDING, ACCEPTED, REJECTED, EXPIRED };

struct Contract {
  int contractID = 0;
  int playerID = 0;
  int teamID = 0;
  int salary = 0;           // per week, in game currency
  int bonusPerGoal = 0;
  int bonusPerCleanSheet = 0;
  int releaseClause = 0;    // 0 = no clause
  int lengthYears = 0;
  int startYear = 0;
  int expiryYear = 0;
  ContractStatus status = ContractStatus::ACTIVE;
};

struct ContractNegotiation {
  int negotiationID = 0;
  int playerID = 0;
  int offeringTeamID = 0;
  int demandedSalary = 0;
  int offeredSalary = 0;
  int offeredLengthYears = 0;
  int roundsCompleted = 0;
  NegotiationStatus status = NegotiationStatus::PENDING;
};
