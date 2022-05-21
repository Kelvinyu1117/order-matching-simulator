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

  template <Side side>
  void notifyTrader(OrderStyle style, TraderId traderId, OrderId orderId,
                    Symbol symbol, Price fillPrice, Quantity fillQuantity) {
    if (mTraderMap.find(traderId) != mTraderMap.end()) {

      mTraderMap[traderId]->notify<side>(style, orderId, symbol, fillPrice,
                                         fillQuantity);
    }
  }
  void notifyTraderAllFilled(TraderId traderId, OrderId orderId);

  const std::unordered_map<TraderId, std::shared_ptr<Trader>> &getTraderMap();

private:
  std::unordered_map<TraderId, std::shared_ptr<Trader>> mTraderMap;
};
} // namespace Core

#endif
