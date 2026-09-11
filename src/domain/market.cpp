#include "market.hpp"
#include "domain_types.hpp"
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

auto PriceLevel::GetTopOrder() const -> Order {
    if (order_list_.empty()) [[unlikely]] {
        // TODO: this doesn't check for anything really
        return {};
    }
    return *order_list_.begin();
}

auto Orderbook::AddOrder(const Order &order) -> void {
    auto add_order = [](const Order &order, auto &side_level, auto &orders_) -> void {
        const auto order_price = order.price;
        auto [iter, inserted] = side_level.try_emplace(order_price, order_price);

        PriceLevel &price_level = iter->second;
        auto order_iter = price_level.AddOrder(order);
        orders_.emplace(order.order_id,
                        OrderLocation{.price_level_ = price_level, .iterator_ = order_iter});
    };

    ValidateOrder(order);
    assert(!orders_.contains(order.order_id));

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

    assert(updated_order.quantity > 0 && updated_order.quantity != Invalid<QuantityType>);

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

auto Orderbook::ExecuteTrade(TradeExec trade_exec, QuantityType exec_qty) -> void {
    auto update_qty = [&](OrderIdType order_id, QuantityType exec_qty) -> void {
        auto iter = orders_.find(order_id);
        if (iter == orders_.end()) {
            // TODO: unhappy path
            assert(0);
        }
        auto curr_qty = iter->second.iterator_->quantity;
        if (curr_qty < exec_qty) {
            // TODO: unhappy path
            assert(0);
        }

        if (curr_qty == exec_qty) {
            DeleteOrder(order_id);
        } else {
            ModifyOrderQuantity(curr_qty - exec_qty, iter->second);
        }
    };

    update_qty(trade_exec.ask_id_, exec_qty);
    update_qty(trade_exec.bid_id_, exec_qty);
}

auto Orderbook::ModifyOrderQuantity(QuantityType new_quantity, OrderLocation &old_order_location)
    -> void {
    auto old_quantity = old_order_location.iterator_->quantity;
    old_order_location.price_level_.get().UpdateQuantity(
        {.new_quantity = new_quantity, .old_quantity = old_quantity});
    old_order_location.iterator_->quantity = new_quantity;
}

auto Orderbook::GetTopOrder(const Side &side) const -> Order {
    const auto get_top_order = [](const auto &side_map) -> Order {
        if (side_map.empty()) {
            return {};
        }
        return side_map.begin()->second.GetTopOrder();
    };
    return side == Side::ASK ? get_top_order(asks_) : get_top_order(bids_);
}

auto Orderbook::ValidateOrder(const Order &order) -> void {
    assert(order.quantity > 0 && order.quantity != Invalid<QuantityType>);
    assert(order.side != Side::INVALID);
    assert(order.order_id > 0 && order.order_id != Invalid<OrderIdType>);
    assert(order.price > 0 && order.price != Invalid<PriceType>);
}

auto OrderbookManager::AddOrderbook(std::unique_ptr<Orderbook> orderbook) -> void {
    const auto orderbook_id = orderbook->GetOrderbookId();
    auto result = orderbooks_.try_emplace(orderbook_id, std::move(orderbook));
    if (!result.second) {
        assert(0);
    }
}

auto OrderbookManager::GetOrderbook(const OrderbookIdType orderbook_id)
    -> const std::unique_ptr<Orderbook> & {
    auto iter = orderbooks_.find(orderbook_id);
    if (iter == orderbooks_.end()) {
        // TODO: unhappy path
        assert(0);
    }
    return iter->second;
}

}; // namespace Domain::Market
