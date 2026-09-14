#include "domain/market.hpp"
#include <gtest/gtest.h>
#include <memory>
// domain_types.hpp is already included by market.hpp

namespace Domain::Market::Testing {

// Test constants to satisfy clang-tidy rules
constexpr PriceType BASE_PRICE = 100;
constexpr PriceType LOWER_PRICE = 99;
constexpr PriceType HIGHER_PRICE = 105;
constexpr PriceType MUCH_HIGHER_PRICE = 106;
constexpr QuantityType BASE_QUANTITY = 50;
constexpr QuantityType LARGER_QUANTITY = 75;
constexpr QuantityType DOUBLE_QUANTITY = 100;
constexpr QuantityType TRIPLE_QUANTITY = 200;
constexpr QuantityType FORTY_QUANTITY = 40;
constexpr QuantityType SIXTY_QUANTITY = 60;
constexpr TimestampType BASE_TIMESTAMP = 1000;
constexpr TimestampType NEXT_TIMESTAMP = 1001;
constexpr TimestampType LATER_TIMESTAMP = 1002;
constexpr TimestampType LARGE_QUANTITY_CONST = 1000000000000ULL;
constexpr PriceType MAX_INT64_PRICE = 9223372036854775807LL;
constexpr OrderIdType NONEXISTENT_ORDER_ID = 100;
constexpr OrderbookIdType ORDERBOOK_ID = 1;
constexpr OrderbookIdType SECOND_ORDERBOOK_ID = 2;
constexpr OrderbookIdType UNKNOWN_ORDERBOOK_ID = 99;

// ============================================================================
// PriceLevel Tests
// ============================================================================

class PriceLevelTest : public ::testing::Test {
  protected:
    PriceLevel price_level_{BASE_PRICE}; // Create a price level at price 100
};

TEST_F(PriceLevelTest, AddSingleOrder) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};

    auto iterator = price_level_.AddOrder(order);

    // Verify that the iterator is valid and points to the added order
    EXPECT_EQ(iterator->order_id, 1);
    EXPECT_EQ(iterator->quantity, BASE_QUANTITY);
    EXPECT_EQ(iterator->price, BASE_PRICE);
}

TEST_F(PriceLevelTest, AddMultipleOrders) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};

    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};

    auto iter1 = price_level_.AddOrder(order1);
    auto iter2 = price_level_.AddOrder(order2);

    // Verify both orders are added correctly
    EXPECT_EQ(iter1->order_id, 1);
    EXPECT_EQ(iter2->order_id, 2);
    EXPECT_EQ(iter1->quantity, BASE_QUANTITY);
    EXPECT_EQ(iter2->quantity, LARGER_QUANTITY);
}

TEST_F(PriceLevelTest, AddOrderAccumulatesQuantity) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = DOUBLE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};

    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = TRIPLE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};

    auto iter1 = price_level_.AddOrder(order1);
    auto iter2 = price_level_.AddOrder(order2);

    EXPECT_NE(iter1, iter2);

    // Both orders must be tracked in the level total: this update only
    // succeeds if the accumulated quantity reaches TRIPLE_QUANTITY.
    EXPECT_EQ(price_level_.UpdateQuantity({.modified_order_new_quantity = DOUBLE_QUANTITY,
                                           .modified_order_old_quantity = TRIPLE_QUANTITY}),
              StatusCode::Success);
}

TEST_F(PriceLevelTest, OrdersMaintainFIFOOrder) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};

    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};

    auto iter1 = price_level_.AddOrder(order1);
    auto iter2 = price_level_.AddOrder(order2);

    // iter1 should come before iter2 in the list
    EXPECT_EQ(iter1->order_id, 1);
    EXPECT_EQ(iter2->order_id, 2);
}

// ============================================================================
// Orderbook Tests
// ============================================================================

class OrderbookTest : public ::testing::Test {
  protected:
    Orderbook orderbook_{ORDERBOOK_ID};
};

TEST_F(OrderbookTest, AddBidOrder) {
    Order bid_order{.order_id = 1,
                    .price = BASE_PRICE,
                    .quantity = BASE_QUANTITY,
                    .side = Side::BID,
                    .timestamp = BASE_TIMESTAMP};

    EXPECT_EQ(orderbook_.AddOrder(bid_order), StatusCode::Success);
}

TEST_F(OrderbookTest, AddAskOrder) {
    Order ask_order{.order_id = 1,
                    .price = HIGHER_PRICE,
                    .quantity = DOUBLE_QUANTITY,
                    .side = Side::ASK,
                    .timestamp = BASE_TIMESTAMP};

    EXPECT_EQ(orderbook_.AddOrder(ask_order), StatusCode::Success);
}

TEST_F(OrderbookTest, AddMultipleBidOrders) {
    Order bid1{.order_id = 1,
               .price = BASE_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::BID,
               .timestamp = BASE_TIMESTAMP};

    Order bid2{.order_id = 2,
               .price = LOWER_PRICE,
               .quantity = LARGER_QUANTITY,
               .side = Side::BID,
               .timestamp = NEXT_TIMESTAMP};

    EXPECT_EQ(orderbook_.AddOrder(bid1), StatusCode::Success);
    EXPECT_EQ(orderbook_.AddOrder(bid2), StatusCode::Success);
}

