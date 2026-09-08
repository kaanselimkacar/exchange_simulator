#include "top_level.hpp"
#include <gtest/gtest.h>

namespace Domain::TopLevel::Testing {

// Test constants to satisfy clang-tidy rules
constexpr PriceType BASE_PRICE = 100;
constexpr QuantityType BASE_QUANTITY = 50;
constexpr QuantityType DOUBLE_QUANTITY = 100;
constexpr TimestampType BASE_TIMESTAMP = 1000;
constexpr TimestampType NEXT_TIMESTAMP = 1001;
constexpr OrderIdType INCOMING_ORDER_1 = 101;
constexpr OrderIdType INCOMING_ORDER_2 = 102;
constexpr OrderbookIdType ORDER_BOOK_ID = 1;

// TODO(top_level): un-skip these tests once src/top_level.hpp exposes a real
// public API. The behaviors they pin (resting-order removal, looping an
// incoming order across successive resting fills, leftover recomputation,
// time-priority across partial resting fills) are owned by top_level, not by
// the matching engine. Today there is no such API, so the bodies are written
// against the matching engine + orderbook and must be re-targeted at top_level.

class TopLevelTest : public ::testing::Test {
  protected:
    Market::Orderbook orderbook_{ORDER_BOOK_ID};
    Domain::MatchingEngine::MatchingEngine engine_{};
};

void AddRestingAsk(Market::Orderbook &orderbook, OrderIdType order_id, PriceType price,
                   QuantityType quantity, TimestampType timestamp = BASE_TIMESTAMP) {
    orderbook.AddOrder(Order{.order_id = order_id,
                             .price = price,
                             .quantity = quantity,
                             .side = Side::ASK,
                             .timestamp = timestamp});
}

TEST_F(TopLevelTest, FullyConsumedRestingOrderRemoved) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    GTEST_SKIP();

    // top_level must consume the resting ask and remove it from the book, so
    // the second incoming order finds an empty book.
    engine_.MatchOrders(incoming, orderbook_);
    auto second_incoming = incoming;
    second_incoming.order_id = INCOMING_ORDER_2;
    second_incoming.timestamp = NEXT_TIMESTAMP;
    auto trade = engine_.MatchOrders(second_incoming, orderbook_);

    EXPECT_TRUE(trade.bid_order_id == Invalid<OrderIdType>);
    EXPECT_TRUE(trade.ask_order_id == Invalid<OrderIdType>);
}

TEST_F(TopLevelTest, LoopSingleTradePerCallAcrossOrders) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(orderbook_, 2, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    GTEST_SKIP();

    // top_level loops MatchOrders until the incoming order is fully filled,
    // updating the book between calls.
    auto first_fill = engine_.MatchOrders(incoming, orderbook_);
    auto second_fill = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(first_fill.ask_order_id, 1);
    EXPECT_EQ(second_fill.ask_order_id, 2);
}

TEST_F(TopLevelTest, LeftoverRecomputedAfterPartialFill) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    GTEST_SKIP();

    // top_level owns the leftover quantity, not the matching engine.
    engine_.MatchOrders(incoming, orderbook_);
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
}

TEST_F(TopLevelTest, PartialRestingFillKeepsTimePriority) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, DOUBLE_QUANTITY, BASE_TIMESTAMP);
    AddRestingAsk(orderbook_, 2, BASE_PRICE, BASE_QUANTITY, NEXT_TIMESTAMP);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    GTEST_SKIP();

    // top_level must update the partially filled resting order in place so the
    // earliest order keeps time priority for the next match.
    engine_.MatchOrders(incoming, orderbook_);
    auto second_incoming = incoming;
    second_incoming.order_id = INCOMING_ORDER_2;
    second_incoming.timestamp = NEXT_TIMESTAMP;
    auto second_fill = engine_.MatchOrders(second_incoming, orderbook_);

    EXPECT_EQ(second_fill.ask_order_id, 1);
}

} // namespace TopLevel::Testing