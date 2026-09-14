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
    PriceLevel(PriceType price) noexcept : price_(price) {
    }

    struct QuantityChange {
        QuantityType modified_order_new_quantity;
        QuantityType modified_order_old_quantity;
    };

    // TODO: wrap the orderlist and orderlist iterators
    [[nodiscard]] auto AddOrder(const Order &order) -> OrderListIterator;
    [[nodiscard]] auto UpdateQuantity(const QuantityChange &qty_change) noexcept -> StatusCode;
    [[nodiscard]] auto IsEmpty() const noexcept -> bool;
    auto DeleteOrder(OrderListIterator order_list_iterator) -> void;

    // TODO: this shouldn't return a copy !!!!!!!!
    [[nodiscard]] auto GetTopOrder() const noexcept -> Order;
    [[nodiscard]] auto GetPrice() const noexcept -> PriceType {
        return price_;
    };

  private:
    PriceType price_;
    QuantityType quantity_{0};
    OrderListType order_list_;
};

class Orderbook {
  public:
    Orderbook(OrderbookIdType orderbook_id) noexcept : orderbook_id_(orderbook_id) {
    }
    [[nodiscard]] auto AddOrder(const Order &order) -> StatusCode;
    [[nodiscard]] auto ModifyOrder(const Order &updated_order) -> StatusCode;
    [[nodiscard]] auto DeleteOrder(OrderIdType order_id) -> StatusCode;

    // TODO: should these take order references for performance reasons?
    struct TradeExec {
        OrderIdType ask_id;
        OrderIdType bid_id;
    };
    [[nodiscard]] auto ExecuteTrade(TradeExec trade_exec, QuantityType exec_qty) -> StatusCode;

    [[nodiscard]] auto GetTopOrder(const Side &side) const -> std::expected<Order, StatusCode>;
    [[nodiscard]] auto GetOrderbookId() const -> OrderbookIdType {
        return orderbook_id_;
    }

  private:
    struct OrderLocation {
      public:
        std::reference_wrapper<PriceLevel> price_level_;
        OrderListIterator iterator_;
    };

    [[nodiscard]] static auto ModifyOrderQuantity(QuantityType new_quantity,
                                                  OrderLocation &old_order_location) -> StatusCode;

    [[nodiscard]] static auto ValidateOrder(const Order &order) noexcept -> StatusCode;

    OrderbookIdType orderbook_id_;

    std::map<PriceType, PriceLevel, std::greater<>> bids_;
    std::map<PriceType, PriceLevel, std::less<>> asks_;
    std::unordered_map<OrderIdType, OrderLocation> orders_;
};

class OrderbookManager {
  public:
    [[nodiscard]] auto AddOrderbook(std::unique_ptr<Orderbook> orderbook) -> StatusCode;

    [[nodiscard]] auto GetOrderbook(OrderbookIdType orderbook_id) const
        -> std::expected<Orderbook *, StatusCode>;

  private:
    std::unordered_map<OrderbookIdType, std::unique_ptr<Orderbook>> orderbooks_;
};
}; // namespace Domain::Market
