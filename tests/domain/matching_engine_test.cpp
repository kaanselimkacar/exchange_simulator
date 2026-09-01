#include "domain/matching_engine.hpp"
#include <gtest/gtest.h>

namespace Domain::MatchingEngine::Testing {

// Test constants to satisfy clang-tidy rules
constexpr PriceType BASE_PRICE = 100;
constexpr PriceType LOWER_PRICE = 99;
constexpr PriceType HIGHER_PRICE = 110;
constexpr QuantityType BASE_QUANTITY = 50;
constexpr QuantityType DOUBLE_QUANTITY = 100;
constexpr TimestampType BASE_TIMESTAMP = 1000;
constexpr TimestampType NEXT_TIMESTAMP = 1001;
constexpr TimestampType LATER_TIMESTAMP = 1002;
constexpr OrderIdType INCOMING_ORDER_1 = 101;
constexpr OrderIdType INCOMING_ORDER_2 = 102;
constexpr OrderbookIdType ORDER_BOOK_ID = 1;

auto IsNoFill(const Domain::Trade &trade) -> bool {
    return trade.bid_order_id == Invalid<OrderIdType> &&
           trade.ask_order_id == Invalid<OrderIdType> && trade.price == Invalid<PriceType> &&
           trade.executed_quantity == Invalid<QuantityType>;
}

auto MakeIncomingOrder(OrderIdType order_id, PriceType price, QuantityType quantity, Side side,
                       TimestampType timestamp) -> Order {
    return Order{.order_id = order_id,
                 .price = price,
                 .quantity = quantity,
                 .side = side,
                 .timestamp = timestamp};
}

void AddRestingAsk(Market::Orderbook &orderbook, OrderIdType order_id, PriceType price,
                   QuantityType quantity, TimestampType timestamp = BASE_TIMESTAMP) {
    orderbook.AddOrder(Order{.order_id = order_id,
                             .price = price,
                             .quantity = quantity,
                             .side = Side::ASK,
                             .timestamp = timestamp});
}

void AddRestingBid(Market::Orderbook &orderbook, OrderIdType order_id, PriceType price,
                   QuantityType quantity, TimestampType timestamp = BASE_TIMESTAMP) {
    orderbook.AddOrder(Order{.order_id = order_id,
                             .price = price,
                             .quantity = quantity,
                             .side = Side::BID,
                             .timestamp = timestamp});
}

class MatchingEngineTest : public ::testing::Test {
  protected:
    Market::Orderbook orderbook_{ORDER_BOOK_ID};
    MatchingEngine engine_{};
};

// ============================================================================
// No Fill Tests
// ============================================================================

TEST_F(MatchingEngineTest, ReturnNoFillWhenBookEmpty) {
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, BASE_PRICE, BASE_QUANTITY, Side::BID,
                                      BASE_TIMESTAMP);

    auto trade = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_TRUE(IsNoFill(trade));
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
}

TEST_F(MatchingEngineTest, ReturnNoFillWhenBidBelowBestAsk) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, LOWER_PRICE, BASE_QUANTITY, Side::BID,
                                      BASE_TIMESTAMP);

    auto trade = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_TRUE(IsNoFill(trade));
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
}

TEST_F(MatchingEngineTest, ReturnNoFillWhenAskAboveBestBid) {
    AddRestingBid(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, HIGHER_PRICE, BASE_QUANTITY, Side::ASK,
                                      BASE_TIMESTAMP);

    auto trade = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_TRUE(IsNoFill(trade));
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
}

// ============================================================================
// Fill Tests
// ============================================================================

TEST_F(MatchingEngineTest, FullFillAtSamePrice) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, BASE_PRICE, BASE_QUANTITY, Side::BID,
                                      BASE_TIMESTAMP);

    auto trade = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(trade.bid_order_id, INCOMING_ORDER_1);
    EXPECT_EQ(trade.ask_order_id, 1);
    EXPECT_EQ(trade.price, BASE_PRICE);
    EXPECT_EQ(trade.executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(trade.timestamp, BASE_TIMESTAMP);
    EXPECT_EQ(incoming.quantity, QuantityType{});
}

TEST_F(MatchingEngineTest, CrossAtBetterPriceUsesRestingPrice) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, HIGHER_PRICE, BASE_QUANTITY, Side::BID,
                                      BASE_TIMESTAMP);

    auto trade = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(trade.ask_order_id, 1);
    EXPECT_EQ(trade.price, BASE_PRICE);
    EXPECT_EQ(trade.executed_quantity, BASE_QUANTITY);
}

