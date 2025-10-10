#include "trade_engine.hpp"


namespace Trading {

	TradeEngine::TradeEngine(Common::ClientId client_id, AlgoType algo_type,
		const TradeEngineCfgHashMap& ticker_cfg,
		Exchange::ClientRequestLFQueue* client_requests,
		Exchange::ClientResponseLFQueue* client_responses,
		Exchange::MEMarketUpdateLFQueue* market_updates) :
		m_client_id(client_id), m_outgoing_ogw_requests(client_requests),
		m_incoming_ogw_responses(client_responses), m_incoming_md_updates(market_updates),
		m_logger("trading_engine_" + std::to_string(client_id) + ".log"),
		m_feature_engine(&m_logger), m_position_keeper(&m_logger),
		m_order_manager(&m_logger, this, m_risk_manager),
		m_risk_manager(&m_logger, &m_position_keeper, ticker_cfg) {

		f_algoOnOrderBookUpdate = [this]
		(auto ticker_id, auto price, auto side, auto book) {
			defaultAlgoOnOrderBookUpdate(ticker_id, price, side, book);	};

		f_algoOnTradeUpdate = [this](auto market_update, auto book) {
			defaultAlgoOnTradeUpdate(market_update, book); };

		f_algoOnOrderUpdate = [this](auto client_response) {
			defaultAlgoOnOrderUpdate(client_response);	};

		if (algo_type == AlgoType::MAKER) {
			m_mm_algo = new MarketMaker(&m_logger, this, &m_feature_engine,
				&m_order_manager, ticker_cfg);
		}
		else if (algo_type == AlgoType::TAKER) {
			m_taker_algo = new LiquidityTaker(&m_logger, this,
				&m_feature_engine, &m_order_manager, ticker_cfg);
		}

		for (TickerId i = 0; i < ticker_cfg.size(); i++) {
			m_logger.log("%:% %() % Initialized % Ticker:% %.\n", __FILE__, __LINE__,
				__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
				algoTypeToString(algo_type), i, ticker_cfg.at(i).toString());
		}
	}

	TradeEngine::~TradeEngine() {
		m_run = false;

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(1s);

		delete m_mm_algo; m_mm_algo = nullptr;
		delete m_taker_algo; m_taker_algo = nullptr;

		for (auto& order_book : m_ticker_order_book) {
			delete order_book;
			order_book = nullptr;
		}

		m_outgoing_ogw_requests = nullptr;
		m_incoming_ogw_responses = nullptr;
		m_incoming_md_updates = nullptr;
	}

	void TradeEngine::start() {
		m_run = true;
		ASSERT(Common::createAndStartThread(-1, "Trading/TradeEngine",
			[this] {run(); }) != nullptr, "Failed to start TradeEngine thread.");
	}

	void TradeEngine::stop() {
		while (m_incoming_ogw_responses->size() || m_incoming_md_updates->size()) {
			m_logger.log("%: % %() % Sleeping till all updates are consumed ogw-size:"
				"% md-size: % \n", __FILE__, __LINE__, __FUNCTION__,
				Common::getCurrentTimeStr(&m_time_str), m_incoming_ogw_responses->size(),
				m_incoming_md_updates->size());

			using namespace std::literals::chrono_literals;
			std::this_thread::sleep_for(10ms);
		}

		m_logger.log("%: % %() % POSITIONS\n%\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), m_position_keeper.toString());
		m_run = false;
	}

	void TradeEngine::run() noexcept {
		m_logger.log("%: % %() %\n", __FILE__, __LINE__,
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str));

		while (m_run) {
			for (auto client_response = m_incoming_ogw_responses->getNextToRead();
				client_response; client_response = m_incoming_ogw_responses->getNextToRead()) {
				m_logger.log("%:% %() % Processing %\n", __FILE__, __LINE__,
					__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
					client_response->toString().c_str());

				onOrderUpdate(client_response);
				m_incoming_ogw_responses->updateReadIndex();
				m_last_event_time = Common::getCurrentNanos();
			}

			for (auto market_update = m_incoming_md_updates->getNextToRead();
				market_update; market_update = m_incoming_md_updates->getNextToRead()) {
				m_logger.log("%:% %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__,
					Common::getCurrentTimeStr(&m_time_str), market_update->toString().c_str());
				ASSERT(market_update->m_ticker_id < m_ticker_order_book.size(),
					"Uknown ticker-id on update:" + market_update->toString());
				m_ticker_order_book[market_update->m_ticker_id]->onMarketUpdate(market_update);
				m_incoming_md_updates->updateReadIndex();
				m_last_event_time = Common::getCurrentNanos();
			}
		}
	}

	void TradeEngine::onOrderBookUpdate(TickerId ticker_id, Price price,
		Side side, MarketOrderBook* book) noexcept {
		m_logger.log("%:% %() % ticker:% price:% side:%\n", __FILE__, __LINE__,
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), ticker_id,
			Common::priceToString(price).c_str(), Common::sideToString(side).c_str());

		const auto bbo = book->getBBO();
		m_position_keeper.updateBBO(ticker_id, bbo);
		m_feature_engine.onOrderBookUpdate(ticker_id, price, side, book);
		f_algoOnOrderBookUpdate(ticker_id, price, side, book);
	}

	void TradeEngine::onTradeUpdate(const Exchange::MEMarketUpdate* market_update,
		MarketOrderBook* book) noexcept {
		m_logger.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__, 
			Common::getCurrentTimeStr(&m_time_str), market_update->toString().c_str());

		m_feature_engine.onTradeUpdate(market_update, book);
		f_algoOnTradeUpdate(market_update, book);
	}

	void TradeEngine::onOrderUpdate(const Exchange::MEClientResponse* client_response)
		noexcept {
		m_logger.log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), client_response->toString().c_str());

		if (client_response->m_type == Exchange::ClientResponseType::FILLED) [[unlikely]] {
			m_position_keeper.addFill(client_response);
		}

		f_algoOnOrderUpdate(client_response);
	}

	auto TradeEngine::silentSeconds() {
		return (Common::getCurrentNanos() - m_last_event_time) / NANOS_TO_SECS;
	}

	void TradeEngine::sendClientRequest(const Exchange::MEClientRequest* client_request)
		noexcept {
		m_logger.log("% :% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), client_request->toString().c_str());

		auto next_write = m_outgoing_ogw_requests->getNextToWriteTo();
		*next_write = std::move(*client_request);
		m_outgoing_ogw_requests->updateWriteIndex();
	}

	void TradeEngine::defaultAlgoOnOrderBookUpdate(TickerId ticker_id,
		Price price, Side side, const MarketOrderBook* book) {
		m_logger.log("%: % %() % ticker: % price: % side: %\n", __FILE__,
			__LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
			ticker_id, Common::priceToString(price).c_str(),
			Common::sideToString(side).c_str());
	}

	void TradeEngine::defaultAlgoOnTradeUpdate(
		const Exchange::MEMarketUpdate* market_update, MarketOrderBook* book) {
		m_logger.log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str),
			market_update->toString().c_str());
	}

	void TradeEngine::defaultAlgoOnOrderUpdate(
		const Exchange::MEClientResponse* client_response) {
		m_logger.log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str),
			client_response->toString().c_str());
	}

}