TEST_F(OrderbookTest, AddMultipleAskOrders) {
    Order ask1{.order_id = 1,
               .price = HIGHER_PRICE,
               .quantity = DOUBLE_QUANTITY,
               .side = Side::ASK,
               .timestamp = BASE_TIMESTAMP};

    Order ask2{.order_id = 2,
               .price = MUCH_HIGHER_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::ASK,
               .timestamp = NEXT_TIMESTAMP};

    EXPECT_EQ(orderbook_.AddOrder(ask1), StatusCode::Success);
    EXPECT_EQ(orderbook_.AddOrder(ask2), StatusCode::Success);
}

TEST_F(OrderbookTest, AddMixedBidAndAskOrders) {
    Order bid1{.order_id = 1,
               .price = BASE_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::BID,
               .timestamp = BASE_TIMESTAMP};

    Order ask1{.order_id = 2,
               .price = HIGHER_PRICE,
               .quantity = LARGER_QUANTITY,
               .side = Side::ASK,
               .timestamp = NEXT_TIMESTAMP};

    Order bid2{.order_id = 3,
               .price = LOWER_PRICE,
               .quantity = DOUBLE_QUANTITY,
               .side = Side::BID,
               .timestamp = LATER_TIMESTAMP};

    EXPECT_EQ(orderbook_.AddOrder(bid1), StatusCode::Success);
    EXPECT_EQ(orderbook_.AddOrder(ask1), StatusCode::Success);
    EXPECT_EQ(orderbook_.AddOrder(bid2), StatusCode::Success);
}

TEST_F(OrderbookTest, MultipleOrdersAtSamePrice) {
    Order bid1{.order_id = 1,
               .price = BASE_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::BID,
               .timestamp = BASE_TIMESTAMP};

    Order bid2{.order_id = 2,
               .price = BASE_PRICE,
               .quantity = LARGER_QUANTITY,
               .side = Side::BID,
               .timestamp = NEXT_TIMESTAMP};

    EXPECT_EQ(orderbook_.AddOrder(bid1), StatusCode::Success);
    EXPECT_EQ(orderbook_.AddOrder(bid2), StatusCode::Success);
}

