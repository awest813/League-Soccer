#pragma once

#include <vector>

enum class BidStatus { PENDING, ACCEPTED, REJECTED, WITHDRAWN };

struct TransferListing {
  int listingID = 0;
  int playerID = 0;
  int listedByTeamID = 0;
  int askingPrice = 0;
  bool isLoan = false;
  int loanFee = 0;
  int loanLengthMonths = 0;
};

struct TransferBid {
  int bidID = 0;
  int biddingTeamID = 0;
  int playerID = 0;
  int bidAmount = 0;
  bool isLoan = false;
  BidStatus status = BidStatus::PENDING;
};

class TransferMarket {
 public:
  TransferMarket() = default;

  void OpenWindow() { m_windowOpen = true; }
  void CloseWindow() { m_windowOpen = false; }
  bool IsWindowOpen() const { return m_windowOpen; }

  int AddListing(const TransferListing& listing);
  void RemoveListing(int listingID);
  const std::vector<TransferListing>& GetListings() const { return m_listings; }

  int SubmitBid(const TransferBid& bid);
  void AcceptBid(int bidID);
  void RejectBid(int bidID);
  const std::vector<TransferBid>& GetBids() const { return m_bids; }

  void ProcessAIBids();

 private:
  bool m_windowOpen = false;
  std::vector<TransferListing> m_listings;
  std::vector<TransferBid> m_bids;
  int m_nextListingID = 1;
  int m_nextBidID = 1;
};
