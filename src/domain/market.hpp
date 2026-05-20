#pragma once
#include "domain_types.hpp"
#include <functional>
#include <list>
#include <map>
#include <unordered_map>

namespace Domain::Market {

using OrderListType = std::list<Order>;
using OrderListIterator = std::list<Order>::iterator;

class PriceLevel {
  public:
    auto AddOrder(const Order &order) -> OrderListIterator;
    PriceLevel(PriceType price) : price_(price) {
    }

  private:
    PriceType price_;
    QuantityType quantity_{Invalid<QuantityType>};
    OrderListType order_list_;
};

class Orderbook {
  public:
    auto AddOrder(const Order &order) -> void;

  private:
    struct OrderLocation {
      public:
        std::reference_wrapper<PriceLevel> price_level_;
        OrderListIterator iterator_;
    };

    OrderbookIdType orderbook_id_{Invalid<OrderbookIdType>};

    std::map<PriceType, PriceLevel, std::greater<>> bids_;
    std::map<PriceType, PriceLevel, std::less<>> asks_;
    std::unordered_map<OrderIdType, OrderLocation> orders_;
};

}; // namespace Domain::Market
