#include "order.h"

namespace Core {
std::string orderCancelReason2Str(OrderCancelReason rsn) {
  switch (rsn) {
  case OrderCancelReason::CANCEL_REQUEST:
    return "CANCEL REQUEST";
  case OrderCancelReason::SELF_TRADE:
    return "SELF TRADE";
  case OrderCancelReason::NO_ORDER_TO_MATCH_MKT_ORDER:
    return "NO ORDER TO MATCH THE MKT ORDER";
  default:
    return "NONE";
  }
}

std::string orderStyle2Str(OrderStyle style) {
  switch (style) {
  case OrderStyle::MKT_ORDER:
    return "MKT_ORDER";
  default:
    return "LIMIT ORDER";
  }
}
} // namespace Core
