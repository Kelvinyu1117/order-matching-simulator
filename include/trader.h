#ifndef COMMON_TRADER
#define COMMON_TRADER
#include "order.h"
#include "types.h"
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

namespace Common {

template <Side side> class Order;
class Trader {
public:
  Trader() = default;
  Trader(std::string traderId);
  Trader(const Trader &other) = default;
  Trader &operator=(const Trader &) = default;
  Trader(Trader &&other) = default;
  Trader &operator=(Trader &&other) = default;

  void notifyAllFilled(OrderId orderId) {
    std::cout << mTraderId << " orderid:" << orderId << " "
              << "is successfully filled\n";
  }

  template <Side side>
  void notify(OrderStyle style, OrderId orderId, Symbol symbol, Price fillPrice,
              Quantity fillQuantity) {

    if constexpr (side == Side::BUY) {
      std::cout << "Fill! " << mTraderId << " "
                << "BUY " << fillQuantity << " " << symbol << " at "
                << fillPrice << '\n';

      mFilledBuyOrders.emplace_back(Order<side>(
          style, mTraderId, orderId, symbol, fillPrice, fillQuantity));
    } else {
      std::cout << "Fill! " << mTraderId << " "
                << "SELL " << fillQuantity << " " << symbol << " at "
                << fillPrice << '\n';

      mFilledSellOrders.emplace_back(Order<side>(
          style, mTraderId, orderId, symbol, fillPrice, fillQuantity));
    }
  }

  const std::vector<Order<Side::BUY>> &getFilledBuyOrders() const {
    return mFilledBuyOrders;
  }

  const std::vector<Order<Side::SELL>> &getFilledSellOrders() const {
    return mFilledSellOrders;
  }

private:
  std::string mTraderId;
  std::vector<Order<Side::BUY>> mOpenBuyOrders;
  std::vector<Order<Side::SELL>> mOpenSellOrders;
  std::vector<Order<Side::BUY>> mFilledBuyOrders;
  std::vector<Order<Side::SELL>> mFilledSellOrders;
};
} // namespace Common

#endif
