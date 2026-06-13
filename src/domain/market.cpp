#include "market.hpp"
#include <cassert>
#include <common/logger.hpp>

namespace Domain::Market {

auto PriceLevel::AddOrder(const Order &order) -> OrderListIterator {
    quantity_ += order.quantity;
    return order_list_.emplace(order_list_.end(), order);
};

auto PriceLevel::UpdateQuantity(const QuantityChange &qty_change) -> void {
    assert(quantity_ + qty_change.new_quantity >= qty_change.old_quantity);
    quantity_ += qty_change.new_quantity;
    quantity_ -= qty_change.old_quantity;
}

auto PriceLevel::IsEmpty() const -> bool {
    return order_list_.empty();
};

auto PriceLevel::DeleteOrder(OrderListIterator order_list_iterator) -> void {
    assert(quantity_ >= order_list_iterator->quantity);
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

auto Orderbook::ModifyOrder(const Order &updated_order) -> void {
    auto old_order_iter = orders_.find(updated_order.order_id);
    assert(old_order_iter != orders_.end());

    auto &old_order_location = old_order_iter->second;
    auto old_price = old_order_location.iterator_->price;
    // auto old_quantity = old_order_location.iterator_->quantity;
    auto old_side = old_order_location.iterator_->side;

    assert(old_side == updated_order.side);
    // check if there is a update to price
    if (updated_order.price != Invalid<PriceType> && updated_order.price != old_price) {
        // perform delete & add
        DeleteOrder(updated_order.order_id);
        AddOrder(updated_order);
    } else {
        // perform quantity update only, preserving price time prio
        ModifyOrderQuantity(updated_order.quantity, old_order_location);
    }
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

auto Orderbook::ModifyOrderQuantity(QuantityType new_quantity, OrderLocation &old_order_location)
    -> void {
    auto old_quantity = old_order_location.iterator_->quantity;
    old_order_location.price_level_.get().UpdateQuantity(
        {.new_quantity = new_quantity, .old_quantity = old_quantity});
    old_order_location.iterator_->quantity = new_quantity;
}

}; // namespace Domain::Market
