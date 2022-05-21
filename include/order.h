#ifndef COMMON_ORDER
#define COMMON_ORDER
#include <types.h>

namespace Common {

enum class OrderStyle {
  MKT_ORDER,
  LIMIT_ORDER,
};

enum class Side { BUY, SELL };

struct OrderCancelRequest {};

template <Side side> class Order;

template <Side side>
bool operator==(const Order<side> &a, const Order<side> &b) {
  return a.getSide() == b.getSide() && a.getOrderStyle() == b.getOrderStyle() &&
         a.getTraderId() == b.getTraderId() &&
         a.getOrderId() == b.getOrderId() && a.getSymbol() == b.getSymbol() &&
         a.getPrice() == b.getPrice() && a.getQuantity() == b.getQuantity();
}
template <Side side>
bool operator!=(const Order<side> &a, const Order<side> &b) {
  return !(a == b);
}

template <Side side> class Order {
public:
  Order() = default;
  Order(OrderStyle style, const TraderId &TraderId, OrderId orderId,
        const Symbol &symbol, Price price, Quantity quantity)
      : mSide(side), mStyle(style), mTraderId(TraderId), mSymbol(symbol),
        mPrice(price), mQuantity(quantity), mOrderId(orderId) {}
  Order(const Order<side> &other) = default;
  Order<side> &operator=(const Order<side> &) = default;
  Order(Order<side> &&other) = default;
  Order<side> &operator=(Order<side> &&other) = default;

  const TraderId &getTraderId() const { return mTraderId; }
  Side getSide() const { return mSide; }
  OrderStyle getOrderStyle() const { return mStyle; }

  OrderId getOrderId() const { return mOrderId; }
  const Symbol &getSymbol() const { return mSymbol; }
  Price getPrice() const { return mPrice; }
  Quantity getQuantity() const { return mQuantity; }
  void setQuantity(Quantity quantity) { mQuantity = quantity; }
  friend bool operator==<side>(const Order<side> &a, const Order<side> &b);

  friend bool operator!=<side>(const Order<side> &a, const Order<side> &b);

private:
  Side mSide;
  OrderStyle mStyle;
  TraderId mTraderId;
  OrderId mOrderId;
  Symbol mSymbol;
  Price mPrice;
  Quantity mQuantity;
};

} // namespace Common

#endif
