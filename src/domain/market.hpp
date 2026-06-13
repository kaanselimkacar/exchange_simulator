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
    PriceLevel(PriceType price) : price_(price) {
    }

    struct QuantityChange {
        QuantityType new_quantity;
        QuantityType old_quantity;
    };

    auto AddOrder(const Order &order) -> OrderListIterator;
    auto UpdateQuantity(const QuantityChange &qty_change) -> void;
    [[nodiscard]] auto IsEmpty() const -> bool;
    auto DeleteOrder(OrderListIterator order_list_iterator) -> void;

  private:
    [[maybe_unused]] PriceType price_;
    QuantityType quantity_{0};
    OrderListType order_list_;
};

class Orderbook {
  public:
    auto AddOrder(const Order &order) -> void;
    auto ModifyOrder(const Order &updated_order) -> void;
    auto DeleteOrder(OrderIdType order_id) -> void;

  private:
    struct OrderLocation {
      public:
        std::reference_wrapper<PriceLevel> price_level_;
        OrderListIterator iterator_;
    };

    static auto ModifyOrderQuantity(QuantityType new_quantity, OrderLocation &old_order_location)
        -> void;

    [[maybe_unused]] OrderbookIdType orderbook_id_{Invalid<OrderbookIdType>};

    std::map<PriceType, PriceLevel, std::greater<>> bids_;
    std::map<PriceType, PriceLevel, std::less<>> asks_;
    std::unordered_map<OrderIdType, OrderLocation> orders_;
};

}; // namespace Domain::Market
