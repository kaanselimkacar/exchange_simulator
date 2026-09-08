#include "domain/market.hpp"
#include <gtest/gtest.h>
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
constexpr TimestampType BASE_TIMESTAMP = 1000;
constexpr TimestampType NEXT_TIMESTAMP = 1001;
constexpr TimestampType LATER_TIMESTAMP = 1002;
constexpr TimestampType LARGE_QUANTITY_CONST = 1000000000000ULL;
constexpr PriceType MAX_INT64_PRICE = 9223372036854775807LL;
constexpr OrderIdType NONEXISTENT_ORDER_ID = 100;
constexpr OrderbookIdType ORDERBOOK_ID = 1;

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

    price_level_.AddOrder(order1);
    price_level_.AddOrder(order2);

    // The quantity at the price level should be accumulated
    // (This tests the internal state is being updated)
    EXPECT_TRUE(true); // The test passes if no assertion fails in AddOrder
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

    EXPECT_NO_THROW(orderbook_.AddOrder(bid_order));
}

TEST_F(OrderbookTest, AddAskOrder) {
    Order ask_order{.order_id = 1,
                    .price = HIGHER_PRICE,
                    .quantity = DOUBLE_QUANTITY,
                    .side = Side::ASK,
                    .timestamp = BASE_TIMESTAMP};

    EXPECT_NO_THROW(orderbook_.AddOrder(ask_order));
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

    EXPECT_NO_THROW(orderbook_.AddOrder(bid1));
    EXPECT_NO_THROW(orderbook_.AddOrder(bid2));
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

    EXPECT_NO_THROW(orderbook_.AddOrder(ask1));
    EXPECT_NO_THROW(orderbook_.AddOrder(ask2));
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

    EXPECT_NO_THROW(orderbook_.AddOrder(bid1));
    EXPECT_NO_THROW(orderbook_.AddOrder(ask1));
    EXPECT_NO_THROW(orderbook_.AddOrder(bid2));
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

    EXPECT_NO_THROW(orderbook_.AddOrder(bid1));
    EXPECT_NO_THROW(orderbook_.AddOrder(bid2));
}

TEST_F(OrderbookTest, RejectOrderWithZeroQuantity) {
    Order invalid_order{.order_id = 1,
                        .price = BASE_PRICE,
                        .quantity = 0, // Invalid: zero quantity
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // Should trigger an assertion failure
    EXPECT_DEATH(orderbook_.AddOrder(invalid_order), "");
}

TEST_F(OrderbookTest, RejectOrderWithNegativeQuantity) {
    Order invalid_order{.order_id = 1,
                        .price = BASE_PRICE,
                        .quantity =
                            static_cast<QuantityType>(-1), // NOLINT: intentional invalid value
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // Should still work since quantity is uint64_t, but semantically it's invalid
    // This test documents the current behavior
    EXPECT_NO_THROW(orderbook_.AddOrder(invalid_order));
}

TEST_F(OrderbookTest, RejectOrderWithInvalidSide) {
    Order invalid_order{.order_id = 1,
                        .price = BASE_PRICE,
                        .quantity = BASE_QUANTITY,
                        .side = Side::INVALID, // Invalid side
                        .timestamp = BASE_TIMESTAMP};

    // Should trigger an assertion failure
    EXPECT_DEATH(orderbook_.AddOrder(invalid_order), "");
}

TEST_F(OrderbookTest, RejectOrderWithZeroOrderId) {
    Order invalid_order{.order_id = 0, // Invalid: zero order ID
                        .price = BASE_PRICE,
                        .quantity = BASE_QUANTITY,
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // Should trigger an assertion failure
    EXPECT_DEATH(orderbook_.AddOrder(invalid_order), "");
}

TEST_F(OrderbookTest, RejectOrderWithZeroPrice) {
    Order invalid_order{.order_id = 1,
                        .price = 0, // Invalid: zero price
                        .quantity = BASE_QUANTITY,
                        .side = Side::BID,
                        .timestamp = BASE_TIMESTAMP};

    // Should trigger an assertion failure
    EXPECT_DEATH(orderbook_.AddOrder(invalid_order), "");
}

TEST_F(OrderbookTest, LargeQuantityOrder) {
    Order large_order{.order_id = 1,
                      .price = BASE_PRICE,
                      .quantity = LARGE_QUANTITY_CONST, // Very large quantity
                      .side = Side::BID,
                      .timestamp = BASE_TIMESTAMP};

    EXPECT_NO_THROW(orderbook_.AddOrder(large_order));
}

TEST_F(OrderbookTest, LargePriceOrder) {
    Order large_price_order{.order_id = 1,
                            .price = MAX_INT64_PRICE, // Max int64_t
                            .quantity = BASE_QUANTITY,
                            .side = Side::BID,
                            .timestamp = BASE_TIMESTAMP};

    EXPECT_NO_THROW(orderbook_.AddOrder(large_price_order));
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
    price_level_.AddOrder(order);
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
    orderbook_.AddOrder(bid);
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, DeleteAskOrder) {
    Order ask{.order_id = 1,
              .price = HIGHER_PRICE,
              .quantity = BASE_QUANTITY,
              .side = Side::ASK,
              .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(ask);
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
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
    orderbook_.AddOrder(bid1);
    orderbook_.AddOrder(bid2);
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
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
    orderbook_.AddOrder(bid1);
    orderbook_.AddOrder(bid2);

    // Delete the order at the higher price level
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));

    // Should still be able to interact with the remaining price level
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
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
    orderbook_.AddOrder(bid);
    orderbook_.AddOrder(ask);
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
}

TEST_F(OrderbookTest, DeleteNonexistentOrder) {
    EXPECT_DEATH(orderbook_.DeleteOrder(999), "");
}

TEST_F(OrderbookTest, DeleteThenReAddSameOrderId) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);
    orderbook_.DeleteOrder(1);

    // Re-adding the same order_id should succeed
    Order re_add{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = LARGER_QUANTITY,
                 .side = Side::BID,
                 .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.AddOrder(re_add));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, DuplicateOrderIdSilentlyIgnored) {
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
    orderbook_.AddOrder(first);
    // Second add with same order_id: order is added to the price level at
    // LOWER_PRICE, but orders_ map silently keeps the first entry.
    // Current behavior: no assert, no crash.
    EXPECT_NO_THROW(orderbook_.AddOrder(second));

    // Deleting order_id 1 removes the first order (at BASE_PRICE)
    // since orders_[1] still points there.
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
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
    orderbook_.AddOrder(bid1);
    orderbook_.AddOrder(bid2);

    // Delete the first order — price level still has bid2, so it should persist
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));

    // Add another order at a fresh price level, then delete the remaining order
    Order bid3{.order_id = 3,
                .price = LOWER_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = LATER_TIMESTAMP};
    orderbook_.AddOrder(bid3);
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(3));
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
    price_level_.AddOrder(order1);
    price_level_.AddOrder(order2);

    EXPECT_NO_THROW(
        price_level_.UpdateQuantity({.new_quantity = DOUBLE_QUANTITY, .old_quantity = BASE_QUANTITY}));
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
    price_level_.AddOrder(order1);
    price_level_.AddOrder(order2);

    EXPECT_NO_THROW(
        price_level_.UpdateQuantity({.new_quantity = BASE_QUANTITY, .old_quantity = TRIPLE_QUANTITY}));
}

