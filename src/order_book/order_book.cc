#include "order_book.h"
#include <iostream>

namespace Core {
OrderBook::BidSideIterator
OrderBook::erase(const OrderBook::BidSideIterator &it) {
  return mBidSide.erase(it);
}
OrderBook::AskSideIterator
OrderBook::erase(const OrderBook::AskSideIterator &it) {
  return mAskSide.erase(it);
}
} // namespace Core
