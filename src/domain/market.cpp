#include "market.hpp"
#include <cassert>
#include <common/logger.hpp>

namespace Domain::Market {

auto PriceLevel::AddOrder(const Order &order) -> OrderListIterator {
    quantity_ += order.quantity;
    return order_list_.emplace(order_list_.end(), order);
};

auto PriceLevel::IsEmpty() -> bool {
    return order_list_.empty();
};

auto PriceLevel::DeleteOrder(OrderListIterator order_list_iterator) -> void {
    quantity_ -= order_list_iterator->quantity;
    order_list_.erase(order_list_iterator);
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
    LogInfo("Order[{}] added to orderbook[{}]", order.order_id, orderbook_id_);
};

auto Orderbook::DeleteOrder(const OrderIdType order_id) -> void {
    auto order_iter = orders_.find(order_id);
    assert(order_iter != orders_.end());

    auto &order_location = order_iter->second;
    auto &price_level = order_location.price_level_.get();

    auto price = order_location.iterator_->price;
    auto side = order_location.iterator_->side;

    price_level.DeleteOrder(order_location.iterator_);

    assert(side != Side::INVALID);
    if (price_level.IsEmpty()) {
        if (side == Side::ASK) {
            asks_.erase(price);
        } else {
            bids_.erase(price);
        }
    }
    orders_.erase(order_iter);
}

}; // namespace Domain::Market