TEST_F(OrderbookTest, RejectOrderWithZeroQuantity) {
    Order invalid_order{.order_id = 1,
                        .price = BASE_PRICE,
                        .quantity = 0, // Invalid: zero quantity
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // The orderbook must reject a zero-quantity order with a status code.
    EXPECT_EQ(orderbook_.AddOrder(invalid_order), StatusCode::InvalidQty);
}

TEST_F(OrderbookTest, RejectOrderWithNegativeQuantity) {
    Order invalid_order{.order_id = 1,
                        .price = BASE_PRICE,
                        .quantity =
                            static_cast<QuantityType>(-1), // NOLINT: intentional invalid value
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // static_cast<QuantityType>(-1) is UINT64_MAX, the same value as
    // Invalid<QuantityType>; it must be rejected like any other sentinel.
    EXPECT_EQ(orderbook_.AddOrder(invalid_order), StatusCode::InvalidQty);
}

TEST_F(OrderbookTest, RejectOrderWithInvalidSide) {
    Order invalid_order{.order_id = 1,
                        .price = BASE_PRICE,
                        .quantity = BASE_QUANTITY,
                        .side = Side::INVALID, // Invalid side
                        .timestamp = BASE_TIMESTAMP};

    // The orderbook must reject an invalid side with a status code.
    EXPECT_EQ(orderbook_.AddOrder(invalid_order), StatusCode::InvalidSide);
}

TEST_F(OrderbookTest, RejectOrderWithZeroOrderId) {
    Order invalid_order{.order_id = 0, // Invalid: zero order ID
                        .price = BASE_PRICE,
                        .quantity = BASE_QUANTITY,
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // The orderbook must reject a zero order id with a status code.
    EXPECT_EQ(orderbook_.AddOrder(invalid_order), StatusCode::InvalidOrderId);
}

TEST_F(OrderbookTest, RejectOrderWithZeroPrice) {
    Order invalid_order{.order_id = 1,
                        .price = 0, // Invalid: zero price
                        .quantity = BASE_QUANTITY,
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // The orderbook must reject a zero price with a status code.
    EXPECT_EQ(orderbook_.AddOrder(invalid_order), StatusCode::InvalidPrice);
}

TEST_F(OrderbookTest, RejectOrderWithInvalidOrderId) {
    Order invalid_order{.order_id = Invalid<OrderIdType>,
                        .price = BASE_PRICE,
                        .quantity = BASE_QUANTITY,
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // The Invalid sentinel is not a legal order id and must be rejected.
    EXPECT_EQ(orderbook_.AddOrder(invalid_order), StatusCode::InvalidOrderId);
}

TEST_F(OrderbookTest, RejectOrderWithInvalidPrice) {
    Order invalid_order{.order_id = 1,
                        .price = Invalid<PriceType>,
                        .quantity = BASE_QUANTITY,
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // The Invalid sentinel is not a legal price and must be rejected.
    EXPECT_EQ(orderbook_.AddOrder(invalid_order), StatusCode::InvalidPrice);
}

TEST_F(OrderbookTest, LargeQuantityOrder) {
    Order large_order{.order_id = 1,
                      .price = BASE_PRICE,
                      .quantity = LARGE_QUANTITY_CONST, // Very large quantity
                      .side = Side::BID,
                      .timestamp = BASE_TIMESTAMP};

    EXPECT_EQ(orderbook_.AddOrder(large_order), StatusCode::Success);
}

TEST_F(OrderbookTest, LargePriceOrder) {
    Order large_price_order{.order_id = 1,
                            .price = MAX_INT64_PRICE, // Max int64_t
                            .quantity = BASE_QUANTITY,
                            .side = Side::BID,
                            .timestamp = BASE_TIMESTAMP};

    EXPECT_EQ(orderbook_.AddOrder(large_price_order), StatusCode::Success);
}

// ============================================================================
// PriceLevel DeleteOrder & IsEmpty Tests
// ============================================================================

TEST_F(PriceLevelTest, IsEmptyInitially) {
    EXPECT_TRUE(price_level_.IsEmpty());
}

TEST_F(PriceLevelTest, IsEmptyAfterAdd) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)price_level_.AddOrder(order);
    EXPECT_FALSE(price_level_.IsEmpty());
}

TEST_F(PriceLevelTest, IsEmptyAfterAddDelete) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    auto iter = price_level_.AddOrder(order);
    price_level_.DeleteOrder(iter);
    EXPECT_TRUE(price_level_.IsEmpty());
}

TEST_F(PriceLevelTest, DeleteSingleOrder) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    auto iter = price_level_.AddOrder(order);
    EXPECT_FALSE(price_level_.IsEmpty());
    price_level_.DeleteOrder(iter);
    EXPECT_TRUE(price_level_.IsEmpty());
}

TEST_F(PriceLevelTest, DeleteFromMultipleOrders) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    Order order3{.order_id = 3,
                 .price = BASE_PRICE,
                 .quantity = DOUBLE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = LATER_TIMESTAMP};

    auto iter1 = price_level_.AddOrder(order1);
    auto iter2 = price_level_.AddOrder(order2);
    auto iter3 = price_level_.AddOrder(order3);

    // Delete the middle order
    price_level_.DeleteOrder(iter2);

    // Remaining iterators should still be valid
    EXPECT_EQ(iter1->order_id, 1);
    EXPECT_EQ(iter3->order_id, 3);
    EXPECT_FALSE(price_level_.IsEmpty());

    // Add a 4th order to verify list is functional after deletion
    Order order4{.order_id = 4,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    auto iter4 = price_level_.AddOrder(order4);
    EXPECT_EQ(iter4->order_id, 4);

    // Delete remaining orders one by one
    price_level_.DeleteOrder(iter1);
    EXPECT_FALSE(price_level_.IsEmpty());
    price_level_.DeleteOrder(iter3);
    EXPECT_FALSE(price_level_.IsEmpty());
    price_level_.DeleteOrder(iter4);
    EXPECT_TRUE(price_level_.IsEmpty());
}

TEST_F(PriceLevelTest, DeleteAllOrders) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};

    auto iter1 = price_level_.AddOrder(order1);
    auto iter2 = price_level_.AddOrder(order2);

    EXPECT_FALSE(price_level_.IsEmpty());
    price_level_.DeleteOrder(iter1);
    EXPECT_FALSE(price_level_.IsEmpty());
    price_level_.DeleteOrder(iter2);
    EXPECT_TRUE(price_level_.IsEmpty());
}

// ============================================================================
// Orderbook DeleteOrder Tests
// ============================================================================

