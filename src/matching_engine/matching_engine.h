#ifndef MATCHING_ENGINE
#define MATCHING_ENGINE
#include "self_trade_handler.h"
#include <atomic>
#include <core/execution_context/execution_context.h>
#include <core/order/order.h>
#include <core/order_book/order_book.h>
#include <memory>
#include <types.h>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace Common;
using namespace Core;

struct MatchingEngineConfig {
  std::optional<SelfTradePreventionConfig> selfTradPreventionConfig;
};

class MatchingEngine {
public:
  MatchingEngine() = default;
  MatchingEngine(std::shared_ptr<MatchingEngineConfig> config)
      : mConfig(config) {}

  MatchingEngine(std::shared_ptr<MatchingEngineConfig> config,
                 const std::vector<Symbol> &stocks)
      : mConfig(config) {
    addStocks(stocks);
  }

  MatchingEngine(const MatchingEngine &other) = delete;
  MatchingEngine &operator=(const MatchingEngine &) = delete;
  MatchingEngine(MatchingEngine &&other) = delete;
  MatchingEngine &operator=(MatchingEngine &&other) = delete;
  void addStocks(const std::vector<Symbol> &symbols);
  void addConfig(std::shared_ptr<MatchingEngineConfig> config);

  template <Side side, OrderStyle style>
  OrderId insert(ExecutionContext &context, const TraderId &traderId,
                 const Symbol &symbol, Quantity quantity) {
    static_assert(
        style == OrderStyle::MKT_ORDER,
        " This function template can only be instantiated by MKT_ORDER");

    auto &bookPtr = mBookMap[symbol];
    // the execution of the market order is guaranteed

    auto orderId = getNextOrderId();

    auto price = (side == Side::BUY) ? bookPtr->template getBest<Side::SELL>()
                                     : bookPtr->template getBest<Side::BUY>();
    Order<side> order(OrderStyle::MKT_ORDER, traderId, orderId, symbol, price,
                      quantity);

    matchMarketOrder<side>(context, traderId, bookPtr, order);
    return mOrderId.load(std::memory_order_relaxed);
  }

  template <Side side, OrderStyle style>
  OrderId insert(ExecutionContext &context, const TraderId &traderId,
                 const Symbol &symbol, const Price price, Quantity quantity) {
    static_assert(
        style == OrderStyle::LIMIT_ORDER,
        " This function template can only be instantiated by LIMIT_ORDER");
    return insert_limit_order<side>(context, traderId, symbol, price, quantity);
  }

  void cancel(ExecutionContext &context,
              const OrderCancelRequest &cancelRequest) {

    bool isCancelled = mBookMap[cancelRequest.mSymbol]->removeOrder(
        cancelRequest.mOrderId, cancelRequest.mTraderId);

    if (isCancelled) {
      context.notifyTrader<OrderStatus::CANCEL>(cancelRequest.mTraderId,
                                                cancelRequest.mOrderId);
    } else {
      context.notifyTrader<OrderStatus::CANCEL_REJECT>(cancelRequest.mTraderId,
                                                       cancelRequest.mOrderId);
    }
  }

  std::unordered_map<std::string, std::shared_ptr<OrderBook>>
  getOrderBookMap() const;

private:
  OrderId getNextOrderId();

  template <Side side>
  OrderId insert_limit_order(ExecutionContext &context,
                             const TraderId &traderId, const Symbol &symbol,
                             const Price price, Quantity quantity) {
    if (price == 0 || quantity == 0) {
      return mOrderId.load(std::memory_order_relaxed);
    }

    auto orderId = getNextOrderId();

    Order<side> order(OrderStyle::LIMIT_ORDER, traderId, orderId, symbol, price,
                      quantity);

    bool matched = tryMatchLimitOrder<side>(context, order);

    if (!matched) {
      mBookMap[symbol]->insert<side>(order);
      context.notifyTrader<side, OrderStyle::LIMIT_ORDER, OrderStatus::OPEN>(
          order.getTraderId(), order.getOrderId(), order.getSymbol(),
          order.getPrice(), order.getQuantity());
    }

    return orderId;
  }

