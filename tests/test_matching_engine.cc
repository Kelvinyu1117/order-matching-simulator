#include "gtest/gtest.h"
#include <core/execution_context/execution_context.h>
#include <core/order_book/order_book.h>
#include <matching_engine/matching_engine.h>
#include <memory>
#include <types.h>
#include <utility>

using namespace Common;
using namespace Core;

class MatchingEngineTest : public ::testing::Test {
protected:
  void SetUp() override {
    mConfig = std::make_shared<MatchingEngineConfig>();
    mConfig->selfTradPreventionConfig = SelfTradePreventionConfig();

    mMatchingEngine.addConfig(mConfig);
    mMatchingEngine.addStocks(mSymbols);
    mExecutionContext.addTraders(mTraderIds);
  }

  void TearDown() override {}

  std::vector<Symbol> mSymbols = {"ABC", "S", "G", "H"};
  std::vector<TraderId> mTraderIds = {"TraderA", "TraderB", "TraderC",
                                      "TraderD", "TraderE", "TraderW",
                                      "TraderX", "TraderY", "TraderZ"};
  std::shared_ptr<MatchingEngineConfig> mConfig;
  MatchingEngine mMatchingEngine;
  ExecutionContext mExecutionContext;
};

TEST_F(MatchingEngineTest, BasicInsertion) {
  auto sym = mSymbols[0];
  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderA", sym, 10, 10);
  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderB", sym, 9, 10);
  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderC", sym, 8, 10);
  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderD", sym, 7, 10);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderA", sym, 20, 10);
  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderB", sym, 19, 10);
  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderC", sym, 18, 10);
  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderD", sym, 17, 10);
  auto mp = mMatchingEngine.getOrderBookMap();
  auto &book = mp[sym];

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 4);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 4);
}

/**
 * @brief
 * Trader A places a buy order of 200 on stock S,
 * the order is stored in the order book as open.
 * Trader B places a sell order of 200 on stock S.
 * Notify both the traders with success message.
 */