TEST_F(OrderbookTest, DeleteBidOrder) {
    Order bid{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::BID,
              .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(bid);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, DeleteAskOrder) {
    Order ask{.order_id = 1,
              .price = HIGHER_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::ASK,
              .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(ask);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, DeleteOrderAtSamePriceLevel) {
    Order bid1{.order_id = 1,
               .price = BASE_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::BID,
               .timestamp = BASE_TIMESTAMP};
    Order bid2{.order_id = 2,
               .price = BASE_PRICE,
               .quantity = LARGER_QUANTITY,
               .side = Side::BID,
               .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(bid1);
    (void)orderbook_.AddOrder(bid2);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
}

TEST_F(OrderbookTest, DeleteOrderFromMultiLevel) {
    Order bid1{.order_id = 1,
               .price = BASE_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::BID,
               .timestamp = BASE_TIMESTAMP};
    Order bid2{.order_id = 2,
               .price = LOWER_PRICE,
               .quantity = LARGER_QUANTITY,
               .side = Side::BID,
               .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(bid1);
    (void)orderbook_.AddOrder(bid2);

    // Delete the order at the higher price level
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);

    // Should still be able to interact with the remaining price level
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
}

TEST_F(OrderbookTest, DeleteCrossSideOrders) {
    Order bid{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::BID,
              .timestamp = BASE_TIMESTAMP};
    Order ask{.order_id = 2,
              .price = HIGHER_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::ASK,
              .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(bid);
    (void)orderbook_.AddOrder(ask);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
}

TEST_F(OrderbookTest, DeleteNonexistentOrder) {
    EXPECT_EQ(orderbook_.DeleteOrder(999), StatusCode::OrderNotFound);
}

TEST_F(OrderbookTest, DeleteThenReAddSameOrderId) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);

    // Re-adding the same order_id should succeed
    Order re_add{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.AddOrder(re_add), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, DuplicateOrderIdRejected) {
    Order first{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    Order second{.order_id = 1,
                 .price = LOWER_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(first);

    // A second live order with the same order_id must be rejected before it
    // can corrupt the price level with an untracked orphan order.
    EXPECT_EQ(orderbook_.AddOrder(second), StatusCode::DuplicateOrderId);
}

TEST_F(OrderbookTest, DuplicateOrderIdAcrossSidesRejected) {
    Order bid{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::BID,
              .timestamp = BASE_TIMESTAMP};
    Order ask{.order_id = 1,
              .price = HIGHER_PRICE,
              .quantity = LARGER_QUANTITY,
              .side = Side::ASK,
              .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(bid);

    // Order ids are unique across both sides of the book.
    EXPECT_EQ(orderbook_.AddOrder(ask), StatusCode::DuplicateOrderId);
}

TEST_F(OrderbookTest, DeleteOrderAfterAddingToNewPriceLevel) {
    Order bid1{.order_id = 1,
               .price = BASE_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::BID,
               .timestamp = BASE_TIMESTAMP};
    Order bid2{.order_id = 2,
               .price = BASE_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::BID,
               .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(bid1);
    (void)orderbook_.AddOrder(bid2);

    // Delete the first order — price level still has bid2, so it should persist
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);

    // Add another order at a fresh price level, then delete the remaining order
    Order bid3{.order_id = 3,
               .price = LOWER_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::BID,
               .timestamp = LATER_TIMESTAMP};
    (void)orderbook_.AddOrder(bid3);
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(3), StatusCode::Success);
}

// ============================================================================
// PriceLevel UpdateQuantity Tests
// ============================================================================

TEST_F(PriceLevelTest, UpdateQuantityIncrease) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    (void)price_level_.AddOrder(order1);
    (void)price_level_.AddOrder(order2);

    EXPECT_EQ(price_level_.UpdateQuantity({.modified_order_new_quantity = DOUBLE_QUANTITY,
                                           .modified_order_old_quantity = BASE_QUANTITY}),
              StatusCode::Success);
}

TEST_F(PriceLevelTest, UpdateQuantityDecrease) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = TRIPLE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    (void)price_level_.AddOrder(order1);
    (void)price_level_.AddOrder(order2);

    EXPECT_EQ(price_level_.UpdateQuantity({.modified_order_new_quantity = BASE_QUANTITY,
                                           .modified_order_old_quantity = TRIPLE_QUANTITY}),
              StatusCode::Success);
}

TEST_F(PriceLevelTest, UpdateQuantityNoChange) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    (void)price_level_.AddOrder(order1);

    EXPECT_EQ(price_level_.UpdateQuantity({.modified_order_new_quantity = BASE_QUANTITY,
                                           .modified_order_old_quantity = BASE_QUANTITY}),
              StatusCode::Success);
}

TEST_F(PriceLevelTest, UpdateQuantityRejectsWhenOldQuantityExceedsLevelTotal) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    (void)price_level_.AddOrder(order1);

    // old_quantity larger than the whole level would drive the total negative;
    // the update must be rejected and leave the level untouched.
    EXPECT_EQ(price_level_.UpdateQuantity({.modified_order_new_quantity = BASE_QUANTITY,
                                           .modified_order_old_quantity = LARGER_QUANTITY}),
              StatusCode::InvalidQty);
}

// ============================================================================
// Orderbook ModifyOrder Tests — Quantity Only (In-Place, Same Price)
// ============================================================================

TEST_F(OrderbookTest, ModifyBidQuantityIncrease) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyBidQuantityDecrease) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = DOUBLE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyAskQuantityIncrease) {
    Order order{.order_id = 1,
                .price = HIGHER_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::ASK,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyAskQuantityDecrease) {
    Order order{.order_id = 1,
                .price = HIGHER_PRICE,
                .quantity = DOUBLE_QUANTITY,
                .side = Side::ASK,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = BASE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyOrderSamePriceExplicitly) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = BASE_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyOrderSameQuantityNoChange) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyOrderMultipleTimes) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order mod1{.order_id = 1,
               .price = Invalid<PriceType>,
               .quantity = DOUBLE_QUANTITY,
               .side = Side::BID,
               .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(mod1), StatusCode::Success);

    Order mod2{.order_id = 1,
               .price = Invalid<PriceType>,
               .quantity = TRIPLE_QUANTITY,
               .side = Side::BID,
               .timestamp = LATER_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(mod2), StatusCode::Success);

    // Modify with explicit same price after sentinel modifies
    Order mod3{.order_id = 1,
               .price = BASE_PRICE,
               .quantity = DOUBLE_QUANTITY,
               .side = Side::BID,
               .timestamp = BASE_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(mod3), StatusCode::Success);

    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyOrderThenDeleteSameOrder) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

