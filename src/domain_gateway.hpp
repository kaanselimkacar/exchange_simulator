#pragma once

#include <array>
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

    auto AddOrder(Order &order, OrderbookIdType orderbook_id) -> void;

    auto ModifyOrder(Order &updated_order, OrderbookIdType orderbook_id) -> void;

    auto DeleteOrder(Order &order, OrderbookIdType orderbook_id) -> void;

  private:
    std::array<Trade, MatchingEngine::MatchingEngine::MAX_TRADES> trades_;
    auto CheckAndExecuteTrade(Order &order, Market::Orderbook &orderbook) -> void;
    auto RejectOrder(Order &order, OrderbookIdType orderbook_id, StatusCode status_code) -> void;
    std::unique_ptr<Market::OrderbookManager> orderbook_manager_;
    std::unique_ptr<MatchingEngine::MatchingEngine> matching_engine_;
};

}; // namespace Domain