TEST_F(MatchingEngineTest, PartiallyFillIncomingOrder) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, BASE_PRICE, DOUBLE_QUANTITY, Side::BID,
                                      BASE_TIMESTAMP);

    auto trade = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(trade.executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
}

TEST_F(MatchingEngineTest, FullyConsumedRestingOrderRemoved) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, BASE_PRICE, DOUBLE_QUANTITY, Side::BID,
                                      BASE_TIMESTAMP);
    engine_.MatchOrders(incoming, orderbook_);

    auto second = MakeIncomingOrder(INCOMING_ORDER_2, BASE_PRICE, DOUBLE_QUANTITY, Side::BID,
                                    NEXT_TIMESTAMP);
    auto trade = engine_.MatchOrders(second, orderbook_);

    EXPECT_TRUE(IsNoFill(trade));
    EXPECT_EQ(second.quantity, DOUBLE_QUANTITY);
}

TEST_F(MatchingEngineTest, LoopSingleTradePerCallAcrossOrders) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(orderbook_, 2, BASE_PRICE, BASE_QUANTITY);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, BASE_PRICE, DOUBLE_QUANTITY, Side::BID,
                                      BASE_TIMESTAMP);

    auto first_fill = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(first_fill.ask_order_id, 1);
    EXPECT_EQ(first_fill.price, BASE_PRICE);
    EXPECT_EQ(first_fill.executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);

    auto second_fill = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(second_fill.ask_order_id, 2);
    EXPECT_EQ(second_fill.executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(incoming.quantity, QuantityType{});
}

TEST_F(MatchingEngineTest, FillBestPriceFirst) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(orderbook_, 2, HIGHER_PRICE, BASE_QUANTITY);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, HIGHER_PRICE, BASE_QUANTITY, Side::BID,
                                      BASE_TIMESTAMP);

    auto first_fill = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(first_fill.ask_order_id, 1);
    EXPECT_EQ(first_fill.price, BASE_PRICE);

    auto second_incoming = MakeIncomingOrder(INCOMING_ORDER_2, HIGHER_PRICE, BASE_QUANTITY,
                                             Side::BID, NEXT_TIMESTAMP);
    auto second_fill = engine_.MatchOrders(second_incoming, orderbook_);

    EXPECT_EQ(second_fill.ask_order_id, 2);
    EXPECT_EQ(second_fill.price, HIGHER_PRICE);
}

TEST_F(MatchingEngineTest, PartialRestingFillKeepsTimePriority) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, DOUBLE_QUANTITY, BASE_TIMESTAMP);
    AddRestingAsk(orderbook_, 2, BASE_PRICE, BASE_QUANTITY, NEXT_TIMESTAMP);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, BASE_PRICE, BASE_QUANTITY, Side::BID,
                                      BASE_TIMESTAMP);

    auto first_fill = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(first_fill.ask_order_id, 1);
    EXPECT_EQ(first_fill.executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(incoming.quantity, QuantityType{});

    auto second_incoming = MakeIncomingOrder(INCOMING_ORDER_2, BASE_PRICE, BASE_QUANTITY,
                                             Side::BID, NEXT_TIMESTAMP);
    auto second_fill = engine_.MatchOrders(second_incoming, orderbook_);

    EXPECT_EQ(second_fill.ask_order_id, 1);
    EXPECT_EQ(second_fill.executed_quantity, BASE_QUANTITY);
}

TEST_F(MatchingEngineTest, MatchAgainstHighestBid) {
    AddRestingBid(orderbook_, 1, BASE_PRICE, BASE_QUANTITY, BASE_TIMESTAMP);
    AddRestingBid(orderbook_, 2, LOWER_PRICE, BASE_QUANTITY, NEXT_TIMESTAMP);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, LOWER_PRICE, BASE_QUANTITY, Side::ASK,
                                      BASE_TIMESTAMP);

    auto trade = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(trade.bid_order_id, 1);
    EXPECT_EQ(trade.ask_order_id, INCOMING_ORDER_1);
    EXPECT_EQ(trade.price, BASE_PRICE);
    EXPECT_EQ(trade.executed_quantity, BASE_QUANTITY);
}

TEST_F(MatchingEngineTest, TradeTimestampMatchesIncoming) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming = MakeIncomingOrder(INCOMING_ORDER_1, BASE_PRICE, BASE_QUANTITY, Side::BID,
                                      LATER_TIMESTAMP);

    auto trade = engine_.MatchOrders(incoming, orderbook_);

    EXPECT_EQ(trade.timestamp, LATER_TIMESTAMP);
}

} // namespace Domain::MatchingEngine::Testing