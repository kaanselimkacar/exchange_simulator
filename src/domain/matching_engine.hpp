#pragma once
#include "market.hpp"

namespace Domain::MatchingEngine {

// methods of this class may very well be static methods
class MatchingEngine {
  public:
    auto MatchOrders(Market::Orderbook &orderbook) -> Domain::Trade;
};
} // namespace Domain::MatchingEngine
