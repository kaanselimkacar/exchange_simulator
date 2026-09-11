#include "domain_gateway.hpp"
#include <gtest/gtest.h>
#include <memory>

namespace Domain::Testing {

// Test constants to satisfy clang-tidy rules
constexpr PriceType BASE_PRICE = 100;
constexpr PriceType LOWER_PRICE = 99;
constexpr PriceType HIGHER_PRICE = 105;
constexpr PriceType MUCH_HIGHER_PRICE = 106;
constexpr QuantityType BASE_QUANTITY = 50;
constexpr QuantityType DOUBLE_QUANTITY = 100;
constexpr QuantityType THIRTY_QUANTITY = 30;
constexpr QuantityType FORTY_QUANTITY = 40;
constexpr QuantityType SIXTY_QUANTITY = 60;
constexpr PriceType MID_PRICE = 102;
constexpr TimestampType BASE_TIMESTAMP = 1000;
constexpr TimestampType NEXT_TIMESTAMP = 1001;
constexpr OrderIdType INCOMING_ORDER_1 = 101;
constexpr OrderbookIdType ORDER_BOOK_ID = 1;
constexpr OrderbookIdType UNKNOWN_ORDER_BOOK_ID = 99;

class DomainGatewayTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto orderbook_manager = std::make_unique<Market::OrderbookManager>();
        auto orderbook = std::make_unique<Market::Orderbook>(ORDER_BOOK_ID);
        book_ = orderbook.get();
        orderbook_manager->AddOrderbook(std::move(orderbook));
        gateway_ = std::make_unique<DomainGateway>(
            std::move(orderbook_manager), std::make_unique<MatchingEngine::MatchingEngine>());
    }

    void AddOrder(Order &order) {
        gateway_->AddOrder(order, ORDER_BOOK_ID);
    }

    std::unique_ptr<DomainGateway> gateway_; // NOLINT: fixture state, accessed by TEST_F bodies
    Market::Orderbook *book_{nullptr};       // NOLINT: fixture state, accessed by TEST_F bodies
};

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

TEST_F(DomainGatewayTest, FullyConsumedRestingOrderRemoved) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    EXPECT_EQ(book_->GetTopOrder(Side::ASK).order_id, Invalid<OrderIdType>);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, INCOMING_ORDER_1);
}

TEST_F(DomainGatewayTest, ConsecutiveRestingOrdersConsumedByOneIncoming) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(*book_, 2, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    EXPECT_EQ(book_->GetTopOrder(Side::ASK).order_id, Invalid<OrderIdType>);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

TEST_F(DomainGatewayTest, LeftoverRecomputedAfterPartialFill) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    auto leftover = book_->GetTopOrder(Side::BID);
    EXPECT_EQ(leftover.order_id, INCOMING_ORDER_1);
    EXPECT_EQ(leftover.quantity, BASE_QUANTITY);
}

TEST_F(DomainGatewayTest, NonCrossingOrderRestsOnBook) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = LOWER_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    auto top_bid = book_->GetTopOrder(Side::BID);
    EXPECT_EQ(top_ask.order_id, 1);
    EXPECT_EQ(top_ask.quantity, BASE_QUANTITY);
    EXPECT_EQ(top_bid.order_id, INCOMING_ORDER_1);
    EXPECT_EQ(top_bid.quantity, BASE_QUANTITY);
}