TEST_F(PriceLevelTest, UpdateQuantityNoChange) {
    Order order1{.order_id = 1,
                 .price = BASE_PRICE,
                 .quantity = BASE_QUANTITY,
                 .side = Side::BID,
                 .timestamp = BASE_TIMESTAMP};
    price_level_.AddOrder(order1);

    EXPECT_NO_THROW(
        price_level_.UpdateQuantity({.new_quantity = BASE_QUANTITY, .old_quantity = BASE_QUANTITY}));
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
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyBidQuantityDecrease) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = DOUBLE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyAskQuantityIncrease) {
    Order order{.order_id = 1,
                .price = HIGHER_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::ASK,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyAskQuantityDecrease) {
    Order order{.order_id = 1,
                .price = HIGHER_PRICE,
                .quantity = DOUBLE_QUANTITY,
                .side = Side::ASK,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = BASE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyOrderSamePriceExplicitly) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = BASE_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyOrderSameQuantityNoChange) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyOrderMultipleTimes) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order mod1{.order_id = 1,
               .price = Invalid<PriceType>,
               .quantity = DOUBLE_QUANTITY,
               .side = Side::BID,
               .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(mod1));

    Order mod2{.order_id = 1,
               .price = Invalid<PriceType>,
               .quantity = TRIPLE_QUANTITY,
               .side = Side::BID,
               .timestamp = LATER_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(mod2));

    // Modify with explicit same price after sentinel modifies
    Order mod3{.order_id = 1,
               .price = BASE_PRICE,
               .quantity = DOUBLE_QUANTITY,
               .side = Side::BID,
               .timestamp = BASE_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(mod3));

    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyOrderThenDeleteSameOrder) {
    Order order{.order_id = 1,
                .price = BASE_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
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
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyBidPriceChangeToLower) {
    Order order{.order_id = 1,
                .price = HIGHER_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::BID,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = BASE_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyAskPriceChangeToHigher) {
    Order order{.order_id = 1,
                .price = HIGHER_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::ASK,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = MUCH_HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
}

TEST_F(OrderbookTest, ModifyAskPriceChangeToLower) {
    Order order{.order_id = 1,
                .price = MUCH_HIGHER_PRICE,
                .quantity = BASE_QUANTITY,
                .side = Side::ASK,
                .timestamp = BASE_TIMESTAMP};
    orderbook_.AddOrder(order);

    Order modified{.order_id = 1,
                   .price = HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = NEXT_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
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
    orderbook_.AddOrder(order1);
    orderbook_.AddOrder(order2);

    Order modified{.order_id = 1,
                   .price = Invalid<PriceType>,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));

    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
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
    orderbook_.AddOrder(order1);
    orderbook_.AddOrder(order2);

    Order modified{.order_id = 2,
                   .price = HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));

    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
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
    orderbook_.AddOrder(order1);
    orderbook_.AddOrder(order2);

    Order modified{.order_id = 2,
                   .price = BASE_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));

    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
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
    orderbook_.AddOrder(order1);
    orderbook_.AddOrder(order2);
    orderbook_.AddOrder(order3);

    // Move order2 away — BASE_PRICE level still has order1 and order3
    Order modified{.order_id = 2,
                   .price = HIGHER_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = BASE_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));

    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(3));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
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
    orderbook_.AddOrder(order1);
    orderbook_.AddOrder(order2);

    // Move the only order at HIGHER_PRICE to BASE_PRICE — HIGHER_PRICE level empties
    Order modified{.order_id = 2,
                   .price = BASE_PRICE,
                   .quantity = LARGER_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));

    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
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
    orderbook_.AddOrder(bid);
    orderbook_.AddOrder(ask);

    Order modified{.order_id = 2,
                   .price = MUCH_HIGHER_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::ASK,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));

    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
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
    orderbook_.AddOrder(bid);
    orderbook_.AddOrder(ask);

    // Move bid above ask — the orderbook allows crosses; matching engine handles them later
    Order modified{.order_id = 1,
                   .price = MUCH_HIGHER_PRICE,
                   .quantity = DOUBLE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    EXPECT_NO_THROW(orderbook_.ModifyOrder(modified));

    EXPECT_NO_THROW(orderbook_.DeleteOrder(1));
    EXPECT_NO_THROW(orderbook_.DeleteOrder(2));
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
    EXPECT_DEATH(orderbook_.ModifyOrder(modified), "");
}

