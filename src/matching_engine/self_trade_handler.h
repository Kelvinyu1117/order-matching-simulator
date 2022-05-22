#ifndef SELF_TRADE_HANDLER
#define SELF_TRADE_HANDLER
#include <core/execution_context/execution_context.h>
#include <core/order/order.h>
#include <core/order_book/order_book.h>
#include <deque>
#include <map>
#include <types.h>
#include <unordered_set>

using namespace Common;
using namespace Core;

/**
 * @brief
 * CANCEL_PASSIVE: cancel the existing order
 * CANCEL_ACTIVE: cancel the incoming order
 * CANCEL_BOTH: cancel both existing order and incoming order
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
  template <OrderStyle style, Side bookSide, Side orderSide>
  static void dispatch(SelfTradePreventionPolicy policy,
                       ExecutionContext &context, OrderQueue<bookSide> &queue,
                       Order<orderSide> &order) {
    static_assert(bookSide != orderSide,
                  "The BookSide should not be same as the OrderSide");
    if (style == OrderStyle::LIMIT_ORDER) {
      switch (policy) {
      case SelfTradePreventionPolicy::CANCEL_ACTIVE:
        cancelActiveLimitOrder<bookSide, orderSide>(context, queue, order);
        break;
      case SelfTradePreventionPolicy::CANCEL_BOTH:
        cancelBothLimitOrder<bookSide, orderSide>(context, queue, order);
        break;
      default:
        cancelPassiveLimitOrder<bookSide, orderSide>(context, queue, order);
      }
    } else {
      switch (policy) {
      case SelfTradePreventionPolicy::CANCEL_ACTIVE:
        cancelActiveMarketOrder<bookSide, orderSide>(context, queue, order);
        break;
      case SelfTradePreventionPolicy::CANCEL_BOTH:
        cancelBothMarketOrder<bookSide, orderSide>(context, queue, order);
        break;
      default:
        cancelPassiveMarketOrder<bookSide, orderSide>(context, queue, order);
      }
    }
  }

private:
  template <Side bookSide, Side orderSide>
  static void cancelActiveMarketOrder(ExecutionContext &context,
                                      OrderQueue<bookSide> &queue,
                                      Order<orderSide> &order) {
    context.notifyTrader<orderSide, OrderStyle::MKT_ORDER, OrderStatus::CANCEL>(
        order.getTraderId(), order.getOrderId(), order.getSymbol(),
        order.getPrice(), order.getQuantity(), OrderCancelReason::SELF_TRADE);

    order.setQuantity(0);
  }

  template <Side bookSide, Side orderSide>
  static void cancelBothMarketOrder(ExecutionContext &context,
                                    OrderQueue<bookSide> &queue,
                                    Order<orderSide> &order) {
    auto &frontOrder = queue.front();
    context.notifyTrader<orderSide, OrderStyle::MKT_ORDER, OrderStatus::CANCEL>(
        order.getTraderId(), order.getOrderId(), order.getSymbol(),
        order.getPrice(), order.getQuantity(), OrderCancelReason::SELF_TRADE);

    context
        .notifyTrader<bookSide, OrderStyle::LIMIT_ORDER, OrderStatus::CANCEL>(
            frontOrder.getTraderId(), frontOrder.getOrderId(),
            frontOrder.getSymbol(), frontOrder.getPrice(),
            frontOrder.getQuantity(), OrderCancelReason::SELF_TRADE);

    queue.pop();
    order.setQuantity(0);
  }

  template <Side bookSide, Side orderSide>
  static void cancelPassiveMarketOrder(ExecutionContext &context,
                                       OrderQueue<bookSide> &queue,
                                       Order<orderSide> &order) {
    auto &frontOrder = queue.front();
    context.notifyTrader<bookSide, OrderStyle::MKT_ORDER, OrderStatus::CANCEL>(
        frontOrder.getTraderId(), frontOrder.getOrderId(),
        frontOrder.getSymbol(), frontOrder.getPrice(), frontOrder.getQuantity(),
        OrderCancelReason::SELF_TRADE);

    queue.pop();
  }

  template <Side bookSide, Side orderSide>
  static void cancelActiveLimitOrder(ExecutionContext &context,
                                     OrderQueue<bookSide> &queue,
                                     Order<orderSide> &order) {
    context
        .notifyTrader<orderSide, OrderStyle::LIMIT_ORDER, OrderStatus::CANCEL>(
            order.getTraderId(), order.getOrderId(), order.getSymbol(),
            order.getPrice(), order.getQuantity(),
            OrderCancelReason::SELF_TRADE);
    order.setQuantity(0);
  }
  template <Side bookSide, Side orderSide>
  static void cancelBothLimitOrder(ExecutionContext &context,
                                   OrderQueue<bookSide> &queue,
                                   Order<orderSide> &order) {
    auto &frontOrder = queue.front();
    context
        .notifyTrader<orderSide, OrderStyle::LIMIT_ORDER, OrderStatus::CANCEL>(
            order.getTraderId(), order.getOrderId(), order.getSymbol(),
            order.getPrice(), order.getQuantity(),
            OrderCancelReason::SELF_TRADE);

    context
        .notifyTrader<bookSide, OrderStyle::LIMIT_ORDER, OrderStatus::CANCEL>(
            frontOrder.getTraderId(), frontOrder.getOrderId(),
            frontOrder.getSymbol(), frontOrder.getPrice(),
            frontOrder.getQuantity(), OrderCancelReason::SELF_TRADE);

    queue.pop();
    order.setQuantity(0);
  }
  template <Side bookSide, Side orderSide>
  static void cancelPassiveLimitOrder(ExecutionContext &context,
                                      OrderQueue<bookSide> &queue,
                                      Order<orderSide> &order) {
    auto &frontOrder = queue.front();
    context
        .notifyTrader<bookSide, OrderStyle::LIMIT_ORDER, OrderStatus::CANCEL>(
            frontOrder.getTraderId(), frontOrder.getOrderId(),
            frontOrder.getSymbol(), frontOrder.getPrice(),
            frontOrder.getQuantity(), OrderCancelReason::SELF_TRADE);

    queue.pop();
  }
};
#endif
