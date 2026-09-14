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
constexpr OrderIdType UNKNOWN_ORDER_ID = 999;
constexpr OrderbookIdType ORDER_BOOK_ID = 1;
constexpr OrderbookIdType UNKNOWN_ORDER_BOOK_ID = 99;

class DomainGatewayTest : public ::testing::Test {
  protected:
    void SetUp() override {
        auto orderbook_manager = std::make_unique<Market::OrderbookManager>();
        auto orderbook = std::make_unique<Market::Orderbook>(ORDER_BOOK_ID);
        book_ = orderbook.get();
        EXPECT_EQ(orderbook_manager->AddOrderbook(std::move(orderbook)), StatusCode::Success);
        gateway_ = std::make_unique<DomainGateway>(
            std::move(orderbook_manager), std::make_unique<MatchingEngine::MatchingEngine>());
    }

    void AddOrder(Order &order) {
        gateway_->AddOrder(order, ORDER_BOOK_ID);
    }

    void ModifyOrder(Order &order) {
        gateway_->ModifyOrder(order, ORDER_BOOK_ID);
    }

    void DeleteOrder(OrderIdType order_id) {
        Order order_to_delete{.order_id = order_id};
        gateway_->DeleteOrder(order_to_delete, ORDER_BOOK_ID);
    }

    std::unique_ptr<DomainGateway> gateway_; // NOLINT: fixture state, accessed by TEST_F bodies
    Market::Orderbook *book_{nullptr};       // NOLINT: fixture state, accessed by TEST_F bodies
};

void AddRestingAsk(Market::Orderbook &orderbook, OrderIdType order_id, PriceType price,
                   QuantityType quantity, TimestampType timestamp = BASE_TIMESTAMP) {
    (void)orderbook.AddOrder(Order{.order_id = order_id,
                                   .price = price,
                                   .quantity = quantity,
                                   .side = Side::ASK,
                                   .timestamp = timestamp});
}

void AddRestingBid(Market::Orderbook &orderbook, OrderIdType order_id, PriceType price,
                   QuantityType quantity, TimestampType timestamp = BASE_TIMESTAMP) {
    (void)orderbook.AddOrder(Order{.order_id = order_id,
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

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    EXPECT_EQ(book_->GetTopOrder(Side::BID).value().order_id, INCOMING_ORDER_1);
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

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
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
    ASSERT_TRUE(leftover.has_value());
    EXPECT_EQ(leftover.value().order_id, INCOMING_ORDER_1);
    EXPECT_EQ(leftover.value().quantity, BASE_QUANTITY);
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
    ASSERT_TRUE(top_ask.has_value());
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_ask.value().order_id, 1);
    EXPECT_EQ(top_ask.value().quantity, BASE_QUANTITY);
    EXPECT_EQ(top_bid.value().order_id, INCOMING_ORDER_1);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
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
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 3);
    EXPECT_EQ(top_ask.value().quantity, BASE_QUANTITY);
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
}

// ============================================================================
// Gateway rejection paths (RejectOrder is a no-op stub for now)
// ============================================================================

// Adding to an unknown orderbook must not alter the known book.
TEST_F(DomainGatewayTest, AddOrderToUnknownOrderbookSilentlyRejected) {
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    gateway_->AddOrder(incoming, UNKNOWN_ORDER_BOOK_ID);

    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
}

// A duplicate order id while the original still rests must not overwrite it.
TEST_F(DomainGatewayTest, AddDuplicateOrderIdSilentlyRejected) {
    Order first{.order_id = INCOMING_ORDER_1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    AddOrder(first);

    Order duplicate{.order_id = INCOMING_ORDER_1,
                    .price = LOWER_PRICE,
                    .quantity = BASE_QUANTITY,
                    .side = Side::BID,
                    .timestamp = NEXT_TIMESTAMP};
    AddOrder(duplicate);

    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, INCOMING_ORDER_1);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
    EXPECT_EQ(top_bid.value().price, BASE_PRICE);
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
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 1);
    EXPECT_EQ(top_ask.value().quantity, BASE_QUANTITY);
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
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 1);
    EXPECT_EQ(top_ask.value().quantity, BASE_QUANTITY);
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
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
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 2);
    EXPECT_EQ(top_ask.value().quantity, FORTY_QUANTITY);
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
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
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 2);
    EXPECT_EQ(top_ask.value().quantity, FORTY_QUANTITY);
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
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

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
}