// ============================================================================
// Orderbook ModifyOrder Tests — Price Change (Delete + Add)
// ============================================================================

TEST_F(OrderbookTest, ModifyBidPriceChangeToHigher) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyBidPriceChangeToLower) {
    Order order{.order_id = 1,
                .price = HIGHER_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = BASE_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyAskPriceChangeToHigher) {
    Order order{.order_id = 1,
                .price = HIGHER_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::ASK,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = MUCH_HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyAskPriceChangeToLower) {
    Order order{.order_id = 1,
                .price = MUCH_HIGHER_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::ASK,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

// ============================================================================
// Orderbook ModifyOrder Tests — Multi-Level & Multi-Order Interop
// ============================================================================

TEST_F(OrderbookTest, ModifyOrderPreservesOtherOrdersAtSamePriceLevel) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(order1);
    (void)orderbook_.AddOrder(order2);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);

    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyOrderToNewPriceLevel) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(order1);
    (void)orderbook_.AddOrder(order2);

    Order modified{.order_id = 2,
                   .price = HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);

    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyOrderToExistingPriceLevel) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = HIGHER_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(order1);
    (void)orderbook_.AddOrder(order2);

    Order modified{.order_id = 2,
                   .price = BASE_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);

    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyOrderFromPriceLevelWithMultipleOrders) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    Order order3{.order_id = 3,
                 .price = BASE_PRICE,
                 .quantity = DOUBLE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = LATER_TIMESTAMP};
    (void)orderbook_.AddOrder(order1);
    (void)orderbook_.AddOrder(order2);
    (void)orderbook_.AddOrder(order3);

    // Move order2 away — BASE_PRICE level still has order1 and order3
    Order modified{.order_id = 2,
                   .price = HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);

    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(3), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyOrderToPriceLevelThatEmptiesSourceLevel) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = HIGHER_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(order1);
    (void)orderbook_.AddOrder(order2);

    // Move the only order at HIGHER_PRICE to BASE_PRICE — HIGHER_PRICE level empties
    Order modified{.order_id = 2,
                   .price = BASE_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);

    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyCrossSideOrdersWithPriceChange) {
    Order bid{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::BID,
              .timestamp = BASE_TIMESTAMP};
    Order ask{.order_id = 2,
              .price = HIGHER_PRICE,
              .quantity = LARGER_QUANTITY,
              .side = Side::ASK,
              .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(bid);
    (void)orderbook_.AddOrder(ask);

    Order modified{.order_id = 2,
                   .price = MUCH_HIGHER_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);

    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
}

TEST_F(OrderbookTest, ModifyBidCrossesAskSide) {
    Order bid{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::BID,
              .timestamp = BASE_TIMESTAMP};
    Order ask{.order_id = 2,
              .price = HIGHER_PRICE,
              .quantity = LARGER_QUANTITY,
              .side = Side::ASK,
              .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(bid);
    (void)orderbook_.AddOrder(ask);

    // Move bid above ask — the orderbook allows crosses; matching engine handles them later
    Order modified{.order_id = 1,
                   .price = MUCH_HIGHER_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::Success);

    EXPECT_EQ(orderbook_.DeleteOrder(1), StatusCode::Success);
    EXPECT_EQ(orderbook_.DeleteOrder(2), StatusCode::Success);
}

// SKIPPED: pins time priority across a price change. ModifyOrder to a new price
// goes through delete+add (market.cpp), which appends the re-added order at the
// TAIL of the destination price level. An earlier-timestamp order therefore
// ends up behind a later order at that level. Disabled until timestamps are
// supported.
TEST_F(OrderbookTest, DISABLED_ModifyOrderPriceChangeKeepsEarliestTimestampPriority) {
    Order order1{.order_id = 1,
                 .price = HIGHER_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    Order order2{.order_id = 2,
                 .price = MUCH_HIGHER_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(order1);
    (void)orderbook_.AddOrder(order2);

    Order modified{.order_id = 1,
                   .price = MUCH_HIGHER_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.ModifyOrder(modified);

    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).value().order_id, 1);
}

// ============================================================================
// Orderbook ModifyOrder Tests — Edge Cases & Death Tests
// ============================================================================

TEST_F(OrderbookTest, ModifyNonexistentOrder) {
    Order modified{.order_id = NONEXISTENT_ORDER_ID,
                   .price = Invalid<PriceType>,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::OrderNotFound);
}

TEST_F(OrderbookTest, ModifyOrderToZeroQuantityAsserts) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = 0,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    // A zero-quantity order would linger forever at the head of its level.
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::InvalidQty);
}

TEST_F(OrderbookTest, ModifyOrderToInvalidQuantityAsserts) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = Invalid<QuantityType>,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};

    // The Invalid sentinel is not a legal quantity, in the modify path too.
    EXPECT_EQ(orderbook_.ModifyOrder(modified), StatusCode::InvalidQty);
}

// ============================================================================
// PriceLevel GetTopOrder Tests
// ============================================================================

// PriceLevel::GetTopOrder is documented as only callable on a non-empty
// level (it triggers std::unreachable() otherwise), so there is no "empty
// level returns default order" contract to pin anymore.

