#include "matching_engine.hpp"
#include "cassert"
#include "domain_types.hpp"
#include <utility>

namespace Domain::MatchingEngine {

auto MatchingEngine::MatchOrders(Order &incoming_order, Market::Orderbook &orderbook,
                                 std::span<Trade> out_trades) -> size_t {
    non_static_class = 1;

    size_t ret_val{0};
    const auto resting_side = incoming_order.side == Side::ASK ? Side::BID : Side::ASK;
    while (incoming_order.quantity != 0) {
        if (ret_val >= MAX_TRADES) [[unlikely]] {
            std::unreachable();
        }
        Domain::Trade trade{};
        const auto &resting_top_order_res = orderbook.GetTopOrder(resting_side);

        if (!resting_top_order_res.has_value()) {
            return ret_val;
        }
        const auto &resting_top_order = resting_top_order_res.value();
        // TODO: this can be added to configuration of the exchange to support for non resting
        // prices, need testing if it actually works
        const auto trade_price = resting_top_order.price;

        // TODO: dont like we make this check every iteration
        const auto &ask_order = resting_side == Side::ASK ? resting_top_order : incoming_order;
        const auto &bid_order = resting_side == Side::ASK ? incoming_order : resting_top_order;

        if (!CheckTrade({.ask_order = ask_order, .bid_order = bid_order})) {
            return ret_val;
        }
        // incoming_order also may partially execute.
        // same for resting side
        // either case one will fully execute.
        trade.ask_order_id = ask_order.order_id;
        trade.bid_order_id = bid_order.order_id;
        trade.price = trade_price;
        trade.executed_quantity = std::min(ask_order.quantity, bid_order.quantity);
        trade.timestamp = incoming_order.timestamp;
        auto execute_res = orderbook.ExecuteTrade(
            {.ask_id = ask_order.order_id, .bid_id = bid_order.order_id}, trade.executed_quantity);

        if (execute_res != StatusCode::Success) [[unlikely]] {
            std::unreachable();
        }

        incoming_order.quantity -= trade.executed_quantity;
        out_trades[ret_val] = trade;
        ++ret_val;
    }

    return ret_val;
}

auto MatchingEngine::CheckTrade(const TradeOrders &trade_order) -> bool {
    bool valid = trade_order.ask_order.order_id != Invalid<OrderIdType> &&
                 trade_order.bid_order.order_id != Invalid<OrderIdType>;
    return valid && (trade_order.ask_order.price <= trade_order.bid_order.price);
}
} // namespace Domain::MatchingEngine