TEST_F(MatchingEngineTest, MatchingTest1) {
  auto sym = mSymbols[1];
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();
  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderA", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderB", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);

  {
    auto &filledBuyOrders = traderMap["TraderA"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderA"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);
  }

  {
    auto &filledBuyOrders = traderMap["TraderB"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderB"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);
  }
}

/**
 * @brief
 * Trader C places a sell order of 300 on stock G.
 * Trader D places a buy order of 200 on stock G.
 * Notify the Trader D with success message.
 * Trader E places a BUY Order of 200 on stock G.
 * Notify the Trader C with success message.
 */
TEST_F(MatchingEngineTest, MatchingTest2) {
  auto sym = "G";
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderC", sym, 10, 300);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderD", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->first, 10);
  EXPECT_EQ(book->begin<Side::SELL>()->second.front().getQuantity(), 100);

  {
    auto &filledBuyOrders = traderMap["TraderD"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderD"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);
  }

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderE", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.front().getQuantity(), 100);

  {
    auto &filledBuyOrders = traderMap["TraderC"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderC"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 2);
    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);
    EXPECT_EQ(filledSellOrders[1].getQuantity(), 100);
  }
}

/**
 * @brief
 * Trader W, X and Y place sell order of 200 on stock H each.
 * Trade Z place a buy order of 600 on stock H.
 * Trader W, X, Y and Z should be notified of success.
 */

TEST_F(MatchingEngineTest, MatchingTest3) {
  auto sym = "H";
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderX", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 2);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 3);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderZ", sym, 10, 600);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);

  {
    auto &filledBuyOrders = traderMap["TraderW"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderW"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);
  }

  {
    auto &filledBuyOrders = traderMap["TraderX"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderX"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);
  }

  {
    auto &filledBuyOrders = traderMap["TraderY"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderY"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);
  }

  {
    auto &filledBuyOrders = traderMap["TraderZ"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderZ"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 3);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);
    EXPECT_EQ(filledBuyOrders[1].getQuantity(), 200);
    EXPECT_EQ(filledBuyOrders[2].getQuantity(), 200);
  }
}

/**
 * @brief
 * Trader W, X and Y place sell order of 200 on stock H at 10, 20, 30
 * respectively. Trade Z place a buy order of 600 on stock H at 40.
 * Trader W, X,Y and Z should be notified of success.
 */

TEST_F(MatchingEngineTest, MatchingTest4) {
  auto sym = "H";
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderX", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 2);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 30, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 3);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderZ", sym, 40, 600);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);

  {
    auto &filledBuyOrders = traderMap["TraderW"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderW"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);
  }

  {
    auto &filledBuyOrders = traderMap["TraderX"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderX"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);
  }

  {
    auto &filledBuyOrders = traderMap["TraderY"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderY"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);
  }

  {
    auto &filledBuyOrders = traderMap["TraderZ"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderZ"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 3);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 10);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);

    EXPECT_EQ(filledBuyOrders[1].getPrice(), 20);
    EXPECT_EQ(filledBuyOrders[1].getQuantity(), 200);

    EXPECT_EQ(filledBuyOrders[2].getPrice(), 30);
    EXPECT_EQ(filledBuyOrders[2].getQuantity(), 200);
  }
}

/**
 * @brief
 * Trader W, X and Y place BUY order of 200 on stock H each.
 * Trade Z place a SELL order of 600 on stock H.
 * Trader W, X, Y and Z should be notified of success.
 */

TEST_F(MatchingEngineTest, MatchingTest5) {
  auto sym = "H";
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderX", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 2);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 3);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderZ", sym, 10, 600);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);

  {
    auto &filledBuyOrders = traderMap["TraderW"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderW"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);
  }

  {
    auto &filledBuyOrders = traderMap["TraderX"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderX"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);
  }

  {
    auto &filledBuyOrders = traderMap["TraderY"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderY"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);
  }

  {
    auto &filledBuyOrders = traderMap["TraderZ"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderZ"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 3);

    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);
    EXPECT_EQ(filledSellOrders[1].getQuantity(), 200);
    EXPECT_EQ(filledSellOrders[2].getQuantity(), 200);
  }
}

/**
 * @brief
 * Trader W, X and Y place BUY order of 200 on stock H at 10, 20, 30
 * respectively. Trade Z place a SELL order of 600 on stock H at 5.
 * Trader W, X,Y and Z should be notified of success.
 */

TEST_F(MatchingEngineTest, MatchingTest6) {
  auto sym = "H";
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderX", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 2);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 30, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 3);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderZ", sym, 5, 600);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);

  {
    auto &filledBuyOrders = traderMap["TraderW"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderW"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 5);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderX"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderX"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 5);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderY"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderY"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 5);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderZ"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderZ"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 3);

    EXPECT_EQ(filledSellOrders[0].getPrice(), 5);
    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);

    EXPECT_EQ(filledSellOrders[1].getPrice(), 5);
    EXPECT_EQ(filledSellOrders[1].getQuantity(), 200);

    EXPECT_EQ(filledSellOrders[2].getPrice(), 5);
    EXPECT_EQ(filledSellOrders[2].getQuantity(), 200);
  }
}

/**
 * @brief
 * Trader W, X and Y place BUY order of 200 on stock H at 10, 20, 30
 * respectively. Trade Z place a SELL order of 600 on stock H at 15.
 * Trader X, Y, should be notified of success.
 */

TEST_F(MatchingEngineTest, MatchingTest7) {
  auto sym = "H";
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderX", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 2);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 30, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 3);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderZ", sym, 15, 600);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->first, 15);
  EXPECT_EQ(book->begin<Side::SELL>()->second.front().getQuantity(), 200);

  {
    auto &filledBuyOrders = traderMap["TraderW"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderW"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 0);
  }

  {
    auto &filledBuyOrders = traderMap["TraderX"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderX"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 15);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderY"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderY"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 15);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderZ"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderZ"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 2);

    EXPECT_EQ(filledSellOrders[0].getPrice(), 15);
    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);

    EXPECT_EQ(filledSellOrders[1].getPrice(), 15);
    EXPECT_EQ(filledSellOrders[1].getQuantity(), 200);
  }
}

/**
 * @brief
 * Trader W, X and Y place SELL order of 200 on stock H at 10, 20, 30
 * respectively. Trade Z place a BUY order of 600 on stock H at 25.
 * Trader W, X, should be notified of success.
 */
TEST_F(MatchingEngineTest, MatchingTest8) {
  auto sym = "H";
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderX", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 2);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 30, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 3);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderZ", sym, 25, 600);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::BUY>()->first, 25);
  EXPECT_EQ(book->begin<Side::BUY>()->second.front().getQuantity(), 200);

  {
    auto &filledBuyOrders = traderMap["TraderW"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderW"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);

    EXPECT_EQ(filledSellOrders[0].getPrice(), 10);
    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderX"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderX"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);

    EXPECT_EQ(filledSellOrders[0].getPrice(), 20);
    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderY"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderY"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 0);
  }

  {
    auto &filledBuyOrders = traderMap["TraderZ"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderZ"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 2);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 10);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);

    EXPECT_EQ(filledBuyOrders[1].getPrice(), 20);
    EXPECT_EQ(filledBuyOrders[1].getQuantity(), 200);
  }
}

/**
 * @brief
 * Trader W, X and Y place SELL order of 200 on stock H at 10, 20, 30
 * respectively. Trade Z place a Market BUY order of 600 on stock H.
 * Trader W, X, Y should be notified of success.
 */
TEST_F(MatchingEngineTest, MatchingTest9) {
  auto sym = "H";
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderX", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 2);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 30, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 3);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::MKT_ORDER>(mExecutionContext,
                                                           "TraderZ", sym, 600);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);

  {
    auto &filledBuyOrders = traderMap["TraderW"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderW"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);

    EXPECT_EQ(filledSellOrders[0].getPrice(), 10);
    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderX"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderX"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);

    EXPECT_EQ(filledSellOrders[0].getPrice(), 20);
    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderY"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderY"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 1);

    EXPECT_EQ(filledSellOrders[0].getPrice(), 30);
    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderZ"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderZ"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 3);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 10);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);

    EXPECT_EQ(filledBuyOrders[1].getPrice(), 20);
    EXPECT_EQ(filledBuyOrders[1].getQuantity(), 200);

    EXPECT_EQ(filledBuyOrders[2].getPrice(), 30);
    EXPECT_EQ(filledBuyOrders[2].getQuantity(), 200);
  }
}

