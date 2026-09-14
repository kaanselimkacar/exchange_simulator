#include "domain/matching_engine.hpp"
#include <array>
#include <gtest/gtest.h>
#include <span>

namespace Domain::MatchingEngine::Testing {

// Test constants to satisfy clang-tidy rules
constexpr PriceType BASE_PRICE = 100;
constexpr PriceType LOWER_PRICE = 99;
constexpr PriceType HIGHER_PRICE = 110;
constexpr PriceType MID_PRICE = 105;
constexpr QuantityType BASE_QUANTITY = 50;
constexpr QuantityType DOUBLE_QUANTITY = 100;
constexpr QuantityType LARGER_QUANTITY = 75;
constexpr TimestampType BASE_TIMESTAMP = 1000;
constexpr TimestampType NEXT_TIMESTAMP = 1001;
constexpr TimestampType LATER_TIMESTAMP = 1002;
constexpr OrderIdType INCOMING_ORDER_1 = 101;
constexpr OrderbookIdType ORDER_BOOK_ID = 1;

auto MakeOrder(OrderIdType order_id, PriceType price, QuantityType quantity, Side side,
               TimestampType timestamp) -> Order {
    return Order{.order_id = order_id,
                 .price = price,
                 .quantity = quantity,
                 .side = side,
                 .timestamp = timestamp};
}

void AddRestingAsk(Market::Orderbook &orderbook, OrderIdType order_id, PriceType price,
                   QuantityType quantity, TimestampType timestamp = BASE_TIMESTAMP) {
    (void)orderbook.AddOrder(MakeOrder(order_id, price, quantity, Side::ASK, timestamp));
}

void AddRestingBid(Market::Orderbook &orderbook, OrderIdType order_id, PriceType price,
                   QuantityType quantity, TimestampType timestamp = BASE_TIMESTAMP) {
    (void)orderbook.AddOrder(MakeOrder(order_id, price, quantity, Side::BID, timestamp));
}

class MatchingEngineTest : public ::testing::Test {
  protected:
    std::array<Trade, MatchingEngine::MAX_TRADES> trades_{};
    Market::Orderbook orderbook_{ORDER_BOOK_ID};
    MatchingEngine engine_{};
};

// ============================================================================
// No Fill Tests
// ============================================================================

TEST_F(MatchingEngineTest, NoFillWhenBookEmptyKeepsIncomingResting) {
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, BASE_PRICE, BASE_QUANTITY, Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    EXPECT_EQ(trades, 0);
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
    auto top_bid = orderbook_.GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, INCOMING_ORDER_1);
}

TEST_F(MatchingEngineTest, NoFillWhenBidBelowBestAsk) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, LOWER_PRICE, BASE_QUANTITY, Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    EXPECT_EQ(trades, 0);
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::ASK).value().order_id, 1);
}

TEST_F(MatchingEngineTest, NoFillWhenAskAboveBestBid) {
    AddRestingBid(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, HIGHER_PRICE, BASE_QUANTITY, Side::ASK, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    EXPECT_EQ(trades, 0);
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).value().order_id, 1);
}

// ============================================================================
// Fill Tests
// ============================================================================

TEST_F(MatchingEngineTest, FullFillAtSamePriceConsumesBothOrders) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, BASE_PRICE, BASE_QUANTITY, Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(trades_[0].bid_order_id, INCOMING_ORDER_1);
    EXPECT_EQ(trades_[0].ask_order_id, 1);
    EXPECT_EQ(trades_[0].price, BASE_PRICE);
    EXPECT_EQ(trades_[0].executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(trades_[0].timestamp, BASE_TIMESTAMP);
    EXPECT_EQ(incoming.quantity, 0);
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::BID).has_value());
}

TEST_F(MatchingEngineTest, CrossTradesAtRestingPrice) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, HIGHER_PRICE, BASE_QUANTITY, Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(trades_[0].ask_order_id, 1);
    EXPECT_EQ(trades_[0].price, BASE_PRICE);
}

TEST_F(MatchingEngineTest, PartialFillLeavesIncomingRemainderResting) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, BASE_PRICE, DOUBLE_QUANTITY, Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(trades_[0].executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
    auto top_bid = orderbook_.GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, INCOMING_ORDER_1);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::ASK).has_value());
}

TEST_F(MatchingEngineTest, SweepConsumesMultipleRestingOrdersInOneCall) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(orderbook_, 2, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, BASE_PRICE, DOUBLE_QUANTITY, Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 2);
    EXPECT_EQ(trades_[0].ask_order_id, 1);
    EXPECT_EQ(trades_[0].executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(trades_[1].ask_order_id, 2);
    EXPECT_EQ(trades_[1].executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(incoming.quantity, 0);
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::BID).has_value());
}

