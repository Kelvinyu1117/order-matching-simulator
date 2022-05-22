#ifndef CORE_EXECUTION_CONTEXT
#define CORE_EXECUTION_CONTEXT
#include <memory>
#include <optional>
#include <trader.h>
#include <types.h>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace Common;
namespace Core {
class ExecutionContext {
public:
  ExecutionContext() = default;
  ExecutionContext(const std::vector<TraderId> &traderIds) {
    addTraders(traderIds);
  }

  ExecutionContext(const ExecutionContext &other) = default;
  ExecutionContext &operator=(const ExecutionContext &) = default;
  ExecutionContext(ExecutionContext &&other) = default;
  ExecutionContext &operator=(ExecutionContext &&other) = default;

  void addTraders(const std::vector<TraderId> &traderIds);

  template <OrderStatus status>
  void notifyTrader(TraderId traderId, OrderId orderId) {
    static_assert(
        status == OrderStatus::CANCEL || status == OrderStatus::CANCEL_REJECT,
        "This function template can only be instantiated by MKT_ORDER");

    if constexpr (status == OrderStatus::CANCEL) {
      mTraderMap[traderId]->notifyCancel(orderId);
    } else if constexpr (status == OrderStatus::CANCEL_REJECT) {
      mTraderMap[traderId]->notifyCancelReject(orderId);
    }
  }

  template <Side side, OrderStatus status>
  void notifyTrader(OrderStyle style, TraderId traderId, OrderId orderId,
                    Symbol symbol, Price price, Quantity quantity) {

    if (mTraderMap.find(traderId) != mTraderMap.end()) {

      if constexpr (status == OrderStatus::FILLED) {
        mTraderMap[traderId]->notifyFill<side>(style, orderId, symbol, price,
                                               quantity);
      } else if constexpr (status == OrderStatus::CANCEL) {
        mTraderMap[traderId]->notifyCancel<side>(orderId);
      } else if constexpr (status == OrderStatus::CANCEL_REJECT) {
        mTraderMap[traderId]->notifyCancelReject(orderId);
      } else {
        mTraderMap[traderId]->notifyOpen<side>(style, orderId, symbol, price,
                                               quantity);
      }
    }
  }

  void notifyTraderAllFilled(TraderId traderId, OrderId orderId);

  const std::unordered_map<TraderId, std::shared_ptr<Trader>> &getTraderMap();

private:
  std::unordered_map<TraderId, std::shared_ptr<Trader>> mTraderMap;
};
} // namespace Core

#endif
