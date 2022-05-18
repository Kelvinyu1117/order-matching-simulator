#ifndef COMMON
#define COMMON
#include <cstdint>
#include <string_view>
namespace Common {
enum class OrderType {
  MKT_ORDER,
  LIMIT_ORDER,
};

enum class Side { BUY, SELL };
/**
 * @brief
 * Assume the price will be int64_t for easy implementation.
 * In real world application, it will be floating point, probably we need to use
 * two int for storing the integer part and the decimal part of the number , in
 * order to preserve the precision
 */
using Price = std::int64_t;
using Quantity = std::uint64_t;
using Symbol = std::string_view;
using OrderId = std::uint64_t;
using ClientId = std::string_view;
} // namespace Common
#endif
