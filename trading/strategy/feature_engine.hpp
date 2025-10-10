#pragma once

#include "common/macros.hpp"
#include "common/logging.hpp"
#include "common/types.hpp"

#include "trading/strategy/market_order_book.hpp"

using namespace Common;

namespace Trading {

	constexpr auto Feature_INVALID = std::numeric_limits<double>::quiet_NaN();

	class FeatureEngine {

		std::string m_time_str;
		Common::Logger* m_logger = nullptr;

		double m_mkt_price = Feature_INVALID;
		double m_agg_trade_qty_ratio = Feature_INVALID;

	public:

		FeatureEngine(Common::Logger* logger) : m_logger(logger) {}

		auto getMktPrice() const noexcept { return m_mkt_price; }
		auto getAggTradeQtyRatio() const noexcept { return m_agg_trade_qty_ratio; }

		void onOrderBookUpdate(TickerId ticker_id, Price price, Side side, MarketOrderBook* book) noexcept {
			auto bbo = book->getBBO();

			if (bbo->m_bid_price != Price_INVALID && bbo->m_ask_price != Price_INVALID) [[likely]] {
				m_mkt_price = (bbo->m_bid_price * bbo->m_ask_qty + bbo->m_ask_price * bbo->m_bid_qty) /
					static_cast<double>(bbo->m_bid_qty + bbo->m_ask_qty);
			}

			m_logger->log("%: % %() % ticker: % price: % side: % mkt-price: % agg - trade - ratio : % \n",
				__FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str), ticker_id,
				Common::priceToString(price).c_str(), Common::sideToString(side).c_str(),
				m_mkt_price, m_agg_trade_qty_ratio);
		}

		void onTradeUpdate(const Exchange::MEMarketUpdate* market_update, MarketOrderBook* book) noexcept {
			auto bbo = book->getBBO();

			if (bbo->m_bid_price != Price_INVALID && bbo->m_ask_price != Price_INVALID) [[likely]] {
				m_agg_trade_qty_ratio = static_cast<double>(market_update->m_qty) /
					(market_update->m_side == Side::BUY ? bbo->m_ask_qty : bbo->m_bid_price);
			}
			m_logger->log("%: % %() % % mkt-price: % agg-trade-ratio: % \n", __FILE__, 
				__LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str), 
				market_update->toString().c_str(), m_mkt_price, m_agg_trade_qty_ratio);
		}
	};
}