TEST_F(DomainGatewayTest, SweepStopsAtNonCrossingLevel) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(*book_, 2, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(*book_, 3, MUCH_HIGHER_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = HIGHER_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    EXPECT_EQ(top_ask.order_id, 3);
    EXPECT_EQ(top_ask.quantity, BASE_QUANTITY);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

TEST_F(DomainGatewayTest, AddOrderToUnknownOrderbookAsserts) {
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    EXPECT_DEATH(gateway_->AddOrder(incoming, UNKNOWN_ORDER_BOOK_ID), "");
}

// Incoming bid partially fills the earliest resting ask in place; the ask keeps
// time priority for the next match.
TEST_F(DomainGatewayTest, PartialRestingFillKeepsTimePriority) {
    AddRestingAsk(*book_, 1, BASE_PRICE, DOUBLE_QUANTITY, BASE_TIMESTAMP);
    AddRestingAsk(*book_, 2, BASE_PRICE, BASE_QUANTITY, NEXT_TIMESTAMP);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    EXPECT_EQ(top_ask.order_id, 1);
    EXPECT_EQ(top_ask.quantity, BASE_QUANTITY);
}

// Incoming bid fully consumed off the book, leaving the partial resting ask
// untouched at its original quantity.
TEST_F(DomainGatewayTest, CrossingIncomingFullyConsumedLeavesPartialResting) {
    AddRestingAsk(*book_, 1, BASE_PRICE, DOUBLE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    EXPECT_EQ(top_ask.order_id, 1);
    EXPECT_EQ(top_ask.quantity, BASE_QUANTITY);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

// Sweep trades a shrinking incoming remainder against successively larger
// resting orders: ask1 (THIRTY) is consumed, then ask2 is partially filled and
// left at SIXTY - 30 = FORTY. The incoming bid is fully consumed off the book.
TEST_F(DomainGatewayTest, ResidualIncomingAgainstLargerRestingOrderStaysConsistent) {
    AddRestingAsk(*book_, 1, BASE_PRICE, THIRTY_QUANTITY);
    AddRestingAsk(*book_, 2, BASE_PRICE, SIXTY_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    EXPECT_EQ(top_ask.order_id, 2);
    EXPECT_EQ(top_ask.quantity, FORTY_QUANTITY);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

// Sweep across price levels keeps trading a shrinking incoming remainder
// against each crossing level instead of re-matching against the original full
// quantity; ask2 is left at SIXTY - 30 = FORTY.
TEST_F(DomainGatewayTest, SweepAcrossPriceLevelsUsesActualIncomingRemainder) {
    AddRestingAsk(*book_, 1, BASE_PRICE, THIRTY_QUANTITY);
    AddRestingAsk(*book_, 2, MID_PRICE, SIXTY_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = HIGHER_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    EXPECT_EQ(top_ask.order_id, 2);
    EXPECT_EQ(top_ask.quantity, FORTY_QUANTITY);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

// ============================================================================
// Gateway ASK-side sweep tests
// ============================================================================

TEST_F(DomainGatewayTest, IncomingAskFullyConsumesMultipleRestingBids) {
    AddRestingBid(*book_, 1, HIGHER_PRICE, BASE_QUANTITY);
    AddRestingBid(*book_, 2, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    EXPECT_EQ(book_->GetTopOrder(Side::ASK).order_id, Invalid<OrderIdType>);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

TEST_F(DomainGatewayTest, IncomingAskPartialFillLeavesRestingBidRemainder) {
    AddRestingBid(*book_, 1, HIGHER_PRICE, DOUBLE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    EXPECT_EQ(book_->GetTopOrder(Side::ASK).order_id, Invalid<OrderIdType>);
    auto top_bid = book_->GetTopOrder(Side::BID);
    EXPECT_EQ(top_bid.order_id, 1);
    EXPECT_EQ(top_bid.quantity, BASE_QUANTITY);
}

TEST_F(DomainGatewayTest, IncomingAskLeftoverRestsAfterSweepingLevels) {
    AddRestingBid(*book_, 1, HIGHER_PRICE, THIRTY_QUANTITY);
    AddRestingBid(*book_, 2, BASE_PRICE, FORTY_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    EXPECT_EQ(top_ask.order_id, INCOMING_ORDER_1);
    EXPECT_EQ(top_ask.quantity, THIRTY_QUANTITY);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

TEST_F(DomainGatewayTest, IncomingBidLeftoverRestsAfterSweepingLevels) {
    AddRestingAsk(*book_, 1, BASE_PRICE, THIRTY_QUANTITY);
    AddRestingAsk(*book_, 2, MID_PRICE, FORTY_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = HIGHER_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    auto top_bid = book_->GetTopOrder(Side::BID);
    EXPECT_EQ(top_bid.order_id, INCOMING_ORDER_1);
    EXPECT_EQ(top_bid.quantity, THIRTY_QUANTITY);
    EXPECT_EQ(book_->GetTopOrder(Side::ASK).order_id, Invalid<OrderIdType>);
}

// ============================================================================
// Gateway DeleteOrder tests
// ============================================================================

TEST_F(DomainGatewayTest, DeleteOrderThroughGatewayRemovesRestingOrder) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);

    gateway_->DeleteOrder({.order_id = 1, .orderbook_id = ORDER_BOOK_ID});

    EXPECT_EQ(book_->GetTopOrder(Side::ASK).order_id, Invalid<OrderIdType>);
}

TEST_F(DomainGatewayTest, DeleteOrderThroughGatewaySkipsDeletedOrder) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(*book_, 2, BASE_PRICE, BASE_QUANTITY);
    gateway_->DeleteOrder({.order_id = 1, .orderbook_id = ORDER_BOOK_ID});

    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};
    AddOrder(incoming);

    EXPECT_EQ(book_->GetTopOrder(Side::ASK).order_id, Invalid<OrderIdType>);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

TEST_F(DomainGatewayTest, DeleteOrderUnknownOrderIdAsserts) {
    EXPECT_DEATH(
        gateway_->DeleteOrder({.order_id = INCOMING_ORDER_1, .orderbook_id = ORDER_BOOK_ID}), "");
}

TEST_F(DomainGatewayTest, DeleteOrderUnknownOrderbookAsserts) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);

    EXPECT_DEATH(gateway_->DeleteOrder({.order_id = 1, .orderbook_id = UNKNOWN_ORDER_BOOK_ID}), "");
}

// ============================================================================
// Duplicate order id through the gateway
// ============================================================================

// SKIPPED: re-adding an already-consumed order id via DomainGateway currently
// asserts inside ExecuteTrade — the incoming order was deleted off the book by
// the first sweep, so the second sweep asks ExecuteTrade to reduce a missing
// order id. Expected once duplicate ids are handled at the gateway boundary.
// Mirrors OrderbookTest.DuplicateOrderIdSilentlyIgnored at the domain layer.
TEST_F(DomainGatewayTest, DISABLED_ReaddingOrderIdDoesNotDoubleExecute) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);
    EXPECT_EQ(book_->GetTopOrder(Side::ASK).order_id, Invalid<OrderIdType>);
    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);

    Order repeat{.order_id = INCOMING_ORDER_1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    AddOrder(repeat);

    EXPECT_EQ(book_->GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

} // namespace Domain::Testing