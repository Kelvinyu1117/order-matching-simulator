#ifndef COMMON_TYPES
#define COMMON_TYPES
#include <cstdint>
#include <string>
namespace Common {
// enum class MatchingStatus {
//   FILL,
//   CANCEL,
//   REJECTED,
// };

/**
 * @brief
 * Assume the price will be int64_t for easy implementation.
 * In real world application, it will be floating point, probably we need to use
 * two int for storing the integer part and the decimal part of the number , in
 * order to preserve the precision
 */
using Price = std::int64_t;
using Quantity = std::uint64_t;
using Symbol = std::string;
using OrderId = std::uint64_t;
using TraderId = std::string;

} // namespace Common
#endif