/**
 * @brief
 * Trader W, X and Y place BUY order of 200 on stock H at 10, 20, 30
 * respectively. Trade Z place a Market SELL order of 400 on stock H.
 * Trader X, Y should be notified of success.
 */
TEST_F(MatchingEngineTest, MatchingTest10) {
  auto sym = "H";
  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderX", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 2);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 30, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 3);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::MKT_ORDER>(
      mExecutionContext, "TraderZ", sym, 400);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);

  {
    auto &filledBuyOrders = traderMap["TraderW"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderW"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 0);
  }

  {
    auto &filledBuyOrders = traderMap["TraderX"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderX"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 20);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderY"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderY"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 1);
    EXPECT_EQ(filledSellOrders.size(), 0);

    EXPECT_EQ(filledBuyOrders[0].getPrice(), 30);
    EXPECT_EQ(filledBuyOrders[0].getQuantity(), 200);
  }

  {
    auto &filledBuyOrders = traderMap["TraderZ"]->getFilledBuyOrders();
    auto &filledSellOrders = traderMap["TraderZ"]->getFilledSellOrders();
    EXPECT_EQ(filledBuyOrders.size(), 0);
    EXPECT_EQ(filledSellOrders.size(), 2);

    EXPECT_EQ(filledSellOrders[0].getPrice(), 30);
    EXPECT_EQ(filledSellOrders[0].getQuantity(), 200);

    EXPECT_EQ(filledSellOrders[1].getPrice(), 20);
    EXPECT_EQ(filledSellOrders[1].getQuantity(), 200);
  }
}

/**
 * @brief
 * Trader W place BUY order of 200 on stock H at 10,
 * Trader W place Market SELL Order of 200 on stock H,
 * given the self-trade prevention is enabled, policy: CANCEL_ACTIVE
 * the market order will be cancelled and the buy order will still in the queue
 */
TEST_F(MatchingEngineTest, MatchingTest11) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = true;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_ACTIVE;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::MKT_ORDER>(
      mExecutionContext, "TraderW", sym, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);
}

/**
 * @brief
 * Trader W place SELL order of 200 on stock H at 10,
 * Trader W place Market BUY Order of 200 on stock H,
 * given the self-trade prevention is enabled, policy: CANCEL_ACTIVE
 * the market order will be cancelled and the buy order will still in the queue
 */
