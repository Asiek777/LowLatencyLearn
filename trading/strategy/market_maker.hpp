#pragma once

#include "common/macros.hpp"
#include "common/logging.hpp"

#include "trading/strategy/feature_engine.hpp"
#include "trading/strategy/order_manager.hpp"

using namespace Common;

namespace Trading {

	class MarketMaker {

		const FeatureEngine* m_feature_engine = nullptr;
		OrderManager* m_order_manager = nullptr;

		std::string m_time_str;
		Common::Logger* m_logger = nullptr;

		const TradeEngineCfgHashMap m_ticker_cfg;

		void onOrderBookUpdate(TickerId ticker_id, Price price, Side side, 
			const MarketOrderBook* book) noexcept;
		void onTradeUpdate(const Exchange::MEMarketUpdate* market_update, 
			MarketOrderBook* book) noexcept;
		void onOrderUpdate(const Exchange::MEClientResponse* client_response) noexcept;


	public:
		MarketMaker(Common::Logger* logger, TradeEngine* trade_engine, 
			const FeatureEngine* feature_engine, OrderManager* order_manager, 
			const TradeEngineCfgHashMap& ticker_cfg);
	};
}