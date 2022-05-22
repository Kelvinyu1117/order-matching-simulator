#ifndef CORE_ORDER_BOOK
#define CORE_ORDER_BOOK
#include <deque>
#include <map>
#include <order/order.h>
#include <types.h>
#include <unordered_set>

using namespace Common;

namespace Core {
template <Side side> class OrderQueue {
public:
  OrderQueue() = default;
  OrderQueue(const OrderQueue &other) = default;
  OrderQueue<side> &operator=(const OrderQueue<side> &) = default;
  OrderQueue(OrderQueue<side> &&other) = default;
  OrderQueue<side> &operator=(OrderQueue<side> &&other) = default;

  auto &front() { return mQueue.front(); }

  void push(Order<side> &&order) { mQueue.emplace(std::move(order)); }

  void push(const Order<side> &order) {
    mQueue.push_back(order);
    mTotalQuantity += order.getQuantity();
    mTraderIds.insert(order.getTraderId());
  }

  void emplace(Order<side> &&order) {
    mQueue.emplace_back(std::forward<Order<side>>(order));
    mTotalQuantity += order.getQuantity();
    mTraderIds.insert(order.getTraderId());
  }

  void pop() {
    auto &target = mQueue.front();
    mTotalQuantity -= target.getQuantity();
    mTraderIds.erase(target.getTraderId());

    mQueue.pop_front();
  };

  void update(const Order<side> &order) {
    // preconditon: there is a order with the target trader id in the queue
    // the trader will lost its time priority when they update the order at the
    // same price level
    erase(order.getTraderId());
    push(order);
  }
  bool erase(TraderId id) {
    bool isFound = false;
    auto it = mQueue.begin();

    for (; it != mQueue.end() && !isFound; it++) {
      if (it->getTraderId() == id) {
        isFound = true;
      }
    }

    if (isFound) {
      mQueue.erase(it);
      mTraderIds.erase(id);
      mTotalQuantity -= it->getQuantity();
    }

    return isFound;
  }

  bool erase(OrderId orderId) {
    bool isFound = false;
    auto it = mQueue.begin();

    for (; it != mQueue.end() && !isFound; it++) {
      if (it->getOrderId() == orderId) {
        isFound = true;
      }
    }

    if (isFound) {
      mTraderIds.erase(it->getTraderId());
      mQueue.erase(it);
      mTotalQuantity -= it->getQuantity();
    }

    return isFound;
  }

  bool erase(OrderId orderId, TraderId traderId) {
    bool isFound = false;
    auto it = mQueue.begin();
    auto res = it;

    for (; it != mQueue.end() && !isFound; it++) {
      if (it->getOrderId() == orderId && it->getTraderId() == traderId) {
        isFound = true;
        res = it;
      }
    }

    if (isFound) {
      mTotalQuantity -= res->getQuantity();
      mTraderIds.erase(res->getTraderId());
      mQueue.erase(res);
    }

    return isFound;
  }

  bool contains(TraderId id) { return mTraderIds.find(id) != mTraderIds.end(); }
  bool empty() const { return mQueue.empty(); }

  size_t numOfOrders() const { return mQueue.size(); }
  std::int32_t totalQuantity() const { return mTotalQuantity; }

private:
  std::deque<Order<side>> mQueue;
  std::unordered_set<TraderId> mTraderIds;
  std::int32_t mTotalQuantity;
};
class OrderBook {
public:
  using BidSideIterator = std::map<Price, OrderQueue<Side::BUY>>::iterator;
  using AskSideIterator = std::map<Price, OrderQueue<Side::SELL>>::iterator;

  template <Side side> Price getBest() {
    if constexpr (side == Side::BUY) {
      return mBidSide.begin()->first;
    } else {
      return mAskSide.begin()->first;
    }
  }

  template <Side side> void clear() {
    if constexpr (side == Side::BUY) {
      return mBidSide.clear();
    } else {
      return mAskSide.clear();
    }
  }

  template <Side side> void insert(const Order<side> &order) {
    // the orderbook only contains limit order
    if (order.getOrderStyle() == OrderStyle::MKT_ORDER)
      return;

    if constexpr (side == Side::BUY) {
      if (mBidSide[order.getPrice()].contains(order.getTraderId())) {
        mBidSide[order.getPrice()].update(order);
      } else {
        mBidSide[order.getPrice()].push(order);
      }

    } else {
      if (mAskSide[order.getPrice()].contains(order.getTraderId())) {
        mAskSide[order.getPrice()].update(order);
      } else {
        mAskSide[order.getPrice()].push(order);
      }
    }
  }

  template <Side side> auto begin() {
    if constexpr (side == Side::BUY) {
      return mBidSide.begin();
    } else {
      return mAskSide.begin();
    }
  }

  template <Side side> auto end() {
    if constexpr (side == Side::BUY) {
      return mBidSide.end();
    } else {
      return mAskSide.end();
    }
  }

  bool removeOrder(OrderId orderId, TraderId traderId) {
    bool isFound = false;

    // Definitely it can be optimized by manipulating the reference of the order
    // in the queue and orderId and traderId, something like a multi-index
    // (OrderId, TraderId) to the order, but I dont have time to do it yet
    for (auto it = mBidSide.begin(); it != mBidSide.end() && !isFound; it++) {
      isFound = it->second.erase(orderId, traderId);
      if (it->second.empty()) {
        mBidSide.erase(it);
      }
    }

    for (auto it = mAskSide.begin(); it != mAskSide.end() && !isFound; it++) {
      isFound = it->second.erase(orderId, traderId);
      if (it->second.empty()) {
        mAskSide.erase(it);
      }
    }

    return isFound;
  }

  template <Side side> auto search(Price px) {
    if constexpr (side == Side::BUY) {
      return mBidSide.lower_bound(px);
    } else {
      return mAskSide.lower_bound(px);
    }
  }

  template <Side side> size_t getNumOfLevels() const {
    if constexpr (side == Side::BUY) {
      return mBidSide.size();
    } else {
      return mAskSide.size();
    }
  }
  BidSideIterator erase(const BidSideIterator &it);
  AskSideIterator erase(const AskSideIterator &it);

private:
  std::map<Price, OrderQueue<Side::BUY>, std::greater<>> mBidSide;
  std::map<Price, OrderQueue<Side::SELL>> mAskSide;
};

} // namespace Core
#endif
