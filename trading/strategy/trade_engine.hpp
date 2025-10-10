#pragma once

#include <functional>

#include "common/thread_utils.hpp"
#include "common/time_utils.hpp"
#include "common/lf_queue.hpp"
#include "common/macros.hpp"
#include "common/logging.hpp"

#include "exchange/order_server/client_request.hpp"
#include "exchange/order_server/client_response.hpp"
#include "exchange/market_data/market_update.hpp"

#include "trading/strategy/market_order_book.hpp"
#include "trading/strategy/feature_engine.hpp"
#include "trading/strategy/position_keeper.hpp"
#include "trading/strategy/order_manager.hpp"
#include "trading/strategy/risk_manager.hpp"
 
#include "trading/strategy/market_maker.hpp"
#include "trading/strategy/liquidity_taker.hpp""


namespace Trading {

	class TradeEngine {
		const ClientId m_client_id;

		MarketOrderBookHashMap m_ticker_order_book;
		
		Exchange::ClientRequestLFQueue* m_outgoing_ogw_requests = nullptr;
		Exchange::ClientResponseLFQueue* m_incoming_ogw_responses = nullptr;
		Exchange::MEMarketUpdateLFQueue* m_incoming_md_updates = nullptr;

		Nanos m_last_event_time = 0;
		volatile bool m_run = false;

		std::string m_time_str;
		Logger m_logger;

		FeatureEngine m_feature_engine;
		PositionKeeper m_position_keeper;
		OrderManager m_order_manager;
		RiskManager m_risk_manager;

		MarketMaker *m_mm_algo = nullptr;
		LiquidityTaker* m_taker_algo = nullptr;


		void defaultAlgoOnOrderBookUpdate(TickerId ticker_id, Price price, Side side,
			const MarketOrderBook* book);
		void defaultAlgoOnTradeUpdate(const Exchange::MEMarketUpdate* market_update,
			MarketOrderBook* book);
		void defaultAlgoOnOrderUpdate(const Exchange::MEClientResponse* client_response);

		void run() noexcept;


	public:
		std::function<void(TickerId ticker_id, Price price, Side side,
			const MarketOrderBook* book)> f_algoOnOrderBookUpdate;
		std::function<void(const Exchange::MEMarketUpdate* market_update,
			MarketOrderBook* book)> f_algoOnTradeUpdate;
		std::function<void(const Exchange::MEClientResponse* client_response)>
			f_algoOnOrderUpdate;

		TradeEngine(Common::ClientId client_id, AlgoType algo_type, 
			const TradeEngineCfgHashMap& ticker_cfg, 
			Exchange::ClientRequestLFQueue* client_requests, 
			Exchange::ClientResponseLFQueue* client_responses, 
			Exchange::MEMarketUpdateLFQueue* market_updates);
		~TradeEngine();

		void start();
		void stop();

		void sendClientRequest(const Exchange::MEClientRequest* client_request) noexcept;

		auto initLastEventTime() { m_last_event_time = Common::getCurrentNanos(); }
		auto silentSeconds();
		auto clientId() const { return m_client_id; }

		void onOrderBookUpdate(TickerId ticker_id, Price price, 
			Side side, MarketOrderBook* book) noexcept;
		void onTradeUpdate(const Exchange::MEMarketUpdate* market_update, 
			MarketOrderBook* book) noexcept;
		void onOrderUpdate(const Exchange::MEClientResponse* client_response) noexcept;

	};
}