  template <Side side>
  void matchMarketOrder(ExecutionContext &context, const TraderId &traderId,
                        std::shared_ptr<OrderBook> &bookPtr,
                        Order<side> order) {

    if constexpr (side == Side::BUY) {

      auto it = bookPtr->template begin<Side::SELL>();
      auto amt = order.getQuantity();
      bool isDone = false;

      while (!isDone && it != bookPtr->template end<Side::SELL>()) {

        auto &orderQueue = it->second;
        bool isOrderCompleted = false;
        order.setPrice(it->first);

        while (!orderQueue.empty() && !isOrderCompleted) {
          if (isSelfTradePreventionEnable() &&
              orderQueue.front().getTraderId() == order.getTraderId()) {
            SelfTradeHandler::dispatch<OrderStyle::MKT_ORDER, Side::SELL,
                                       Side::BUY>(
                mConfig->selfTradPreventionConfig->policy, context, orderQueue,
                order);
            isOrderCompleted = !order.getQuantity();
          } else {
            auto &frontOrder = orderQueue.front();
            auto frontOrderId = frontOrder.getOrderId();
            auto frontOrderTraderId = frontOrder.getTraderId();
            auto frontOrderPx = frontOrder.getPrice();
            auto frontOrderQty = frontOrder.getQuantity();
            auto matchedQty = std::min(frontOrderQty, amt);

            auto fillpx = std::min(frontOrderPx, order.getPrice());
            context.notifyTrader<Side::SELL, OrderStyle::LIMIT_ORDER,
                                 OrderStatus::FILLED>(
                frontOrderTraderId, frontOrderId, order.getSymbol(),
                frontOrderPx, matchedQty);

            frontOrder.setQuantity(frontOrderQty - matchedQty);
            if (frontOrder.getQuantity() == 0) {
              context.notifyTraderAllFilled(frontOrderTraderId, frontOrderId);
              orderQueue.pop();
            }

            context.notifyTrader<Side::BUY, OrderStyle::MKT_ORDER,
                                 OrderStatus::FILLED>(
                order.getTraderId(), order.getOrderId(), order.getSymbol(),
                fillpx, matchedQty);

            amt = std::max(static_cast<Quantity>(0), amt - matchedQty);
            order.setQuantity(amt);

            isOrderCompleted = amt == 0;
          }
        }

        isDone = order.getQuantity() == 0;
        if (isDone) {
          context.notifyTraderAllFilled(order.getTraderId(),
                                        order.getOrderId());
        }

        if (orderQueue.empty()) {
          bookPtr->erase(it++);
        }
      }

      if (!isDone) {
        context.notifyTrader<side, OrderStyle::MKT_ORDER, OrderStatus::CANCEL>(
            order.getTraderId(), order.getOrderId(), order.getSymbol(), 0,
            order.getQuantity(),
            OrderCancelReason::NO_ORDER_TO_MATCH_MKT_ORDER);
        return;
      }

    } else {
      auto it = bookPtr->template begin<Side::BUY>();
      auto amt = order.getQuantity();
      bool isDone = false;

      while (!isDone && it != bookPtr->template end<Side::BUY>()) {
        auto &orderQueue = it->second;
        bool isOrderCompleted = false;

        while (!orderQueue.empty() && !isOrderCompleted) {

          if (isSelfTradePreventionEnable() &&
              orderQueue.front().getTraderId() == order.getTraderId()) {
            SelfTradeHandler::dispatch<OrderStyle::MKT_ORDER, Side::BUY,
                                       Side::SELL>(
                mConfig->selfTradPreventionConfig->policy, context, orderQueue,
                order);
            isOrderCompleted = !order.getQuantity();
          } else {
            auto &frontOrder = orderQueue.front();
            auto frontOrderId = frontOrder.getOrderId();
            auto frontOrderTraderId = frontOrder.getTraderId();
            auto frontOrderPx = frontOrder.getPrice();
            auto frontOrderQty = frontOrder.getQuantity();
            auto matchedQty = std::min(frontOrderQty, amt);
            auto fillpx = std::min(frontOrderPx, order.getPrice());

            context.notifyTrader<Side::BUY, OrderStyle::LIMIT_ORDER,
                                 OrderStatus::FILLED>(
                frontOrderTraderId, frontOrderId, order.getSymbol(), fillpx,
                matchedQty);

            frontOrder.setQuantity(frontOrderQty - matchedQty);
            if (frontOrder.getQuantity() == 0) {
              context.notifyTraderAllFilled(frontOrderTraderId, frontOrderId);
              orderQueue.pop();
            }
            context.notifyTrader<Side::SELL, OrderStyle::MKT_ORDER,
                                 OrderStatus::FILLED>(
                order.getTraderId(), order.getOrderId(), order.getSymbol(),
                fillpx, matchedQty);

            amt = std::max(static_cast<Quantity>(0), amt - matchedQty);

            order.setQuantity(amt);
            isOrderCompleted = amt == 0;
          }
        }

        isDone = order.getQuantity() == 0;
        if (isDone) {
          context.notifyTraderAllFilled(order.getTraderId(),
                                        order.getOrderId());
        }

        if (orderQueue.empty()) {
          bookPtr->erase(it++);
        }
      }

      if (!isDone) {
        context.notifyTrader<side, OrderStyle::MKT_ORDER, OrderStatus::CANCEL>(
            order.getTraderId(), order.getOrderId(), order.getSymbol(), 0,
            order.getQuantity(),
            OrderCancelReason::NO_ORDER_TO_MATCH_MKT_ORDER);
        return;
      }
    }
  }

  template <Side side>
  bool tryMatchLimitOrder(ExecutionContext &context, Order<side> &order) {
    auto &bookPtr = mBookMap[order.getSymbol()];

    if constexpr (side == Side::BUY) {
      return matchBuyOrder(context, bookPtr, order);
    } else {
      return matchSellOrder(context, bookPtr, order);
    }
  }

  bool matchBuyOrder(ExecutionContext &context,
                     std::shared_ptr<OrderBook> &bookPtr,
                     Order<Side::BUY> &order);

  bool matchSellOrder(ExecutionContext &context,
                      std::shared_ptr<OrderBook> &bookPtr,
                      Order<Side::SELL> &order);

  bool isSelfTradePreventionEnable() const;

private:
  std::atomic<OrderId> mOrderId{0};
  std::unordered_map<Symbol, std::shared_ptr<OrderBook>> mBookMap;
  std::shared_ptr<MatchingEngineConfig> mConfig;
};

#endif
