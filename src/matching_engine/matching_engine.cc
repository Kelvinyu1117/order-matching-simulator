#include "matching_engine.h"

using namespace Core;

void MatchingEngine::addStocks(const std::vector<Symbol> &symbols) {
  for (const auto &symbol : symbols) {
    if (mBookMap.find(symbol) == mBookMap.end()) {
      mBookMap[symbol] = std::make_shared<OrderBook>();
    }
  }
}

void MatchingEngine::addConfig(std::shared_ptr<MatchingEngineConfig> config) {
  mConfig = config;
}

std::unordered_map<std::string, std::shared_ptr<OrderBook>>
MatchingEngine::getOrderBookMap() const {
  return mBookMap;
}

bool MatchingEngine::isSelfTradePreventionEnable() const {
  return mConfig && mConfig->selfTradPreventionConfig &&
         mConfig->selfTradPreventionConfig->enable;
}

std::uint64_t MatchingEngine::getNextOrderId() {
  return mOrderId.fetch_add(1, std::memory_order_relaxed);
}

bool MatchingEngine::matchBuyOrder(ExecutionContext &context,
                                   std::shared_ptr<OrderBook> &bookPtr,
                                   Order<Side::BUY> &order) {
  if (bookPtr->template getNumOfLevels<Side::SELL>() == 0) {
    return false;
  }

  auto it = bookPtr->template begin<Side::SELL>();
  auto best_ask = it->first;
  auto px = order.getPrice();
  auto amt = order.getQuantity();

  if (px < best_ask) {
    return false;
  }

  bool isDone = false;

  while (!isDone && px >= it->first &&
         it != bookPtr->template end<Side::SELL>()) {

    auto &orderQueue = it->second;
    bool isOrderCompleted = false;

    while (!orderQueue.empty() && !isOrderCompleted) {
      if (isSelfTradePreventionEnable() &&
          orderQueue.front().getTraderId() == order.getTraderId()) {
        SelfTradeHandler::dispatch<OrderStyle::LIMIT_ORDER, Side::SELL,
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
            frontOrderTraderId, frontOrderId, order.getSymbol(), frontOrderPx,
            matchedQty);

        frontOrder.setQuantity(frontOrderQty - matchedQty);
        if (frontOrder.getQuantity() == 0) {
          context.notifyTraderAllFilled(frontOrderTraderId, frontOrderId);
          orderQueue.pop();
        }

        context.notifyTrader<Side::BUY, OrderStyle::LIMIT_ORDER,
                             OrderStatus::FILLED>(
            order.getTraderId(), order.getOrderId(), order.getSymbol(), fillpx,
            matchedQty);

        amt = std::max(static_cast<Quantity>(0), amt - matchedQty);
        order.setQuantity(amt);

        isOrderCompleted = amt == 0;
      }
    }

    isDone = order.getQuantity() == 0;
    if (isDone) {
      context.notifyTraderAllFilled(order.getTraderId(), order.getOrderId());
    }
    if (orderQueue.empty()) {
      bookPtr->erase(it++);
    }
  }

  return isDone;
}

bool MatchingEngine::matchSellOrder(ExecutionContext &context,
                                    std::shared_ptr<OrderBook> &bookPtr,
                                    Order<Side::SELL> &order) {

  bool should_proceed = true;

  if (bookPtr->template getNumOfLevels<Side::BUY>() == 0) {
    return false;
  }

  auto it = bookPtr->template begin<Side::BUY>();
  auto best_bid = it->first;
  auto px = order.getPrice();
  auto amt = order.getQuantity();

  if (px > best_bid) {
    return false;
  }

  bool isDone = false;

  while (!isDone && px <= it->first &&
         it != bookPtr->template end<Side::BUY>()) {
    auto &orderQueue = it->second;
    bool isOrderCompleted = false;

    while (!orderQueue.empty() && !isOrderCompleted) {

      if (isSelfTradePreventionEnable() &&
          orderQueue.front().getTraderId() == order.getTraderId()) {
        SelfTradeHandler::dispatch<OrderStyle::LIMIT_ORDER, Side::BUY,
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
        context.notifyTrader<Side::SELL, OrderStyle::LIMIT_ORDER,
                             OrderStatus::FILLED>(
            order.getTraderId(), order.getOrderId(), order.getSymbol(), fillpx,
            matchedQty);

        amt = std::max(static_cast<Quantity>(0), amt - matchedQty);

        order.setQuantity(amt);
        isOrderCompleted = amt == 0;
      }
    }

    isDone = order.getQuantity() == 0;
    if (isDone) {
      context.notifyTraderAllFilled(order.getTraderId(), order.getOrderId());
    }

    if (orderQueue.empty()) {
      bookPtr->erase(it++);
    }
  }

  return isDone;
}
