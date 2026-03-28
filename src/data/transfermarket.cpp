#include "transfermarket.hpp"

#include <algorithm>

int TransferMarket::AddListing(const TransferListing& listing) {
  TransferListing l = listing;
  l.listingID = m_nextListingID++;
  m_listings.push_back(l);
  return l.listingID;
}

void TransferMarket::RemoveListing(int listingID) {
  m_listings.erase(
      std::remove_if(m_listings.begin(), m_listings.end(),
                     [listingID](const TransferListing& l) { return l.listingID == listingID; }),
      m_listings.end());
}

int TransferMarket::SubmitBid(const TransferBid& bid) {
  TransferBid b = bid;
  b.bidID = m_nextBidID++;
  b.status = BidStatus::PENDING;
  m_bids.push_back(b);
  return b.bidID;
}

void TransferMarket::AcceptBid(int bidID) {
  for (auto& b : m_bids) {
    if (b.bidID == bidID) {
      b.status = BidStatus::ACCEPTED;
      break;
    }
  }
}

void TransferMarket::RejectBid(int bidID) {
  for (auto& b : m_bids) {
    if (b.bidID == bidID) {
      b.status = BidStatus::REJECTED;
      break;
    }
  }
}

void TransferMarket::ProcessAIBids() {
  // Stub: future implementation will have AI teams evaluate listed players
  // and auto-submit bids based on budget and tactical need
}
