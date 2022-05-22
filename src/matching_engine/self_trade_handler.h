#ifndef SELF_TRADE_HANDLER
#define SELF_TRADE_HANDLER
#include <core/execution_context/execution_context.h>
#include <core/order_book/order_book.h>
#include <deque>
#include <map>
#include <order.h>
#include <types.h>
#include <unordered_set>

using namespace Common;
using namespace Core;

/**
 * @todo
 *  add multiple handler for different policies
 *  return a flag to indicate is there any self-trade happen -> if it has been
 * handled, dont match
 */

enum class SelfTradePreventionPolicy {
  CANCEL_PASSIVE,
  CANCEL_ACTIVE,
  CANCEL_BOTH
};

struct SelfTradePreventionConfig {
  bool enable;
  SelfTradePreventionPolicy policy;
};

class SelfTradeHandler {
public:
  template <Side bookSide, Side orderSide>
  static void dispatch(SelfTradePreventionPolicy policy,
                       ExecutionContext &context, OrderQueue<bookSide> &queue,
                       Order<orderSide> &order) {
    static_assert(bookSide != orderSide,
                  "The BookSide should not be same as the OrderSide");
    switch (policy) {
    case SelfTradePreventionPolicy::CANCEL_ACTIVE:
      cancel_active<bookSide, orderSide>(context, queue, order);
      break;
    case SelfTradePreventionPolicy::CANCEL_BOTH:
      cancel_both<bookSide, orderSide>(context, queue, order);
      break;
    default:
      cancel_passive<bookSide, orderSide>(context, queue, order);
    }
  }

private:
  template <Side bookSide, Side orderSide>
  static void cancel_active(ExecutionContext &context,
                            OrderQueue<bookSide> &queue,
                            Order<orderSide> &order) {
    context.notifyTrader<orderSide, OrderStatus::CANCEL>(
        order.getOrderStyle(), order.getTraderId(), order.getOrderId(),
        order.getSymbol(), order.getPrice(), order.getQuantity());
  }
  template <Side bookSide, Side orderSide>
  static void cancel_both(ExecutionContext &context,
                          OrderQueue<bookSide> &queue,
                          Order<orderSide> &order) {
    auto &frontOrder = queue.front();
    context.notifyTrader<orderSide, OrderStatus::CANCEL>(
        order.getOrderStyle(), order.getTraderId(), order.getOrderId(),
        order.getSymbol(), order.getPrice(), order.getQuantity());

    context.notifyTrader<bookSide, OrderStatus::CANCEL>(
        frontOrder.getOrderStyle(), frontOrder.getTraderId(),
        frontOrder.getOrderId(), frontOrder.getSymbol(), frontOrder.getPrice(),
        frontOrder.getQuantity());

    queue.pop();
  }
  template <Side bookSide, Side orderSide>
  static void cancel_passive(ExecutionContext &context,
                             OrderQueue<bookSide> &queue,
                             Order<orderSide> &order) {
    auto &frontOrder = queue.front();
    context.notifyTrader<bookSide, OrderStatus::CANCEL>(
        frontOrder.getOrderStyle(), frontOrder.getTraderId(),
        frontOrder.getOrderId(), frontOrder.getSymbol(), frontOrder.getPrice(),
        frontOrder.getQuantity());

    queue.pop();
  }
};
#endif