TEST_F(PriceLevelTest, GetTopOrderReturnsFirstInsertedOrder) {
    Order first{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    Order second{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    (void)price_level_.AddOrder(first);
    (void)price_level_.AddOrder(second);

    auto top = price_level_.GetTopOrder();
    EXPECT_EQ(top.order_id, 1);
    EXPECT_EQ(top.price, BASE_PRICE);
    EXPECT_EQ(top.quantity, BASE_QUANTITY);
}

TEST_F(PriceLevelTest, GetTopOrderIsNonMutating) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    (void)price_level_.AddOrder(order);

    auto top_first = price_level_.GetTopOrder();
    auto top_second = price_level_.GetTopOrder();
    EXPECT_FALSE(price_level_.IsEmpty());
    EXPECT_EQ(top_first.order_id, 1);
    EXPECT_EQ(top_second.order_id, 1);
}

TEST_F(PriceLevelTest, GetTopOrderFollowsQueueAfterDelete) {
    Order first{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    Order second{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    auto first_iter = price_level_.AddOrder(first);
    (void)price_level_.AddOrder(second);

    price_level_.DeleteOrder(first_iter);
    auto top = price_level_.GetTopOrder();
    EXPECT_EQ(top.order_id, 2);
}

TEST_F(PriceLevelTest, GetTopOrderReturnsHeadByArrivalNotTimestamp) {
    // First inserted order carries a later timestamp than the second one;
    // the head is decided by arrival order, not the timestamp field.
    Order first{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = LATER_TIMESTAMP};
    Order second{.order_id = 2,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    (void)price_level_.AddOrder(first);
    (void)price_level_.AddOrder(second);

    auto top = price_level_.GetTopOrder();
    EXPECT_EQ(top.order_id, 1);
}

// ============================================================================
// Orderbook GetTopOrder Tests
// ============================================================================

TEST_F(OrderbookTest, GetTopOrderOnEmptyBookReturnsNoValueForBothSides) {
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::BID).has_value());
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::ASK).has_value());
}

TEST_F(OrderbookTest, BestBidIsHighestPrice) {
    Order lower_bid{.order_id = 1,
                    .price = LOWER_PRICE,
                    .quantity = BASE_QUANTITY,
                    .side = Side::BID,
                    .timestamp = BASE_TIMESTAMP};
    Order higher_bid{.order_id = 2,
                     .price = BASE_PRICE,
                     .quantity = LARGER_QUANTITY,
                     .side = Side::BID,
                     .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(lower_bid);
    (void)orderbook_.AddOrder(higher_bid);

    auto top = orderbook_.GetTopOrder(Side::BID);
    EXPECT_EQ(top.value().order_id, 2);
    EXPECT_EQ(top.value().price, BASE_PRICE);
}

TEST_F(OrderbookTest, BestAskIsLowestPrice) {
    Order higher_ask{.order_id = 1,
                     .price = MUCH_HIGHER_PRICE,
                     .quantity = BASE_QUANTITY,
                     .side = Side::ASK,
                     .timestamp = BASE_TIMESTAMP};
    Order lower_ask{.order_id = 2,
                    .price = HIGHER_PRICE,
                    .quantity = LARGER_QUANTITY,
                    .side = Side::ASK,
                    .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(higher_ask);
    (void)orderbook_.AddOrder(lower_ask);

    auto top = orderbook_.GetTopOrder(Side::ASK);
    EXPECT_EQ(top.value().order_id, 2);
    EXPECT_EQ(top.value().price, HIGHER_PRICE);
}

TEST_F(OrderbookTest, GetTopOrderIsSideIsolated) {
    // Crossing ask sits below the best bid, but must never surface as a bid.
    Order bid{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::BID,
              .timestamp = BASE_TIMESTAMP};
    Order crossing_ask{.order_id = 2,
                       .price = LOWER_PRICE,
                       .quantity = LARGER_QUANTITY,
                       .side = Side::ASK,
                       .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(bid);
    (void)orderbook_.AddOrder(crossing_ask);

    auto top_bid = orderbook_.GetTopOrder(Side::BID);
    auto top_ask = orderbook_.GetTopOrder(Side::ASK);
    EXPECT_EQ(top_bid.value().order_id, 1);
    EXPECT_EQ(top_ask.value().order_id, 2);
}

TEST_F(OrderbookTest, SamePriceLevelReturnsSeniorOrder) {
    Order ask_first{.order_id = 1,
                    .price = HIGHER_PRICE,
                    .quantity = BASE_QUANTITY,
                    .side = Side::ASK,
                    .timestamp = BASE_TIMESTAMP};
    Order ask_second{.order_id = 2,
                     .price = HIGHER_PRICE,
                     .quantity = LARGER_QUANTITY,
                     .side = Side::ASK,
                     .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(ask_first);
    (void)orderbook_.AddOrder(ask_second);

    auto top = orderbook_.GetTopOrder(Side::ASK);
    EXPECT_EQ(top.value().order_id, 1);
}

TEST_F(OrderbookTest, GetTopOrderTracksModifyAndDelete) {
    Order bid1{.order_id = 1,
               .price = BASE_PRICE,
               .quantity = BASE_QUANTITY,
               .side = Side::BID,
               .timestamp = BASE_TIMESTAMP};
    Order bid2{.order_id = 2,
               .price = LOWER_PRICE,
               .quantity = LARGER_QUANTITY,
               .side = Side::BID,
               .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(bid1);
    (void)orderbook_.AddOrder(bid2);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).value().order_id, 1);

    // Move bid1 down to bid2's level: bid2 is senior there and becomes best bid.
    Order modified{.order_id = 1,
                   .price = LOWER_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    (void)orderbook_.ModifyOrder(modified);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).value().order_id, 2);

    (void)orderbook_.DeleteOrder(2);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).value().order_id, 1);

    (void)orderbook_.DeleteOrder(1);
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::BID).has_value());
}

