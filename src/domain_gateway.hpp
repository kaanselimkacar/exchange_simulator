#pragma once

#include <domain/matching_engine.hpp>
#include <memory>

namespace Domain {

class DomainGateway {

  public:
    DomainGateway(std::unique_ptr<Market::OrderbookManager> orderbook_manager,
                  std::unique_ptr<MatchingEngine::MatchingEngine> matching_engine)
        : orderbook_manager_(std::move(orderbook_manager)),
          matching_engine_(std::move(matching_engine)) {
    }

    struct DeleteOrderStruct {
        OrderIdType order_id;
        OrderbookIdType orderbook_id;
    };

    auto AddOrder(Order &order, OrderbookIdType orderbook_id) -> void;

    auto DeleteOrder(DeleteOrderStruct delete_order) -> void;

  private:
    auto CheckAndExecuteTrade(Order &order, Market::Orderbook &orderbook) -> void;
    std::unique_ptr<Market::OrderbookManager> orderbook_manager_;
    std::unique_ptr<MatchingEngine::MatchingEngine> matching_engine_;
};

}; // namespace Domain
