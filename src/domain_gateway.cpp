#include "domain_gateway.hpp"
#include <cassert>

namespace Domain {

auto DomainGateway::AddOrder(Order &order, const OrderbookIdType orderbook_id) -> void {
    auto orderbook_res = orderbook_manager_->GetOrderbook(orderbook_id);
    if (!orderbook_res.has_value()) {
        RejectOrder(order, orderbook_id, orderbook_res.error());
        return;
    }
    auto *orderbook_ptr = orderbook_res.value();
    auto add_order_res = orderbook_ptr->AddOrder(order);
    if (add_order_res != StatusCode::Success) {
        RejectOrder(order, orderbook_id, add_order_res);
        return;
    }
    CheckAndExecuteTrade(order, *orderbook_ptr);
}

auto DomainGateway::ModifyOrder(Order &updated_order, OrderbookIdType orderbook_id) -> void {
    auto orderbook_res = orderbook_manager_->GetOrderbook(orderbook_id);
    if (!orderbook_res.has_value()) {
        RejectOrder(updated_order, orderbook_id, orderbook_res.error());
        return;
    }
    auto *orderbook_ptr = orderbook_res.value();
    auto modify_order_res = orderbook_ptr->ModifyOrder(updated_order);
    if (modify_order_res != StatusCode::Success) {
        RejectOrder(updated_order, orderbook_id, modify_order_res);
        return;
    }
    CheckAndExecuteTrade(updated_order, *orderbook_ptr);
}

auto DomainGateway::DeleteOrder(Order &order, OrderbookIdType orderbook_id) -> void {
    auto orderbook_res = orderbook_manager_->GetOrderbook(orderbook_id);
    if (!orderbook_res.has_value()) {
        RejectOrder(order, orderbook_id, orderbook_res.error());
        return;
    }
    auto *orderbook_ptr = orderbook_res.value();
    auto delete_order_res = orderbook_ptr->DeleteOrder(order.order_id);
    if (delete_order_res != StatusCode::Success) {
        RejectOrder(order, orderbook_id, delete_order_res);
        return;
    }
}

auto DomainGateway::CheckAndExecuteTrade(Order &order, Market::Orderbook &orderbook) -> void {

    [[maybe_unused]] size_t no_trades =
        matching_engine_->MatchOrders(order, orderbook, std::span{trades_});
    // TODO: missing a lot of stuff here!
}

auto DomainGateway::RejectOrder([[maybe_unused]] Order &order,
                                [[maybe_unused]] OrderbookIdType orderbook_id,
                                [[maybe_unused]] StatusCode status_code) -> void {
    // TODO:
}

}; // namespace Domain
