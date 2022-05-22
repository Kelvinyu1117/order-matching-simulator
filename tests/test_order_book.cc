#include "gtest/gtest.h"
#include <core/order/order.h>
#include <core/order_book/order_book.h>
#include <types.h>

using namespace Common;
using namespace Core;

class OrderBookTest : public ::testing::Test {
protected:
  void SetUp() override {
    mOrderbook.clear<Side::BUY>();
    mOrderbook.clear<Side::SELL>();
  }

  void TearDown() override {}

  TraderId mTrader1Id = "Trader1";
  TraderId mTrader2Id = "Trader2";
  TraderId mTrader3Id = "Trader3";
  OrderBook mOrderbook;
};

TEST_F(OrderBookTest, TestBidOrderInsertionMultiplePriceLevel) {

  Order<Side::BUY> buyOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 100,
                             10);
  Order<Side::BUY> buyOrder2(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 101,
                             10);
  Order<Side::BUY> buyOrder3(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 102,
                             10);

  mOrderbook.insert<Side::BUY>(buyOrder1);

  mOrderbook.insert<Side::BUY>(buyOrder2);

  mOrderbook.insert<Side::BUY>(buyOrder3);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::BUY>(), 3);
  auto best_bid_iter = mOrderbook.begin<Side::BUY>();
  EXPECT_EQ(best_bid_iter->first, 102);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder3);

  best_bid_iter++;

  EXPECT_EQ(best_bid_iter->first, 101);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder2);

  best_bid_iter++;

  EXPECT_EQ(best_bid_iter->first, 100);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder1);
}

TEST_F(OrderBookTest, TestSameTraderBidOrderInsertionSamePriceLevel) {

  Order<Side::BUY> buyOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 100,
                             10);
  Order<Side::BUY> buyOrder2(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 100,
                             5);
  Order<Side::BUY> buyOrder3(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 100,
                             2);

  mOrderbook.insert<Side::BUY>(buyOrder1);

  mOrderbook.insert<Side::BUY>(buyOrder2);

  mOrderbook.insert<Side::BUY>(buyOrder3);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::BUY>(), 1);

  auto best_bid_iter = mOrderbook.begin<Side::BUY>();
  EXPECT_EQ(best_bid_iter->first, 100);
  EXPECT_EQ(best_bid_iter->second.numOfOrders(), 1);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder3);
}

TEST_F(OrderBookTest, TestDifferentTraderBidOrderInsertionSamePriceLevel) {

  Order<Side::BUY> buyOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 100,
                             10);
  Order<Side::BUY> buyOrder2(OrderStyle::LIMIT_ORDER, mTrader2Id, 0, "ABC", 100,
                             5);
  Order<Side::BUY> buyOrder3(OrderStyle::LIMIT_ORDER, mTrader3Id, 0, "ABC", 100,
                             2);

  mOrderbook.insert<Side::BUY>(buyOrder1);

  mOrderbook.insert<Side::BUY>(buyOrder2);

  mOrderbook.insert<Side::BUY>(buyOrder3);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::BUY>(), 1);

  auto best_bid_iter = mOrderbook.begin<Side::BUY>();
  EXPECT_EQ(best_bid_iter->first, 100);
  EXPECT_EQ(best_bid_iter->second.numOfOrders(), 3);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder1);

  best_bid_iter->second.pop();

  EXPECT_EQ(best_bid_iter->second.numOfOrders(), 2);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder2);

  best_bid_iter->second.pop();

  EXPECT_EQ(best_bid_iter->second.numOfOrders(), 1);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder3);

  best_bid_iter->second.pop();

  EXPECT_EQ(best_bid_iter->second.empty(), true);
}

TEST_F(OrderBookTest, TestAskOrderInsertionMultiplePriceLevel) {
  Order<Side::SELL> sellOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               100, 10);
  Order<Side::SELL> sellOrder2(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               101, 10);
  Order<Side::SELL> sellOrder3(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               102, 10);

  mOrderbook.insert<Side::SELL>(sellOrder1);

  mOrderbook.insert<Side::SELL>(sellOrder2);

  mOrderbook.insert<Side::SELL>(sellOrder3);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::SELL>(), 3);

  auto best_ask_iter = mOrderbook.begin<Side::SELL>();
  EXPECT_EQ(best_ask_iter->first, 100);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder1);

  best_ask_iter++;

  EXPECT_EQ(best_ask_iter->first, 101);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder2);

  best_ask_iter++;

  EXPECT_EQ(best_ask_iter->first, 102);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder3);
}

