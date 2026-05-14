#include "market.hpp"
#include <cassert>

namespace Domain::Market {

auto PriceLevel::AddOrder(const Order &order) -> OrderListIterator {
    quantity_ += order.quantity;
    return order_list_.emplace(order_list_.end(), order);
};

auto Orderbook::AddOrder(const Order &order) -> void {
    auto add_order = [](const Order &order, auto &side_level, auto &orders_) -> void {
        const auto order_price = order.price;
        auto [iter, inserted] = side_level.try_emplace(order_price, order_price);

        PriceLevel &price_level = iter->second;
        auto order_iter = price_level.AddOrder(order);
        orders_.emplace(order.order_id,
                        OrderLocation{.price_level_ = price_level, .iterator_ = order_iter});
    };

    assert(order.quantity > 0);
    assert(order.side != Side::INVALID);
    assert(order.order_id > 0);
    assert(order.price > 0);
    if (order.side == Side::ASK) {
        add_order(order, asks_, orders_);
    } else {
        add_order(order, bids_, orders_);
    }
};
}; // namespace Domain::Market