TEST_F(MatchingEngineTest, MatchingTest12) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = true;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_ACTIVE;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::MKT_ORDER>(mExecutionContext,
                                                           "TraderW", sym, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);
}

/**
 * @brief
 * Trader W place SELL order of 200 on stock H at 10,
 * Trader W place Market BUY Order of 200 on stock H,
 * given the self-trade prevention is enabled, policy: CANCEL_BOTH
 * Both orders will be cancelled
 */
TEST_F(MatchingEngineTest, MatchingTest13) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = true;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_BOTH;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::MKT_ORDER>(mExecutionContext,
                                                           "TraderW", sym, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
}

/**
 * @brief
 * Trader W, Y place SELL order of 200 on stock H at 10,
 * Trader W place Market BUY Order of 200 on stock H,
 * given the self-trade prevention is enabled, policy: CANCEL_PASSIVE
 * W's SELL order will be cancelled in the queue, Y will be notified for order
 * fill, W's Market BUY order will be filled as well
 */
TEST_F(MatchingEngineTest, MatchingTest14) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = true;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_PASSIVE;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 2);

  mMatchingEngine.insert<Side::BUY, OrderStyle::MKT_ORDER>(mExecutionContext,
                                                           "TraderW", sym, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
}

/**
 * @brief
 * Trader W, place SELL order of 200 on stock H at 10,
 * Trader W place Market BUY Order of 200 on stock H,
 * given the self-trade prevention is enabled, policy: CANCEL_PASSIVE
 * W's SELL order will be cancelled in the queue, W's Market BUY order will be
 * cancelled as well because there is no price level can match
 */
TEST_F(MatchingEngineTest, MatchingTest15) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = true;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_PASSIVE;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::MKT_ORDER>(mExecutionContext,
                                                           "TraderW", sym, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
}

/**
 * @brief
 * Trader W place Market BUY Order of 200 on stock H,
 * W's Market BUY order will be cancelled because there is no price level can
 * match
 */
TEST_F(MatchingEngineTest, MatchingTest16) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = false;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_PASSIVE;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::BUY, OrderStyle::MKT_ORDER>(mExecutionContext,
                                                           "TraderW", sym, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
}

/**
 * @brief
 * Trader W place Market SELL Order of 200 on stock H,
 * W's Market SELL order will be cancelled because there is no price level can
 * match
 */
TEST_F(MatchingEngineTest, MatchingTest17) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = false;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_PASSIVE;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::MKT_ORDER>(
      mExecutionContext, "TraderW", sym, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
}

/**
 * @brief
 * Trader W place SELL order of 200 on stock H at 10,
 * Trader W place LIMIT BUY Order of 200 on stock H,
 * given the self-trade prevention is enabled, policy: CANCEL_ACTIVE
 * the market order will be cancelled and the buy order will still in the queue
 */
TEST_F(MatchingEngineTest, MatchingTest18) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = true;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_ACTIVE;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);
}

/**
 * @brief
 * Trader W place SELL order of 200 on stock H at 10,
 * Trader W place LIMIT BUY Order of 200 on stock H,
 * given the self-trade prevention is enabled, policy: CANCEL_BOTH
 * Both orders will be cancelled
 */
TEST_F(MatchingEngineTest, MatchingTest19) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = true;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_BOTH;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
}

/**
 * @brief
 * Trader W place SELL order of 200 on stock H at 10,
 * Trader W place LIMIT BUY Order of 200 on stock H,
 * given the self-trade prevention is enabled, policy: CANCEL_PASSIVE
 * W's SELL order will be cancelled in the queue, the BUY order will be in the
 * queue
 */
TEST_F(MatchingEngineTest, MatchingTest20) {
  auto sym = "H";
  mConfig->selfTradPreventionConfig->enable = true;
  mConfig->selfTradPreventionConfig->policy =
      SelfTradePreventionPolicy::CANCEL_PASSIVE;

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);

  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);
}

/**
 * @brief
 * Trader W, Y place SELL order of 200 on stock H at 10, 20
 * Trader W cancel the order
 * W should be notified the order has been cancelled
 */