// ============================================================================
// Orderbook ExecuteTrade Tests
// ============================================================================

TEST_F(OrderbookTest, ExecuteTradePartialFillReducesBothOrdersInPlace) {
    Order ask{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = DOUBLE_QUANTITY,
              .side = Side::ASK,
              .timestamp = BASE_TIMESTAMP};
    Order bid{.order_id = 2,
              .price = BASE_PRICE,
              .quantity = DOUBLE_QUANTITY,
              .side = Side::BID,
              .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(ask);
    (void)orderbook_.AddOrder(bid);

    EXPECT_EQ(orderbook_.ExecuteTrade({.ask_id = 1, .bid_id = 2}, FORTY_QUANTITY),
              StatusCode::Success);

    auto top_ask = orderbook_.GetTopOrder(Side::ASK);
    auto top_bid = orderbook_.GetTopOrder(Side::BID);
    EXPECT_EQ(top_ask.value().order_id, 1);
    EXPECT_EQ(top_ask.value().quantity, SIXTY_QUANTITY);
    EXPECT_EQ(top_bid.value().order_id, 2);
    EXPECT_EQ(top_bid.value().quantity, SIXTY_QUANTITY);
}

TEST_F(OrderbookTest, ExecuteTradeFullFillDeletesBothOrders) {
    Order ask{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::ASK,
              .timestamp = BASE_TIMESTAMP};
    Order bid{.order_id = 2,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::BID,
              .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(ask);
    (void)orderbook_.AddOrder(bid);

    EXPECT_EQ(orderbook_.ExecuteTrade({.ask_id = 1, .bid_id = 2}, BASE_QUANTITY),
              StatusCode::Success);

    EXPECT_FALSE(orderbook_.GetTopOrder(Side::ASK).has_value());
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::BID).has_value());
}

