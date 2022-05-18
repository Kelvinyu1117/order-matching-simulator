#ifndef ORDER_BOOK
#define ORDER_BOOK
#include <common.h>
#include <deque>
#include <map>
#include <order.h>

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

  void push(const Order<side> &order) { mQueue.emplace(order); }

  void pop() { return mQueue.pop(); };

  bool empty() const { return mQueue.empty(); }

  size_t size() const { return mQueue.size(); }

private:
  std::deque<Order<side>> mQueue;
};
class OrderBook {
public:
  using BidSideIterator = std::map<Price, OrderQueue<Side::BUY>>::iterator;
  using AskSideIterator = std::map<Price, OrderQueue<Side::SELL>>::iterator;

  template <Side side> void insert(const Order<side> &order) {
    if constexpr (side == Side::BUY) {
      mBidSide[order.getPrice()].push(order);
    } else {
      mAskSide[order.getPrice()].push(order);
    }
  }

  template <Side side> void insert(const Order<side> &&order) {
    if constexpr (side == Side::BUY) {
      mBidSide[order.mPrice].push(std::move(order));
    } else {
      mAskSide[order.mPrice].push(std::move(order));
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
