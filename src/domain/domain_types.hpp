#pragma once
#include <cstdint>

namespace Domain {

using OrderIdType = uint64_t;
using OrderbookIdType = uint32_t;
using PriceType = int64_t;
using QuantityType = uint64_t;
using TimestampType = uint64_t;

template <typename T> inline constexpr T Invalid = static_cast<T>(-1);

enum class Side : uint8_t { BID = 0, ASK = 1, INVALID = Invalid<uint8_t> };

// probably only support simple order types
enum class TimeInForce : uint8_t { DAY = 0, ImmediateOrCancel = 1, FillOrKill = 2 };

/// POD Order type for domain, used for processing messages inside Domain
struct Order {
  public:
    OrderIdType order_id{Invalid<OrderIdType>};
    PriceType price{Invalid<PriceType>};
    QuantityType quantity{Invalid<QuantityType>};
    Side side{Side::INVALID};
    TimestampType timestamp{Invalid<TimestampType>}; // not sure about this
};

struct Trade {
  public:
    OrderIdType bid_order_id{Invalid<OrderIdType>};
    OrderIdType ask_order_id{Invalid<OrderIdType>};
    PriceType price{Invalid<PriceType>};
    QuantityType executed_quantity{Invalid<QuantityType>};
    TimestampType timestamp{Invalid<TimestampType>}; // not sure about this
};

} // namespace Domain
