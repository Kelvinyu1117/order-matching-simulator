#include "execution_context.h"
namespace Core {
void ExecutionContext::addTraders(const std::vector<TraderId> &traderIds) {
  for (auto &id : traderIds) {
    mTraderMap[id] = std::make_shared<Trader>(id);
  }
}
void ExecutionContext::notifyTraderAllFilled(TraderId traderId,
                                             OrderId orderId) {
  if (mTraderMap.find(traderId) != mTraderMap.end()) {

    mTraderMap[traderId]->notifyAllFilled(orderId);
  }
}
std::unordered_map<TraderId, std::shared_ptr<Trader>>
ExecutionContext::getTraderMap() {
  return mTraderMap;
}
} // namespace Core
