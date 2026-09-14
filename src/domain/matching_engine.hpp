#pragma once
#include "market.hpp"
#include <span>

namespace Domain::MatchingEngine {

// TODO: methods of this class may very well be static methods
class MatchingEngine {
  public:
    // currently requires incoming_order added to orderbook before matching
    [[nodiscard]] auto MatchOrders(Order &incoming_order, Market::Orderbook &orderbook,
                                   std::span<Trade> out_trades) -> size_t;
    static constexpr size_t MAX_TRADES = 1024;

  private:
    struct TradeOrders {
        Order ask_order;
        Order bid_order;
    };
    static auto CheckTrade(const TradeOrders &trade_orders) -> bool;
    int non_static_class{1};
};
} // namespace Domain::MatchingEngine