TEST_F(OrderbookTest, ExecuteTradeFullyConsumesAskLeavesPartialBid) {
    Order ask{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::ASK,
              .timestamp = BASE_TIMESTAMP};
    Order bid{.order_id = 2,
              .price = BASE_PRICE,
              .quantity = DOUBLE_QUANTITY,
              .side = Side::BID,
              .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(ask);
    (void)orderbook_.AddOrder(bid);

    EXPECT_EQ(orderbook_.ExecuteTrade({.ask_id = 1, .bid_id = 2}, BASE_QUANTITY),
              StatusCode::Success);

    EXPECT_FALSE(orderbook_.GetTopOrder(Side::ASK).has_value());
    auto top_bid = orderbook_.GetTopOrder(Side::BID);
    EXPECT_EQ(top_bid.value().order_id, 2);
    EXPECT_EQ(top_bid.value().quantity, BASE_QUANTITY);
}

TEST_F(OrderbookTest, ExecuteTradeFullyConsumesBidLeavesPartialAsk) {
    Order ask{.order_id = 1,
              .price = BASE_PRICE,
              .quantity = DOUBLE_QUANTITY,
              .side = Side::ASK,
              .timestamp = BASE_TIMESTAMP};
    Order bid{.order_id = 2,
              .price = BASE_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::BID,
              .timestamp = NEXT_TIMESTAMP};
    (void)orderbook_.AddOrder(ask);
    (void)orderbook_.AddOrder(bid);

    EXPECT_EQ(orderbook_.ExecuteTrade({.ask_id = 1, .bid_id = 2}, BASE_QUANTITY),
              StatusCode::Success);

    auto top_ask = orderbook_.GetTopOrder(Side::ASK);
    EXPECT_EQ(top_ask.value().order_id, 1);
    EXPECT_EQ(top_ask.value().quantity, BASE_QUANTITY);
    EXPECT_FALSE(orderbook_.GetTopOrder(Side::BID).has_value());
}

TEST_F(OrderbookTest, ExecuteTradePartialFillPreservesTimePriorityAtLevel) {
    Order ask_first{.order_id = 1,
                    .price = BASE_PRICE,
                    .quantity = DOUBLE_QUANTITY,
                    .side = Side::ASK,
                    .timestamp = BASE_TIMESTAMP};
    Order ask_second{.order_id = 2,
                     .price = BASE_PRICE,
                     .quantity = BASE_QUANTITY,
                     .side = Side::ASK,
                     .timestamp = NEXT_TIMESTAMP};
    Order bid{.order_id = 3,
              .price = BASE_PRICE,
              .quantity = DOUBLE_QUANTITY,
              .side = Side::BID,
              .timestamp = BASE_TIMESTAMP};
    (void)orderbook_.AddOrder(ask_first);
    (void)orderbook_.AddOrder(ask_second);
    (void)orderbook_.AddOrder(bid);

    EXPECT_EQ(orderbook_.ExecuteTrade({.ask_id = 1, .bid_id = 3}, FORTY_QUANTITY),
              StatusCode::Success);

    auto top_ask = orderbook_.GetTopOrder(Side::ASK);
    EXPECT_EQ(top_ask.value().order_id, 1);
    EXPECT_EQ(top_ask.value().quantity, SIXTY_QUANTITY);
}

// ExecuteTrade's failures on unknown ids / exec quantity above resting are
// intentional-unreachable paths in market.cpp (std::unreachable()), which compiles
// to UB rather than a reliably observable abort. They are not pinned here until the
// domain contract returns a defined StatusCode on those paths.

// ============================================================================
// OrderbookManager Tests
// ============================================================================

class OrderbookManagerTest : public ::testing::Test {
  protected:
    OrderbookManager orderbook_manager_;
};

TEST_F(OrderbookManagerTest, AddAndGetSingleOrderbook) {
    EXPECT_EQ(orderbook_manager_.AddOrderbook(std::make_unique<Orderbook>(ORDERBOOK_ID)),
              StatusCode::Success);

    EXPECT_EQ(orderbook_manager_.GetOrderbook(ORDERBOOK_ID).value()->GetOrderbookId(),
              ORDERBOOK_ID);
}

TEST_F(OrderbookManagerTest, AddAndGetMultipleOrderbooks) {
    EXPECT_EQ(orderbook_manager_.AddOrderbook(std::make_unique<Orderbook>(ORDERBOOK_ID)),
              StatusCode::Success);
    EXPECT_EQ(orderbook_manager_.AddOrderbook(std::make_unique<Orderbook>(SECOND_ORDERBOOK_ID)),
              StatusCode::Success);

    EXPECT_EQ(orderbook_manager_.GetOrderbook(ORDERBOOK_ID).value()->GetOrderbookId(),
              ORDERBOOK_ID);
    EXPECT_EQ(orderbook_manager_.GetOrderbook(SECOND_ORDERBOOK_ID).value()->GetOrderbookId(),
              SECOND_ORDERBOOK_ID);
}

TEST_F(OrderbookManagerTest, GetReturnsSameInstanceAcrossCalls) {
    EXPECT_EQ(orderbook_manager_.AddOrderbook(std::make_unique<Orderbook>(ORDERBOOK_ID)),
              StatusCode::Success);
    auto first_res = orderbook_manager_.GetOrderbook(ORDERBOOK_ID);
    auto second_res = orderbook_manager_.GetOrderbook(ORDERBOOK_ID);
    EXPECT_TRUE(first_res.has_value());
    EXPECT_TRUE(second_res.has_value());

    (void)first_res.value()->AddOrder(Order{.order_id = 1,
                                            .price = BASE_PRICE,
                                            .quantity = BASE_QUANTITY,
                                            .side = Side::BID,
                                            .timestamp = BASE_TIMESTAMP});

    EXPECT_EQ(second_res.value()->GetTopOrder(Side::BID).value().order_id, 1);
}

TEST_F(OrderbookManagerTest, DuplicateOrderbookIdRejected) {
    EXPECT_EQ(orderbook_manager_.AddOrderbook(std::make_unique<Orderbook>(ORDERBOOK_ID)),
              StatusCode::Success);

    EXPECT_EQ(orderbook_manager_.AddOrderbook(std::make_unique<Orderbook>(ORDERBOOK_ID)),
              StatusCode::DuplicateOrderbookId);
}

TEST_F(OrderbookManagerTest, GetUnknownOrderbookIdReturnsOrderbookNotFound) {
    auto res = orderbook_manager_.GetOrderbook(UNKNOWN_ORDERBOOK_ID);
    EXPECT_FALSE(res.has_value());
    EXPECT_EQ(res.error(), StatusCode::OrderbookNotFound);
}

TEST_F(OrderbookManagerTest, OrderbooksAreIsolated) {
    EXPECT_EQ(orderbook_manager_.AddOrderbook(std::make_unique<Orderbook>(ORDERBOOK_ID)),
              StatusCode::Success);
    EXPECT_EQ(orderbook_manager_.AddOrderbook(std::make_unique<Orderbook>(SECOND_ORDERBOOK_ID)),
              StatusCode::Success);

    (void)orderbook_manager_.GetOrderbook(ORDERBOOK_ID)
        .value()
        ->AddOrder(Order{.order_id = 1,
                         .price = BASE_PRICE,
                         .quantity = BASE_QUANTITY,
                         .side = Side::BID,
                         .timestamp = BASE_TIMESTAMP});

    EXPECT_EQ(orderbook_manager_.GetOrderbook(ORDERBOOK_ID)
                  .value()
                  ->GetTopOrder(Side::BID)
                  .value()
                  .order_id,
              1);
    EXPECT_FALSE(orderbook_manager_.GetOrderbook(SECOND_ORDERBOOK_ID)
                     .value()
                     ->GetTopOrder(Side::BID)
                     .has_value());
}

} // namespace Domain::Market::Testing
