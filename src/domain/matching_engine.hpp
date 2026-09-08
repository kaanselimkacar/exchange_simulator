#pragma once
#include "market.hpp"

namespace Domain::MatchingEngine {

// methods of this class may very well be static methods
class MatchingEngine {
  public:
    auto MatchOrders(const Order &incoming_order, const Market::Orderbook &orderbook)
        -> Domain::Trade;

  private:
    struct TradeOrders {
        Order ask_order;
        Order bid_order;
    };
    static auto CheckTrade(const TradeOrders &trade_orders) -> bool;
    int non_static_class{1};
};
} // namespace Domain::MatchingEngine
