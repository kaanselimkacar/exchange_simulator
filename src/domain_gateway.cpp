#include "domain_gateway.hpp"
#include <cassert>

namespace Domain {

auto DomainGateway::AddOrder(Order &order, const OrderbookIdType orderbook_id) -> void {
    const auto &orderbook = orderbook_manager_->GetOrderbook(orderbook_id);
    orderbook->AddOrder(order);
    CheckAndExecuteTrade(order, *orderbook);
}

auto DomainGateway::ModifyOrder(Order &updated_order, OrderbookIdType orderbook_id) -> void {
    const auto &orderbook = orderbook_manager_->GetOrderbook(orderbook_id);
    orderbook->ModifyOrder(updated_order);
    CheckAndExecuteTrade(updated_order, *orderbook);
}

auto DomainGateway::DeleteOrder(DeleteOrderStruct delete_order) -> void {
    const auto &orderbook = orderbook_manager_->GetOrderbook(delete_order.orderbook_id);
    orderbook->DeleteOrder(delete_order.order_id);
}

auto DomainGateway::CheckAndExecuteTrade(Order &order, Market::Orderbook &orderbook) -> void {

    auto trade = matching_engine_->MatchOrders(order, orderbook);
    while (trade.ask_order_id != Invalid<OrderIdType> && order.quantity > 0) {
        orderbook.ExecuteTrade({.ask_id_ = trade.ask_order_id, .bid_id_ = trade.bid_order_id},
                               trade.executed_quantity);
        // TODO: better assertion needed
        assert(order.quantity >= trade.executed_quantity);

        order.quantity -= trade.executed_quantity;
        trade = matching_engine_->MatchOrders(order, orderbook);
    }
    // TODO: missing a lot of stuff here!
}

}; // namespace Domain
