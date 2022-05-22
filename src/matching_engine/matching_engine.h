#ifndef MATCHING_ENGINE
#define MATCHING_ENGINE
#include "self_trade_handler.h"
#include <atomic>
#include <core/execution_context/execution_context.h>
#include <core/order_book/order_book.h>
#include <memory>
#include <order.h>
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
  MatchingEngine() = delete;
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

  template <Side side, OrderStyle style>
  OrderId insert(ExecutionContext &context, const TraderId &traderId,
                 const Symbol &symbol, const Quantity quantity) {
    static_assert(
        style == OrderStyle::MKT_ORDER,
        " This function template can only be instantiated by MKT_ORDER");

    auto &bookPtr = mBookMap[symbol];

    auto orderId = getNextOrderId();

    Order<side> order(traderId, orderId, symbol, bookPtr->getBest<side>(),
                      quantity);

    if constexpr (side == Side::BUY) {
      matchBuyOrder(context, bookPtr, order);
    } else {
      matchSellOrder(context, bookPtr, order);
    }
    return orderId;
  }

  template <Side side, OrderStyle style>
  OrderId insert(ExecutionContext &context, const TraderId &traderId,
                 const Symbol &symbol, const Price price,
                 const Quantity quantity) {
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

  const std::unordered_map<std::string, std::unique_ptr<OrderBook>> &
  getOrderBookMap();

private:
  OrderId getNextOrderId();

  template <Side side>
  OrderId insert_limit_order(ExecutionContext &context,
                             const TraderId &traderId, const Symbol &symbol,
                             const Price price, const Quantity quantity) {
    if (price == 0 || quantity == 0) {
      return mOrderId.load(std::memory_order_relaxed);
    }

    auto orderId = getNextOrderId();

    Order<side> order(traderId, orderId, symbol, price, quantity);

    bool matched = tryMatchLimitOrder<side>(context, order);

    if (!matched) {
      mBookMap[symbol]->insert<side>(order);
    }

    return orderId;
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
                     std::unique_ptr<OrderBook> &bookPtr,
                     Order<Side::BUY> &order);

  bool matchSellOrder(ExecutionContext &context,
                      std::unique_ptr<OrderBook> &bookPtr,
                      Order<Side::SELL> &order);

  bool isSelfTradePreventionEnable();

private:
  std::atomic<OrderId> mOrderId{0};
  std::unordered_map<Symbol, std::unique_ptr<OrderBook>> mBookMap;
  std::shared_ptr<MatchingEngineConfig> mConfig;
};

#endif
