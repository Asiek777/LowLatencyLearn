#include "trading/strategy/market_maker.hpp"

#include "trading/strategy/trade_engine.hpp"


namespace Trading {
	MarketMaker::MarketMaker(Common::Logger* logger, TradeEngine* trade_engine,
		const FeatureEngine* feature_engine, OrderManager* order_manager,
		const TradeEngineCfgHashMap& ticker_cfg) :
		m_feature_engine(feature_engine), m_order_manager(order_manager),
		m_logger(logger), m_ticker_cfg(ticker_cfg) {
		trade_engine->f_algoOnOrderBookUpdate = [this]
		(auto ticker_id, auto price, auto side, auto book) {
			onOrderBookUpdate(ticker_id, price, side, book); };

		trade_engine->f_algoOnTradeUpdate = [this](auto market_update, auto book) {
			onTradeUpdate(market_update, book); };

		trade_engine->f_algoOnOrderUpdate = [this](auto client_response) {
			onOrderUpdate(client_response);	};
	}

	void MarketMaker::onOrderBookUpdate(TickerId ticker_id, Price price, Side side,
		const MarketOrderBook* book) noexcept {
		m_logger->log("%: % %() % ticker: % price: % side: %\n", __FILE__, __LINE__,
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), ticker_id,
			Common::priceToString(price).c_str(), Common::sideToString(side).c_str());

		const auto bbo = book->getBBO();
		const auto fair_price = m_feature_engine->getMktPrice();

		if (bbo->m_bid_price != Price_INVALID && bbo->m_ask_price != Price_INVALID &&
			fair_price != Feature_INVALID) [[likely]] {
			m_logger->log("%: % %() % % fair-price: %\n", __FILE__, __LINE__,
				__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
				bbo->toString().c_str(), fair_price);
			const auto clip = m_ticker_cfg.at(ticker_id).m_clip;
			const auto threshold = m_ticker_cfg.at(ticker_id).m_threshold;

			const auto bid_price = bbo->m_bid_price -
				(fair_price - bbo->m_bid_price >= threshold ? 0 : 1);
			const auto ask_price = bbo->m_ask_price +
				(bbo->m_ask_price - fair_price >= threshold ? 0 : 1);
			m_order_manager->moveOrders(ticker_id, bid_price, ask_price, clip);
		}
	}

	void MarketMaker::onTradeUpdate(const Exchange::MEMarketUpdate* market_update,
		MarketOrderBook* /*book*/) noexcept {
		m_logger->log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__, 
			Common::getCurrentTimeStr(&m_time_str), market_update->toString().c_str());
	}

	void MarketMaker::onOrderUpdate(
		const Exchange::MEClientResponse* client_response) noexcept {
		m_logger->log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), client_response->toString().c_str());
		m_order_manager->onOrderUpdate(client_response);
	}
}
