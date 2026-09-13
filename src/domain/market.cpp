#include "market.hpp"
#include "domain_types.hpp"
#include <cassert>
#include <common/logger.hpp>
#include <utility>

namespace Domain::Market {

auto PriceLevel::AddOrder(const Order &order) -> OrderListIterator {
    quantity_ += order.quantity;
    return order_list_.emplace(order_list_.end(), order);
};

auto PriceLevel::UpdateQuantity(const QuantityChange &qty_change) noexcept -> StatusCode {
    if (qty_change.modified_order_old_quantity > quantity_) [[unlikely]] {
        return StatusCode::InvalidQty;
    }
    quantity_ += qty_change.modified_order_new_quantity;
    quantity_ -= qty_change.modified_order_old_quantity;
    return StatusCode::Success;
}

auto PriceLevel::IsEmpty() const noexcept -> bool {
    return order_list_.empty();
};

auto PriceLevel::DeleteOrder(OrderListIterator order_list_iterator) -> void {
    if (quantity_ < order_list_iterator->quantity) [[unlikely]] {
        std::unreachable();
    }
    quantity_ -= order_list_iterator->quantity;
    order_list_.erase(order_list_iterator);
};

auto PriceLevel::GetTopOrder() const noexcept -> Order {
    if (order_list_.empty()) [[unlikely]] {
        std::unreachable();
    }
    return *order_list_.begin();
}

auto Orderbook::AddOrder(const Order &order) -> StatusCode {
    auto add_order = [](const Order &order, auto &side_level, auto &orders_) -> void {
        const auto order_price = order.price;
        auto [iter, inserted] = side_level.try_emplace(order_price, order_price);

        PriceLevel &price_level = iter->second;
        auto order_iter = price_level.AddOrder(order);
        orders_.emplace(order.order_id,
                        OrderLocation{.price_level_ = price_level, .iterator_ = order_iter});
    };

    auto validation_res = ValidateOrder(order);
    if (validation_res != StatusCode::Success) [[unlikely]] {
        return validation_res;
    }
    if (orders_.contains(order.order_id)) [[unlikely]] {
        return StatusCode::DuplicateOrderId;
    }

    if (order.side == Side::ASK) {
        add_order(order, asks_, orders_);
    } else {
        add_order(order, bids_, orders_);
    }
    LogInfo("Order[{}] added to orderbook[{}]", order.order_id, orderbook_id_);
    return StatusCode::Success;
};

auto Orderbook::ModifyOrder(const Order &updated_order) -> StatusCode {
    auto old_order_iter = orders_.find(updated_order.order_id);
    if (old_order_iter == orders_.end()) [[unlikely]] {
        return StatusCode::OrderNotFound;
    }

    auto &old_order_location = old_order_iter->second;
    auto old_price = old_order_location.iterator_->price;
    auto old_side = old_order_location.iterator_->side;

    if (updated_order.quantity <= 0 || updated_order.quantity == Invalid<QuantityType>)
        [[unlikely]] {
        return StatusCode::InvalidQty;
    }

    if (old_side != updated_order.side) [[unlikely]] {
        return StatusCode::InvalidSide;
    }
    // check if there is a update to price
    if (updated_order.price != Invalid<PriceType> && updated_order.price != old_price) {
        if (updated_order.price <= 0) [[unlikely]] {
            return StatusCode::InvalidPrice;
        }
        // perform delete & add
        auto status_res = DeleteOrder(updated_order.order_id);
        if (status_res != StatusCode::Success) [[unlikely]] {
            return status_res;
        }
        return AddOrder(updated_order);
    }
    return ModifyOrderQuantity(updated_order.quantity, old_order_location);
};

auto Orderbook::DeleteOrder(const OrderIdType order_id) -> StatusCode {
    auto order_iter = orders_.find(order_id);

    if (order_iter == orders_.end()) [[unlikely]] {
        return StatusCode::OrderNotFound;
    }
    auto &order_location = order_iter->second;
    auto &price_level = order_location.price_level_.get();
    auto side = order_location.iterator_->side;

    if (side == Side::INVALID) [[unlikely]] {
        return StatusCode::InvalidSide;
    }

    auto price = order_location.iterator_->price;

    price_level.DeleteOrder(order_location.iterator_);

    if (price_level.IsEmpty()) {
        if (side == Side::ASK) {
            asks_.erase(price);
        } else {
            bids_.erase(price);
        }
    }
    orders_.erase(order_iter);
    return StatusCode::Success;
}

auto Orderbook::ExecuteTrade(TradeExec trade_exec, QuantityType exec_qty) -> StatusCode {
    auto update_qty = [&](OrderIdType order_id, QuantityType exec_qty) -> StatusCode {
        auto iter = orders_.find(order_id);
        if (iter == orders_.end()) [[unlikely]] {
            std::unreachable();
        }
        auto curr_qty = iter->second.iterator_->quantity;
        if (curr_qty < exec_qty) [[unlikely]] {
            std::unreachable();
        }

        if (curr_qty == exec_qty) {
            return DeleteOrder(order_id);
        }
        return ModifyOrderQuantity(curr_qty - exec_qty, iter->second);
    };

    auto status_res = update_qty(trade_exec.ask_id, exec_qty);
    if (status_res != StatusCode::Success) [[unlikely]] {
        return status_res;
    }
    return update_qty(trade_exec.bid_id, exec_qty);
}

auto Orderbook::ModifyOrderQuantity(QuantityType new_quantity, OrderLocation &old_order_location)
    -> StatusCode {
    auto old_quantity = old_order_location.iterator_->quantity;
    auto status_res = old_order_location.price_level_.get().UpdateQuantity(
        {.modified_order_new_quantity = new_quantity, .modified_order_old_quantity = old_quantity});
    if (status_res != StatusCode::Success) [[unlikely]] {
        return status_res;
    }
    old_order_location.iterator_->quantity = new_quantity;
    return StatusCode::Success;
}

auto Orderbook::GetTopOrder(const Side &side) const -> std::expected<Order, StatusCode> {
    const auto get_top_order = [](const auto &side_map) -> std::expected<Order, StatusCode> {
        if (side_map.empty()) [[unlikely]] {
            return make_error<StatusCode::OrderNotFound>();
        }
        return side_map.begin()->second.GetTopOrder();
    };
    return side == Side::ASK ? get_top_order(asks_) : get_top_order(bids_);
}

auto Orderbook::ValidateOrder(const Order &order) noexcept -> StatusCode {
    if (order.quantity <= 0 || order.quantity == Invalid<QuantityType>) [[unlikely]] {
        return StatusCode::InvalidQty;
    }
    if (order.side == Side::INVALID) [[unlikely]] {
        return StatusCode::InvalidSide;
    }
    if (order.order_id <= 0 || order.order_id == Invalid<OrderIdType>) [[unlikely]] {
        return StatusCode::InvalidOrderId;
    }
    if (order.price <= 0 || order.price == Invalid<PriceType>) [[unlikely]] {
        return StatusCode::InvalidPrice;
    }
    return StatusCode::Success;
}

auto OrderbookManager::AddOrderbook(std::unique_ptr<Orderbook> orderbook) -> StatusCode {
    const auto orderbook_id = orderbook->GetOrderbookId();
    auto result = orderbooks_.try_emplace(orderbook_id, std::move(orderbook));
    if (!result.second) [[unlikely]] {
        return StatusCode::DuplicateOrderbookId;
    }
    return StatusCode::Success;
}

auto OrderbookManager::GetOrderbook(const OrderbookIdType orderbook_id) const
    -> std::expected<Orderbook *, StatusCode> {
    auto iter = orderbooks_.find(orderbook_id);
    if (iter == orderbooks_.end()) [[unlikely]] {
        return make_error<StatusCode::OrderbookNotFound>();
    }
    return iter->second.get();
}

}; // namespace Domain::Market