TEST_F(MatchingEngineTest, SweepStopsAtNonCrossingLevel) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(orderbook_, 2, HIGHER_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, MID_PRICE, DOUBLE_QUANTITY, Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
    auto top_ask = orderbook_.GetTopOrder(Side::ASK);
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 2);
    EXPECT_EQ(top_ask.value().quantity, BASE_QUANTITY);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).value().order_id, INCOMING_ORDER_1);
}

TEST_F(MatchingEngineTest, FillsBestPriceLevelFirst) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(orderbook_, 2, HIGHER_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, HIGHER_PRICE, BASE_QUANTITY, Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(trades_[0].ask_order_id, 1);
    EXPECT_EQ(trades_[0].price, BASE_PRICE);
}

TEST_F(MatchingEngineTest, EarliestOrderWinsWithinPriceLevel) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, DOUBLE_QUANTITY, BASE_TIMESTAMP);
    AddRestingAsk(orderbook_, 2, BASE_PRICE, BASE_QUANTITY, NEXT_TIMESTAMP);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, BASE_PRICE, BASE_QUANTITY, Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(trades_[0].ask_order_id, 1);
}

TEST_F(MatchingEngineTest, SweepPartiallyFillsLastRestingOrder) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, DOUBLE_QUANTITY, BASE_TIMESTAMP);
    AddRestingAsk(orderbook_, 2, BASE_PRICE, DOUBLE_QUANTITY, NEXT_TIMESTAMP);
    auto incoming = MakeOrder(INCOMING_ORDER_1, BASE_PRICE, DOUBLE_QUANTITY + LARGER_QUANTITY,
                              Side::BID, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 2);
    EXPECT_EQ(trades_[0].ask_order_id, 1);
    EXPECT_EQ(trades_[0].executed_quantity, DOUBLE_QUANTITY);
    EXPECT_EQ(trades_[1].ask_order_id, 2);
    EXPECT_EQ(trades_[1].executed_quantity, LARGER_QUANTITY);
    EXPECT_EQ(incoming.quantity, 0);
    auto top_ask = orderbook_.GetTopOrder(Side::ASK);
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 2);
    EXPECT_EQ(top_ask.value().quantity, DOUBLE_QUANTITY - LARGER_QUANTITY);
}

TEST_F(MatchingEngineTest, TradeTimestampMatchesIncoming) {
    AddRestingAsk(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, BASE_PRICE, BASE_QUANTITY, Side::BID, LATER_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(trades_[0].timestamp, LATER_TIMESTAMP);
}

// ============================================================================
// ASK-side Fill Tests
// ============================================================================

TEST_F(MatchingEngineTest, AskFullFillAtSamePrice) {
    AddRestingBid(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, BASE_PRICE, BASE_QUANTITY, Side::ASK, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(trades_[0].bid_order_id, 1);
    EXPECT_EQ(trades_[0].ask_order_id, INCOMING_ORDER_1);
    EXPECT_EQ(trades_[0].price, BASE_PRICE);
    EXPECT_EQ(trades_[0].executed_quantity, BASE_QUANTITY);
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::BID).has_value());
}

TEST_F(MatchingEngineTest, AskCrossTradesAtRestingPrice) {
    AddRestingBid(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, LOWER_PRICE, BASE_QUANTITY, Side::ASK, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(trades_[0].bid_order_id, 1);
    EXPECT_EQ(trades_[0].price, BASE_PRICE);
}

TEST_F(MatchingEngineTest, AskPartialFillLeavesIncomingRemainderResting) {
    AddRestingBid(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, BASE_PRICE, DOUBLE_QUANTITY, Side::ASK, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 1);
    EXPECT_EQ(trades_[0].executed_quantity, BASE_QUANTITY);
    EXPECT_EQ(incoming.quantity, BASE_QUANTITY);
    auto top_ask = orderbook_.GetTopOrder(Side::ASK);
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, INCOMING_ORDER_1);
    EXPECT_EQ(top_ask.value().quantity, BASE_QUANTITY);
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::BID).has_value());
}

TEST_F(MatchingEngineTest, AskSweepConsumesMultipleRestingBids) {
    AddRestingBid(orderbook_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingBid(orderbook_, 2, BASE_PRICE, BASE_QUANTITY);
    auto incoming =
        MakeOrder(INCOMING_ORDER_1, BASE_PRICE, DOUBLE_QUANTITY, Side::ASK, BASE_TIMESTAMP);
    (void)orderbook_.AddOrder(incoming);

    auto trades = engine_.MatchOrders(incoming, orderbook_, trades_);

    ASSERT_EQ(trades, 2);
    EXPECT_EQ(trades_[0].bid_order_id, 1);
    EXPECT_EQ(trades_[1].bid_order_id, 2);
    EXPECT_EQ(incoming.quantity, 0);
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::BID).has_value());
}

} // namespace Domain::MatchingEngine::Testing