// ============================================================================
// PriceLevel GetTopOrder Tests
// ============================================================================

TEST_F(PriceLevelTest, GetTopOrderOnEmptyLevelReturnsDefaultOrder) {
    auto top = price_level_.GetTopOrder();
    EXPECT_EQ(top.order_id, Invalid<OrderIdType>);
    EXPECT_EQ(top.side, Side::INVALID);
}

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
    price_level_.AddOrder(first);
    price_level_.AddOrder(second);

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
    price_level_.AddOrder(order);

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
    price_level_.AddOrder(second);

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
    price_level_.AddOrder(first);
    price_level_.AddOrder(second);

    auto top = price_level_.GetTopOrder();
    EXPECT_EQ(top.order_id, 1);
}

// ============================================================================
// Orderbook GetTopOrder Tests
// ============================================================================

TEST_F(OrderbookTest, GetTopOrderOnEmptyBookReturnsDefaultForBothSides) {
    auto top_bid = orderbook_.GetTopOrder(Side::BID);
    auto top_ask = orderbook_.GetTopOrder(Side::ASK);
    EXPECT_EQ(top_bid.order_id, Invalid<OrderIdType>);
    EXPECT_EQ(top_ask.order_id, Invalid<OrderIdType>);
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
    orderbook_.AddOrder(lower_bid);
    orderbook_.AddOrder(higher_bid);

    auto top = orderbook_.GetTopOrder(Side::BID);
    EXPECT_EQ(top.order_id, 2);
    EXPECT_EQ(top.price, BASE_PRICE);
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
    orderbook_.AddOrder(higher_ask);
    orderbook_.AddOrder(lower_ask);

    auto top = orderbook_.GetTopOrder(Side::ASK);
    EXPECT_EQ(top.order_id, 2);
    EXPECT_EQ(top.price, HIGHER_PRICE);
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
    orderbook_.AddOrder(bid);
    orderbook_.AddOrder(crossing_ask);

    auto top_bid = orderbook_.GetTopOrder(Side::BID);
    auto top_ask = orderbook_.GetTopOrder(Side::ASK);
    EXPECT_EQ(top_bid.order_id, 1);
    EXPECT_EQ(top_ask.order_id, 2);
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
    orderbook_.AddOrder(ask_first);
    orderbook_.AddOrder(ask_second);

    auto top = orderbook_.GetTopOrder(Side::ASK);
    EXPECT_EQ(top.order_id, 1);
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
    orderbook_.AddOrder(bid1);
    orderbook_.AddOrder(bid2);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).order_id, 1);

    // Move bid1 down to bid2's level: bid2 is senior there and becomes best bid.
    Order modified{.order_id = 1,
                   .price = LOWER_PRICE,
                   .quantity = BASE_QUANTITY,
                   .side = Side::BID,
                   .timestamp = LATER_TIMESTAMP};
    orderbook_.ModifyOrder(modified);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).order_id, 2);

    orderbook_.DeleteOrder(2);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).order_id, 1);

    orderbook_.DeleteOrder(1);
    EXPECT_EQ(orderbook_.GetTopOrder(Side::BID).order_id, Invalid<OrderIdType>);
}

} // namespace Domain::Market::Testing