TEST_F(OrderBookTest, TestSameTraderAskOrderInsertionSamePriceLevel) {
  Order<Side::SELL> sellOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               100, 10);
  Order<Side::SELL> sellOrder2(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               100, 5);
  Order<Side::SELL> sellOrder3(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               100, 1);

  mOrderbook.insert<Side::SELL>(sellOrder1);

  mOrderbook.insert<Side::SELL>(sellOrder2);

  mOrderbook.insert<Side::SELL>(sellOrder3);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::SELL>(), 1);

  auto best_ask_iter = mOrderbook.begin<Side::SELL>();
  EXPECT_EQ(best_ask_iter->first, 100);
  EXPECT_EQ(best_ask_iter->second.numOfOrders(), 1);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder3);
}

TEST_F(OrderBookTest, TestDifferentTraderAskOrderInsertionSamePriceLevel) {
  Order<Side::SELL> sellOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               100, 10);
  Order<Side::SELL> sellOrder2(OrderStyle::LIMIT_ORDER, mTrader2Id, 0, "ABC",
                               100, 5);
  Order<Side::SELL> sellOrder3(OrderStyle::LIMIT_ORDER, mTrader3Id, 0, "ABC",
                               100, 1);

  mOrderbook.insert<Side::SELL>(sellOrder1);

  mOrderbook.insert<Side::SELL>(sellOrder2);

  mOrderbook.insert<Side::SELL>(sellOrder3);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::SELL>(), 1);

  auto best_ask_iter = mOrderbook.begin<Side::SELL>();

  EXPECT_EQ(best_ask_iter->first, 100);
  EXPECT_EQ(best_ask_iter->second.numOfOrders(), 3);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder1);

  best_ask_iter->second.pop();

  EXPECT_EQ(best_ask_iter->second.numOfOrders(), 2);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder2);

  best_ask_iter->second.pop();

  EXPECT_EQ(best_ask_iter->second.numOfOrders(), 1);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder3);

  best_ask_iter->second.pop();

  EXPECT_EQ(best_ask_iter->second.empty(), true);
}

TEST_F(OrderBookTest, TestBidOrderLevelRemoval) {
  Order<Side::BUY> buyOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 100,
                             10);
  mOrderbook.insert<Side::BUY>(buyOrder1);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::BUY>(), 1);
  auto best_bid_iter = mOrderbook.begin<Side::BUY>();
  EXPECT_EQ(best_bid_iter->first, 100);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder1);

  best_bid_iter = mOrderbook.erase(best_bid_iter);
  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(best_bid_iter, mOrderbook.end<Side::BUY>());
}

TEST_F(OrderBookTest, TestAskOrderLevelRemoval) {
  Order<Side::SELL> sellOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               100, 10);
  mOrderbook.insert<Side::SELL>(sellOrder1);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::SELL>(), 1);
  auto best_ask_iter = mOrderbook.begin<Side::SELL>();
  EXPECT_EQ(best_ask_iter->first, 100);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder1);

  best_ask_iter = mOrderbook.erase(best_ask_iter);
  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(best_ask_iter, mOrderbook.end<Side::SELL>());
}

TEST_F(OrderBookTest, TestBidOrderRemoval) {
  Order<Side::BUY> buyOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 100,
                             10);
  Order<Side::BUY> buyOrder2(OrderStyle::LIMIT_ORDER, mTrader2Id, 1, "ABC", 100,
                             50);
  Order<Side::BUY> buyOrder3(OrderStyle::LIMIT_ORDER, mTrader3Id, 2, "ABC", 100,
                             60);

  mOrderbook.insert<Side::BUY>(buyOrder1);
  mOrderbook.insert<Side::BUY>(buyOrder2);
  mOrderbook.insert<Side::BUY>(buyOrder3);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::BUY>(), 1);
  auto best_bid_iter = mOrderbook.begin<Side::BUY>();
  EXPECT_EQ(best_bid_iter->first, 100);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder1);
  EXPECT_EQ(best_bid_iter->second.totalQuantity(), 120);

  bool success = mOrderbook.removeOrder(1, mTrader2Id);
  EXPECT_EQ(success, true);
  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(best_bid_iter->second.front(), buyOrder1);
  EXPECT_EQ(best_bid_iter->second.totalQuantity(), 70);
}

