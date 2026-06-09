#pragma once
#include <cstdint>
namespace Protocols::Example {

class ExampleConfig {
  public:
    // using OrderIdType = char[14];
    using OrderbookIdType = uint32_t;
    using PriceType = int32_t;
    using QuantityType = int64_t;
};
} // namespace Protocols::Example
