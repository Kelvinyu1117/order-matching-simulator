#ifndef ORDER
#define ORDER
#include <common.h>

namespace Common {
template <Side side> class Order;

template <Side side>
bool operator==(const Order<side> &a, const Order<side> &b) {
  return a.getSide() == b.getSide() && a.getTraderId() == b.getTraderId() &&
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
  Order(const ClientId &clientId, OrderId orderId, const Symbol &symbol,
        Price price, Quantity quantity)
      : mSide(side), mTraderId(clientId), mSymbol(symbol), mPrice(price),
        mQuantity(quantity), mOrderId(orderId) {}
  Order(const Order<side> &other) = default;
  Order<side> &operator=(const Order<side> &) = default;
  Order(Order<side> &&other) = default;
  Order<side> &operator=(Order<side> &&other) = default;

  const ClientId &getTraderId() const { return mTraderId; }
  Side getSide() const { return mSide; }
  OrderId getOrderId() const { return mOrderId; }
  const Symbol &getSymbol() const { return mSymbol; }
  Price getPrice() const { return mPrice; }
  Quantity getQuantity() const { return mQuantity; }
  void setQuantity(Quantity quantity) { mQuantity = quantity; }
  friend bool operator==<side>(const Order<side> &a, const Order<side> &b);

  friend bool operator!=<side>(const Order<side> &a, const Order<side> &b);

private:
  Side mSide;
  ClientId mTraderId;
  OrderId mOrderId;
  Symbol mSymbol;
  Price mPrice;
  Quantity mQuantity;
};

} // namespace Common

#endif