TEST_F(OrderBookTest, TestAskOrderRemoval) {
  Order<Side::SELL> sellOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               100, 10);
  Order<Side::SELL> sellOrder2(OrderStyle::LIMIT_ORDER, mTrader2Id, 1, "ABC",
                               100, 50);
  Order<Side::SELL> sellOrder3(OrderStyle::LIMIT_ORDER, mTrader3Id, 2, "ABC",
                               100, 60);

  mOrderbook.insert<Side::SELL>(sellOrder1);
  mOrderbook.insert<Side::SELL>(sellOrder2);
  mOrderbook.insert<Side::SELL>(sellOrder3);

  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::SELL>(), 1);
  auto best_ask_iter = mOrderbook.begin<Side::SELL>();
  EXPECT_EQ(best_ask_iter->first, 100);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder1);
  EXPECT_EQ(best_ask_iter->second.totalQuantity(), 120);

  bool success = mOrderbook.removeOrder(0, mTrader1Id);
  EXPECT_EQ(success, true);
  EXPECT_EQ(mOrderbook.getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(best_ask_iter->second.front(), sellOrder2);
  EXPECT_EQ(best_ask_iter->second.totalQuantity(), 110);
}

TEST_F(OrderBookTest, TestBidOrderSearch) {
  Order<Side::BUY> buyOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 100,
                             10);
  Order<Side::BUY> buyOrder2(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 101,
                             10);
  Order<Side::BUY> buyOrder3(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 102,
                             10);
  Order<Side::BUY> buyOrder4(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC", 103,
                             10);

  mOrderbook.insert<Side::BUY>(buyOrder1);

  mOrderbook.insert<Side::BUY>(buyOrder2);

  mOrderbook.insert<Side::BUY>(buyOrder3);

  mOrderbook.insert<Side::BUY>(buyOrder4);

  {
    auto bookLevel = mOrderbook.search<Side::BUY>(101);

    EXPECT_NE(bookLevel, mOrderbook.end<Side::BUY>());
    EXPECT_EQ(bookLevel->second.front(), buyOrder2);
  }

  {
    auto bookLevel = mOrderbook.search<Side::BUY>(200);
    EXPECT_EQ(bookLevel, mOrderbook.begin<Side::BUY>());
  }

  {
    auto bookLevel = mOrderbook.search<Side::BUY>(50);
    EXPECT_EQ(bookLevel, mOrderbook.end<Side::BUY>());
  }
}

TEST_F(OrderBookTest, TestAskOrderSearch) {
  Order<Side::SELL> sellOrder1(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               100, 10);
  Order<Side::SELL> sellOrder2(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               101, 10);
  Order<Side::SELL> sellOrder3(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               102, 10);
  Order<Side::SELL> sellOrder4(OrderStyle::LIMIT_ORDER, mTrader1Id, 0, "ABC",
                               103, 10);

  mOrderbook.insert<Side::SELL>(sellOrder1);

  mOrderbook.insert<Side::SELL>(sellOrder2);

  mOrderbook.insert<Side::SELL>(sellOrder3);

  mOrderbook.insert<Side::SELL>(sellOrder4);

  {
    auto bookLevel = mOrderbook.search<Side::SELL>(101);

    EXPECT_NE(bookLevel, mOrderbook.end<Side::SELL>());
    EXPECT_EQ(bookLevel->second.front(), sellOrder2);
  }

  {
    auto bookLevel = mOrderbook.search<Side::SELL>(200);
    EXPECT_EQ(bookLevel, mOrderbook.end<Side::SELL>());
  }

  {
    auto bookLevel = mOrderbook.search<Side::SELL>(50);
    EXPECT_EQ(bookLevel, mOrderbook.begin<Side::SELL>());
  }
}
