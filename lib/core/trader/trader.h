#ifndef COMMON_TRADER
#define COMMON_TRADER
#include "types.h"
#include <iostream>
#include <list>
#include <order/order.h>
#include <string>
#include <unordered_map>
#include <vector>

using namespace Common;

namespace Core {
template <Side side> class Order;
class Trader {
public:
  Trader() = default;
  Trader(std::string traderId) : mTraderId(traderId){};
  Trader(const Trader &other) = default;
  Trader &operator=(const Trader &) = default;
  Trader(Trader &&other) = default;
  Trader &operator=(Trader &&other) = default;

  void notifyAllFilled(OrderId orderId) {
    std::cout << mTraderId << " orderid:" << orderId << " "
              << "is successfully filled\n";
  }

  template <Side side, OrderStyle style>
  void notifyFill(OrderId orderId, Symbol symbol, Price fillPrice,
                  Quantity fillQuantity) {

    if constexpr (side == Side::BUY) {
      std::cout << "Fill! " << mTraderId << " "
                << "ORDER_TYPE: " << orderStyle2Str(style) << " BUY "
                << fillQuantity << " " << symbol << " at " << fillPrice << '\n';

      mFilledBuyOrders.emplace_back(Order<side>(
          style, mTraderId, orderId, symbol, fillPrice, fillQuantity));
    } else {
      std::cout << "Fill! " << mTraderId << " "
                << "ORDER_TYPE: " << orderStyle2Str(style) << " SELL "
                << fillQuantity << " " << symbol << " at " << fillPrice << '\n';

      mFilledSellOrders.emplace_back(Order<side>(
          style, mTraderId, orderId, symbol, fillPrice, fillQuantity));
    }
  }

  void notifyCancel(OrderId orderId,
                    OrderCancelReason rsn = OrderCancelReason::CANCEL_REQUEST) {
    // precondition: the order must be in the open order lists
    auto it = mOpenBuyOrders.begin();
    auto end = mOpenBuyOrders.end();
    while (it != end && it->getOrderId() != orderId) {
      it++;
    }

    if (it != end) {
      std::cout << "Order Cancel! " << mTraderId << " CANCEL "
                << "ORDER_TYPE: " << orderStyle2Str(it->getOrderStyle())
                << " BUY " << it->getQuantity() << " " << it->getSymbol()
                << " at " << it->getPrice()
                << " reason: " << orderCancelReason2Str(rsn) << '\n';
      mOpenBuyOrders.erase(it);
    } else {
      auto it = mOpenSellOrders.begin();
      auto end = mOpenSellOrders.end();
      while (it != end && it->getOrderId() != orderId) {
        it++;
      }

      std::cout << "Order Cancel! " << mTraderId << " CANCEL "
                << "ORDER_TYPE: " << orderStyle2Str(it->getOrderStyle())
                << " SELL " << it->getQuantity() << " " << it->getSymbol()
                << " at " << it->getPrice()
                << " reason: " << orderCancelReason2Str(rsn) << '\n';

      mOpenSellOrders.erase(it);
    }
  }

  template <Side side, OrderStyle style>
  void notifyCancel(OrderId orderId, Symbol symbol, Price price,
                    Quantity quantity,
                    OrderCancelReason rsn = OrderCancelReason::CANCEL_REQUEST) {

    if constexpr (side == Side::BUY) {

      std::cout << "Order Cancel! " << mTraderId << " CANCEL "
                << "ORDER_TYPE: " << orderStyle2Str(style) << " BUY "
                << quantity << " " << symbol << " at " << price
                << " reason: " << orderCancelReason2Str(rsn) << '\n';

      if constexpr (style == OrderStyle::LIMIT_ORDER) {
        auto it = mOpenBuyOrders.begin();
        auto end = mOpenBuyOrders.end();
        while (it != end && it->getOrderId() != orderId) {
          it++;
        }

        if (it != end) {

          mOpenBuyOrders.erase(it);
        }
      }

    } else {
      std::cout << "Order Cancel! " << mTraderId << " CANCEL "
                << "SELL " << quantity << " " << symbol << " at " << price
                << " reason: " << orderCancelReason2Str(rsn) << '\n';
      if constexpr (style == OrderStyle::LIMIT_ORDER) {
        auto it = mOpenSellOrders.begin();
        auto end = mOpenSellOrders.end();
        while (it != end && it->getOrderId() != orderId) {
          it++;
        }

        if (it != end) {
          mOpenSellOrders.erase(it);
        }
      }
    }
  }

  void notifyCancelReject(OrderId orderId) {

    std::cout << "Order Cancel Reject! " << mTraderId << " CANCEL "
              << "order: " << orderId << " failed" << '\n';
  }

  template <Side side, OrderStyle style>
  void notifyOpen(OrderId orderId, Symbol symbol, Price fillPrice,
                  Quantity fillQuantity) {

    if constexpr (side == Side::BUY) {
      std::cout << "Order Open! " << mTraderId << " "
                << "BUY " << fillQuantity << " " << symbol << " at "
                << fillPrice << '\n';

      mOpenBuyOrders.emplace_back(Order<side>(style, mTraderId, orderId, symbol,
                                              fillPrice, fillQuantity));
    } else {
      std::cout << "Open! " << mTraderId << " "
                << "SELL " << fillQuantity << " " << symbol << " at "
                << fillPrice << '\n';

      mOpenSellOrders.emplace_back(Order<side>(
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
  std::list<Order<Side::BUY>> mOpenBuyOrders;
  std::list<Order<Side::SELL>> mOpenSellOrders;
  std::vector<Order<Side::BUY>> mFilledBuyOrders;
  std::vector<Order<Side::SELL>> mFilledSellOrders;
};
} // namespace Core

#endif
