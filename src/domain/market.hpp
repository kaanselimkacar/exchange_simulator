#pragma once
#include "domain_types.hpp"
#include <functional>
#include <list>
#include <map>
#include <memory>
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

    [[nodiscard]] auto GetTopOrder() const -> Order;
    [[nodiscard]] auto GetPrice() const -> PriceType {
        return price_;
    };

  private:
    PriceType price_;
    QuantityType quantity_{0};
    OrderListType order_list_;
};

class Orderbook {
  public:
    Orderbook(OrderbookIdType orderbook_id) : orderbook_id_(orderbook_id) {
    }
    auto AddOrder(const Order &order) -> void;
    auto ModifyOrder(const Order &updated_order) -> void;
    auto DeleteOrder(OrderIdType order_id) -> void;

    // TODO: should these take order references for performance reasons?
    struct TradeExec {
        OrderIdType ask_id_;
        OrderIdType bid_id_;
    };
    auto ExecuteTrade(TradeExec trade_exec, QuantityType exec_qty) -> void;

    [[nodiscard]] auto GetTopOrder(const Side &side) const -> Order;
    [[nodiscard]] auto GetOrderbookId() const -> OrderbookIdType {
        return orderbook_id_;
    }

  private:
    struct OrderLocation {
      public:
        std::reference_wrapper<PriceLevel> price_level_;
        OrderListIterator iterator_;
    };

    static auto ModifyOrderQuantity(QuantityType new_quantity, OrderLocation &old_order_location)
        -> void;

    static auto ValidateOrder(const Order &order) -> void;

    OrderbookIdType orderbook_id_;

    std::map<PriceType, PriceLevel, std::greater<>> bids_;
    std::map<PriceType, PriceLevel, std::less<>> asks_;
    std::unordered_map<OrderIdType, OrderLocation> orders_;
};

class OrderbookManager {
  public:
    auto AddOrderbook(std::unique_ptr<Orderbook> orderbook) -> void;

    [[nodiscard]] auto GetOrderbook(OrderbookIdType orderbook_id)
        -> const std::unique_ptr<Orderbook> &;

  private:
    std::unordered_map<OrderbookIdType, std::unique_ptr<Orderbook>> orderbooks_;
};
}; // namespace Domain::Market