TEST_F(MatchingEngineTest, MatchingTest21) {
  auto sym = "H";

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  auto id = mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 2);

  OrderCancelRequest req;
  req.mOrderId = id;
  req.mSymbol = sym;
  req.mTraderId = "TraderW";

  mMatchingEngine.cancel(mExecutionContext, req);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);
}

/**
 * @brief
 * Trader W, Y place BUY order of 200 on stock H at 10, 20
 * Trader W cancel the order
 * W should be notified the order has been cancelled
 */
TEST_F(MatchingEngineTest, MatchingTest22) {
  auto sym = "H";

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  auto id = mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 2);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);

  OrderCancelRequest req;
  req.mOrderId = id;
  req.mSymbol = sym;
  req.mTraderId = "TraderW";

  mMatchingEngine.cancel(mExecutionContext, req);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);
}

/**
 * @brief
 * Trader W, Y place SELL order of 200 on stock H at 10, 10
 * Trader W cancel the order
 * W should be notified the order has been cancelled
 * Y's order should in the queue
 */
TEST_F(MatchingEngineTest, MatchingTest23) {
  auto sym = "H";

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  auto id = mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 2);

  OrderCancelRequest req;
  req.mOrderId = id;
  req.mSymbol = sym;
  req.mTraderId = "TraderW";

  mMatchingEngine.cancel(mExecutionContext, req);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.front().getTraderId(), "TraderY");
}

/**
 * @brief
 * Trader W, Y place BUY order of 200 on stock H at 10, 10
 * Trader W cancel the order
 * W should be notified the order has been cancelled
 * Y's order should in the queue
 */
TEST_F(MatchingEngineTest, MatchingTest24) {
  auto sym = "H";

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  auto id = mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 2);

  OrderCancelRequest req;
  req.mOrderId = id;
  req.mSymbol = sym;
  req.mTraderId = "TraderW";

  mMatchingEngine.cancel(mExecutionContext, req);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);
  EXPECT_EQ(book->begin<Side::BUY>()->second.front().getTraderId(), "TraderY");
}

/**
 * @brief
 * Trader W, Y place SELL order of 200 on stock H at 10, 20
 * Trader W cancel the order
 * W should be notified the order has been cancelled
 * Y's order should in the queue
 */
TEST_F(MatchingEngineTest, MatchingTest25) {
  auto sym = "H";

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  auto id = mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::SELL, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 2);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);

  OrderCancelRequest req;
  req.mOrderId = id;
  req.mSymbol = sym;
  req.mTraderId = "TraderW";

  mMatchingEngine.cancel(mExecutionContext, req);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.numOfOrders(), 1);
  EXPECT_EQ(book->begin<Side::SELL>()->second.front().getTraderId(), "TraderY");
}

/**
 * @brief
 * Trader W, Y place BUY order of 200 on stock H at 10, 20
 * Trader W cancel the order
 * W should be notified the order has been cancelled
 * Y's order should in the queue
 */
TEST_F(MatchingEngineTest, MatchingTest26) {
  auto sym = "H";

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  auto id = mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderW", sym, 10, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  mMatchingEngine.insert<Side::BUY, OrderStyle::LIMIT_ORDER>(
      mExecutionContext, "TraderY", sym, 20, 200);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 2);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);

  OrderCancelRequest req;
  req.mOrderId = id;
  req.mSymbol = sym;
  req.mTraderId = "TraderW";

  mMatchingEngine.cancel(mExecutionContext, req);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 1);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
  EXPECT_EQ(book->begin<Side::BUY>()->second.numOfOrders(), 1);
  EXPECT_EQ(book->begin<Side::BUY>()->second.front().getTraderId(), "TraderY");
}

/**
 * @brief
 * Trader W cancel the order when the book is empty
 * W should be notified the order has been cancelled
 * Y's order should in the queue
 */
TEST_F(MatchingEngineTest, MatchingTest27) {
  auto sym = "H";

  auto book = mMatchingEngine.getOrderBookMap()[sym];
  auto traderMap = mExecutionContext.getTraderMap();

  OrderCancelRequest req;
  req.mOrderId = 0;
  req.mSymbol = sym;
  req.mTraderId = "TraderW";

  mMatchingEngine.cancel(mExecutionContext, req);
  EXPECT_EQ(book->getNumOfLevels<Side::BUY>(), 0);
  EXPECT_EQ(book->getNumOfLevels<Side::SELL>(), 0);
}
