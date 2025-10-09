#include "liquidity_taker.hpp"

namespace Trading {

	LiquidityTaker::LiquidityTaker(Common::Logger* logger, TradeEngine* trade_engine,
		FeatureEngine* feature_engine, OrderManager* order_manager,
		const TradeEngineCfgHashMap& ticker_cfg) :
		m_feature_engine(feature_engine), m_order_manager(order_manager),
		m_logger(logger), m_ticker_cfg(ticker_cfg) {
		trade_engine->m_algoOnOrderBookUpdate = [this]
		(auto ticker_id, auto price, auto side, auto book) {
			onOrderBookUpdate(ticker_id, price, side, book); };

		trade_engine->m_algoOnTradeupdate = [this](auto market_update, auto book) {
			onTradeUpdate(market_update, book); };

		trade_engine->m_algoOnOrderUpdate = [this](auto client_response) {
			onOrderUpdate(client_response);	}
	}


	void LiquidityTaker::onTradeUpdate(const Exchange::MEMarketUpdate* market_update,
		MarketOrderBook* book) noexcept {
		m_logger->log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), market_update->toString().c_str());

		const auto bbo = book->getBBO();
		const auto agg_qty_ratio = m_feature_engine->getAggTradeQtyRatio();

		if (bbo->m_bid_price != Price_INVALID && bbo->m_ask_price != Price_INVALID &&
			agg_qty_ratio != Feature_INVALID) [[likely]] {
			m_logger->log("%: % %() % % agg-qty-ratio: %\n", __FILE__, __LINE__,
				__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
				bbo->toString().c_str(), agg_qty_ratio);

			const auto clip = m_ticker_cfg.at(market_update->m_ticker_id).m_clip;
			const auto threshold = m_ticker_cfg.at(market_update->m_ticker_id).m_threshold;

			if (agg_qty_ratio >= threshold) {
				if (market_update->m_side == Side::BUY) {
					m_order_manager->moveOrders(market_update->m_ticker_id,
						bbo->m_ask_price, Price_INVALID, clip);
				}
				else {
					m_order_manager->moveOrders(market_update->m_ticker_id,
						Price_INVALID, bbo->m_bid_price, clip);
				}
			}
		}
	}

	void LiquidityTaker::onOrderBookUpdate(TickerId ticker_id, Price price, Side side,
		const MarketOrderBook* /*book*/) noexcept {
		m_logger->log("%:% %() % ticker:% price:% side:%\n", __FILE__, __LINE__, 
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), ticker_id, 
			
			Common::priceToString(price).c_str(), Common::sideToString(side).c_str());
	}

	void LiquidityTaker::onOrderUpdate(
		const Exchange::MEClientResponse* client_response) noexcept {
		m_logger->log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), client_response->toString().c_str());
		m_order_manager->onOrderUpdate(client_response);
	}
}