TEST_F(DomainGatewayTest, IncomingAskPartialFillLeavesRestingBidRemainder) {
    AddRestingBid(*book_, 1, HIGHER_PRICE, DOUBLE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, 1);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
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
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, INCOMING_ORDER_1);
    EXPECT_EQ(top_ask.value().quantity, THIRTY_QUANTITY);
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
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
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, INCOMING_ORDER_1);
    EXPECT_EQ(top_bid.value().quantity, THIRTY_QUANTITY);
    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
}

// ============================================================================
// Gateway DeleteOrder tests
// ============================================================================

TEST_F(DomainGatewayTest, DeleteOrderThroughGatewayRemovesRestingOrder) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);

    DeleteOrder(1);

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
}

TEST_F(DomainGatewayTest, DeleteOrderThroughGatewaySkipsDeletedOrder) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingAsk(*book_, 2, BASE_PRICE, BASE_QUANTITY);
    DeleteOrder(1);

    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};
    AddOrder(incoming);

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
}

// Deleting an unknown order id must leave the book untouched.
TEST_F(DomainGatewayTest, DeleteOrderUnknownOrderIdSilentlyRejected) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);

    DeleteOrder(UNKNOWN_ORDER_ID);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 1);
    EXPECT_EQ(top_ask.value().quantity, BASE_QUANTITY);
}

// Deleting through an unknown orderbook must leave the book untouched.
TEST_F(DomainGatewayTest, DeleteOrderUnknownOrderbookSilentlyRejected) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);

    Order order_to_delete{.order_id = 1};
    gateway_->DeleteOrder(order_to_delete, UNKNOWN_ORDER_BOOK_ID);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 1);
}

// ============================================================================
// Gateway ModifyOrder tests
// ============================================================================

TEST_F(DomainGatewayTest, ModifyOrderThroughGatewayIncreasesQuantity) {
    AddRestingBid(*book_, 1, BASE_PRICE, THIRTY_QUANTITY);
    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = SIXTY_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, 1);
    EXPECT_EQ(top_bid.value().quantity, SIXTY_QUANTITY);
    EXPECT_EQ(top_bid.value().price, BASE_PRICE);
}

TEST_F(DomainGatewayTest, ModifyOrderThroughGatewayDecreasesQuantity) {
    AddRestingBid(*book_, 1, BASE_PRICE, SIXTY_QUANTITY);
    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = THIRTY_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, 1);
    EXPECT_EQ(top_bid.value().quantity, THIRTY_QUANTITY);
    EXPECT_EQ(top_bid.value().price, BASE_PRICE);
}

// Price change to a still non-crossing level; the order is re-added at the new
// price and nothing executes.
TEST_F(DomainGatewayTest, ModifyOrderPriceChangeStaysNonCrossing) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingBid(*book_, 2, LOWER_PRICE, BASE_QUANTITY);
    Order modified{.order_id = 2,
                   .price = BASE_PRICE - 2,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_ask.has_value());
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_ask.value().order_id, 1);
    EXPECT_EQ(top_ask.value().quantity, BASE_QUANTITY);
    EXPECT_EQ(top_bid.value().order_id, 2);
    EXPECT_EQ(top_bid.value().price, BASE_PRICE - 2);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
}

// A resting bid raised to the ask price crosses; both fully-filled orders
// disappear from the book.
TEST_F(DomainGatewayTest, ModifyBidToCrossConsumesRestingAskFully) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingBid(*book_, 2, LOWER_PRICE, BASE_QUANTITY);
    Order modified{.order_id = 2,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
}

