#include "matching_engine.hpp"
#include "cassert"
#include "domain_types.hpp"

namespace Domain::MatchingEngine {

auto MatchingEngine::MatchOrders(const Order &incoming_order, const Market::Orderbook &orderbook)
    -> Domain::Trade {
    non_static_class = 1;
    assert(incoming_order.side == Side::ASK || incoming_order.side == Side::BID);
    Domain::Trade result{};
    const auto resting_side = incoming_order.side == Side::ASK ? Side::BID : Side::ASK;
    const auto &resting_top_order = orderbook.GetTopOrder(resting_side);

    // TODO: this can be added to configuration of the exchange to support for non resting prices,
    // need testing if it actually works
    const auto trade_price = resting_top_order.price;

    const auto &ask_order = resting_side == Side::ASK ? resting_top_order : incoming_order;
    const auto &bid_order = resting_side == Side::ASK ? incoming_order : resting_top_order;
    if (CheckTrade({.ask_order = ask_order, .bid_order = bid_order})) {
        // incoming_order also may partially execute.
        // same for resting side
        // either case one will fully execute.
        result.ask_order_id = ask_order.order_id;
        result.bid_order_id = bid_order.order_id;
        result.price = trade_price;
        result.executed_quantity = std::min(ask_order.quantity, bid_order.quantity);
        result.timestamp = incoming_order.timestamp;
    }

    return result;
}

auto MatchingEngine::CheckTrade(const TradeOrders &trade_order) -> bool {
    bool valid = trade_order.ask_order.order_id != Invalid<OrderIdType> &&
                 trade_order.bid_order.order_id != Invalid<OrderIdType>;
    return valid && (trade_order.ask_order.price <= trade_order.bid_order.price);
}
} // namespace Domain::MatchingEngine
