#include "matching_engine.h"

using namespace Core;

void MatchingEngine::addStocks(const std::vector<Symbol> &symbols) {
  for (const auto &symbol : symbols) {
    if (mBookMap.find(symbol) == mBookMap.end()) {
      mBookMap[symbol] = std::make_unique<OrderBook>();
    }
  }
}

const std::unordered_map<std::string, std::unique_ptr<OrderBook>> &
MatchingEngine::getOrderBookMap() {
  return mBookMap;
}

std::uint64_t MatchingEngine::getNextOrderId() {
  return mOrderId.fetch_add(1, std::memory_order_relaxed);
}

bool MatchingEngine::matchBuyOrder(ExecutionContext &context,
                                   std::unique_ptr<OrderBook> &bookPtr,
                                   Order<Side::BUY> &order) {
  if (bookPtr->template getNumOfLevels<Side::SELL>() == 0) {
    return false;
  }

  auto it = bookPtr->template begin<Side::SELL>();
  auto px = order.getPrice();
  auto amt = order.getQuantity();

  if (px < it->first) {
    return false;
  }

  bool is_done = false;

  while (!is_done && px >= it->first &&
         it != bookPtr->template end<Side::SELL>()) {
    auto &orderQueue = it->second;

    bool isLevelCleared = false;
    while (!orderQueue.empty() && !isLevelCleared) {
      auto &frontOrder = orderQueue.front();
      auto frontOrderId = frontOrder.getOrderId();
      auto frontOrderTraderId = frontOrder.getTraderId();
      auto frontOrderPx = frontOrder.getPrice();
      auto frontOrderQty = frontOrder.getQuantity();
      auto matchedQty = std::min(frontOrderQty, amt);

      auto fillpx = std::min(frontOrderPx, order.getPrice());
      context.notifyTrader<Side::SELL>(
          order.getOrderStyle(), frontOrderTraderId, frontOrderId,
          order.getSymbol(), frontOrderPx, matchedQty);

      frontOrder.setQuantity(frontOrderQty - matchedQty);
      if (frontOrder.getQuantity() == 0) {
        context.notifyTraderAllFilled(frontOrderTraderId, frontOrderId);
        orderQueue.pop();
      }

      context.notifyTrader<Side::BUY>(order.getOrderStyle(),
                                      order.getTraderId(), order.getOrderId(),
                                      order.getSymbol(), fillpx, matchedQty);

      amt = std::max(static_cast<Quantity>(0), amt - matchedQty);
      order.setQuantity(amt);

      isLevelCleared = amt == 0;
    }

    is_done = order.getQuantity() == 0;
    if (is_done) {
      context.notifyTraderAllFilled(order.getTraderId(), order.getOrderId());
    }
    if (orderQueue.empty()) {
      bookPtr->erase(it++);
    }
  }

  return is_done;
}

bool MatchingEngine::matchSellOrder(ExecutionContext &context,
                                    std::unique_ptr<OrderBook> &bookPtr,
                                    Order<Side::SELL> &order) {

  if (bookPtr->template getNumOfLevels<Side::BUY>() == 0) {
    return false;
  }

  auto it = bookPtr->template begin<Side::BUY>();
  auto px = order.getPrice();
  auto amt = order.getQuantity();

  if (px > it->first) {
    return false;
  }

  bool is_done = false;

  while (!is_done && px <= it->first &&
         it != bookPtr->template end<Side::BUY>()) {
    auto &orderQueue = it->second;

    bool isLevelCleared = false;
    while (!orderQueue.empty() && !isLevelCleared) {
      auto &frontOrder = orderQueue.front();
      auto frontOrderId = frontOrder.getOrderId();
      auto frontOrderTraderId = frontOrder.getTraderId();
      auto frontOrderPx = frontOrder.getPrice();
      auto frontOrderQty = frontOrder.getQuantity();
      auto matchedQty = std::min(frontOrderQty, amt);
      auto fillpx = std::min(frontOrderPx, order.getPrice());

      context.notifyTrader<Side::BUY>(order.getOrderStyle(), frontOrderTraderId,
                                      frontOrderId, order.getSymbol(), fillpx,
                                      matchedQty);

      frontOrder.setQuantity(frontOrderQty - matchedQty);
      if (frontOrder.getQuantity() == 0) {
        context.notifyTraderAllFilled(frontOrderTraderId, frontOrderId);
        orderQueue.pop();
      }
      context.notifyTrader<Side::SELL>(order.getOrderStyle(),
                                       order.getTraderId(), order.getOrderId(),
                                       order.getSymbol(), fillpx, matchedQty);

      amt = std::max(static_cast<Quantity>(0), amt - matchedQty);

      order.setQuantity(amt);
      isLevelCleared = amt == 0;
    }

    is_done = order.getQuantity() == 0;
    if (is_done) {
      context.notifyTraderAllFilled(order.getTraderId(), order.getOrderId());
    }

    if (orderQueue.empty()) {
      bookPtr->erase(it++);
    }
  }

  return is_done;
}
