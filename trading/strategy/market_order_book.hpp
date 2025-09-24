#pragma once

#include "common/types.hpp"
#include "common/mem_pool.hpp"
#include "common/logging.hpp"
#include "common/time_utils.hpp"

#include "trading/strategy/market_order.hpp"
#include "exchange/market_data/market_update.hpp"

namespace Trading {

	class TradeEngine;

	class MarketOrderBook final {

		const TickerId m_ticker_id;

		TradeEngine* m_trade_engine = nullptr;

		OrderHashMap m_oid_to_order;

		MemPool<MarketOrdersAtPrice> m_orders_at_price_pool;
		MarketOrdersAtPrice* m_bids_by_price = nullptr;
		MarketOrdersAtPrice* m_asks_by_price = nullptr;

		ORdersAtPriceHashMap m_price_orders_at_price;

		MemPool<MarketOrder> m_order_pool;

		BBO m_bbo;

		std::string m_time_str;
		Logger* m_logger = nullptr;

		void setTradeEngine(TradeEngine* trade_engine) { m_trade_engine = trade_engine; }
		void onMarketUpdate(const Exchange::MEMarketUpdate* market_update) noexcept;

		unsigned long priceToIndex(Price price) const noexcept;
		MarketOrdersAtPrice* getOrdersAtPrice(Price price) const noexcept;

		void addOrder(MarketOrder* order) noexcept;
		void addOrdersAtPrice(MarketOrdersAtPrice* new_orders_at_price) noexcept;

		void removeOrder(MarketOrder* order) noexcept;
		void removeOrdersAtPrice(Side side, Price price) noexcept;

		void updateBBO(bool update_bid, bool update_ask) noexcept;

	public:

		MarketOrderBook(TickerId ticker_id, Logger * logger);
		~MarketOrderBook();
	};

	typedef std::array<MarketOrderBook*, ME_MAX_TICKERS> MarketOrderBookHashMap;
}