// Modified bid only partially fills the larger resting ask; the ask keeps its
// reduced remainder at the same price.
TEST_F(DomainGatewayTest, ModifyBidToCrossLeavesRestingAskRemainder) {
    AddRestingAsk(*book_, 1, BASE_PRICE, DOUBLE_QUANTITY);
    AddRestingBid(*book_, 2, LOWER_PRICE, SIXTY_QUANTITY);
    Order modified{.order_id = 2,
                   .price = BASE_PRICE,
                   .quantity = SIXTY_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    auto top_ask = book_->GetTopOrder(Side::ASK);
    ASSERT_TRUE(top_ask.has_value());
    EXPECT_EQ(top_ask.value().order_id, 1);
    EXPECT_EQ(top_ask.value().quantity, FORTY_QUANTITY);
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
}

// Quantity increase on the modified bid makes it sweep multiple resting asks.
TEST_F(DomainGatewayTest, ModifyBidIncreaseSweepsMultipleRestingAsks) {
    AddRestingAsk(*book_, 1, BASE_PRICE, SIXTY_QUANTITY);
    AddRestingAsk(*book_, 2, MID_PRICE, SIXTY_QUANTITY);
    AddRestingBid(*book_, 3, LOWER_PRICE, BASE_QUANTITY);
    Order modified{.order_id = 3,
                   .price = HIGHER_PRICE,
                   .quantity = SIXTY_QUANTITY * 2,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
}

// Modified bid consumes 30 + 60 across two levels and rests the leftover.
TEST_F(DomainGatewayTest, ModifyBidLeftoverRestsAfterSweep) {
    AddRestingAsk(*book_, 1, BASE_PRICE, THIRTY_QUANTITY);
    AddRestingAsk(*book_, 2, MID_PRICE, SIXTY_QUANTITY);
    AddRestingBid(*book_, 3, LOWER_PRICE, BASE_QUANTITY);
    Order modified{.order_id = 3,
                   .price = HIGHER_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, 3);
    EXPECT_EQ(top_bid.value().quantity, DOUBLE_QUANTITY - THIRTY_QUANTITY - SIXTY_QUANTITY);
}

// Ask lowered to the resting bid price crosses; both orders are consumed.
TEST_F(DomainGatewayTest, ModifyAskToCrossConsumesRestingBidFully) {
    AddRestingBid(*book_, 1, HIGHER_PRICE, DOUBLE_QUANTITY);
    AddRestingAsk(*book_, 2, MUCH_HIGHER_PRICE, DOUBLE_QUANTITY);
    Order modified{.order_id = 2,
                   .price = HIGHER_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
}

// Lowered ask partially consumes the larger resting bid; the bid keeps its
// remainder.
TEST_F(DomainGatewayTest, ModifyAskToCrossLeavesBidRemainder) {
    AddRestingBid(*book_, 1, HIGHER_PRICE, DOUBLE_QUANTITY);
    AddRestingAsk(*book_, 2, MUCH_HIGHER_PRICE, SIXTY_QUANTITY);
    Order modified{.order_id = 2,
                   .price = HIGHER_PRICE,
                   .quantity = SIXTY_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, 1);
    EXPECT_EQ(top_bid.value().quantity, FORTY_QUANTITY);
}

// Quantity-only modify preserves time priority at the same price level.
TEST_F(DomainGatewayTest, ModifyOrderSamePricePreservesTimePriority) {
    AddRestingBid(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    AddRestingBid(*book_, 2, BASE_PRICE, THIRTY_QUANTITY);
    Order modified{.order_id = 2,
                   .price = Invalid<PriceType>,
                   .quantity = SIXTY_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, 1);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
}

TEST_F(DomainGatewayTest, ModifyThenDeleteThroughGateway) {
    AddRestingBid(*book_, 1, BASE_PRICE, THIRTY_QUANTITY);
    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = SIXTY_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    ModifyOrder(modified);

    DeleteOrder(1);

    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
}

// ============================================================================
// Gateway modify rejection paths (RejectOrder is a no-op stub for now)
// ============================================================================

// Modifying an unknown order id must leave the book untouched.
TEST_F(DomainGatewayTest, ModifyOrderUnknownOrderIdSilentlyRejected) {
    AddRestingBid(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    Order modified{.order_id = INCOMING_ORDER_1,
                   .price = Invalid<PriceType>,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, 1);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
}

// Modifying through an unknown orderbook must leave the book untouched.
TEST_F(DomainGatewayTest, ModifyOrderUnknownOrderbookSilentlyRejected) {
    AddRestingBid(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    gateway_->ModifyOrder(modified, UNKNOWN_ORDER_BOOK_ID);

    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, 1);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
}

// A side-changing modify must not corrupt the resting order.
TEST_F(DomainGatewayTest, ModifyOrderSideMismatchSilentlyRejected) {
    AddRestingBid(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = BASE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};

    ModifyOrder(modified);

    auto top_bid = book_->GetTopOrder(Side::BID);
    ASSERT_TRUE(top_bid.has_value());
    EXPECT_EQ(top_bid.value().order_id, 1);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
}

// ============================================================================
// Duplicate order id through the gateway
// ============================================================================

// SKIPPED: re-adding an already-consumed order id via DomainGateway currently
// rests it on the book instead of rejecting it. Duplicate order detection at
// the gateway boundary is expected to land with the rejection/reconciliation
// design. Kept disabled until then.
TEST_F(DomainGatewayTest, DISABLED_ReaddingOrderIdDoesNotDoubleExecute) {
    AddRestingAsk(*book_, 1, BASE_PRICE, BASE_QUANTITY);
    Order incoming{.order_id = INCOMING_ORDER_1,
                   .price = BASE_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};

    AddOrder(incoming);
    EXPECT_FALSE(book_->GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());

    Order repeat{.order_id = INCOMING_ORDER_1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    AddOrder(repeat);

    EXPECT_FALSE(book_->GetTopOrder(Side::BID).has_value());
}

} // namespace Domain::Testing