#pragma once

#include "common/macros.hpp"
#include "common/types.hpp"
#include "common/logging.hpp"

#include "exchange/order_server/client_response.hpp"
#include "trading/strategy/market_order_book.hpp"

using namespace Common;

namespace Trading {

	struct PositionInfo {

		int32_t m_position = 0;
		double m_real_pnl = 0, m_unreal_pnl = 0, m_total_pnl = 0;
		std::array<double, sideToIndex(Side::BUY) + 1> m_open_vwap;
		Qty m_volume = 0;
		const BBO* m_bbo = nullptr;

		auto toString() const {
			std::stringstream ss;
			ss << "Position{" <<
				"pos:" << m_position <<
				" u-pnl:" << m_unreal_pnl <<
				" r-pnl:" << m_real_pnl <<
				" t-pnl:" << m_total_pnl <<
				" vol:" << qtyToString(m_volume) <<
				" vwaps:[" << (m_position ?
					m_open_vwap.at(sideToIndex(Side::BUY)) / std::abs(m_position) : 0) <<
				"X" << (m_position ?
					m_open_vwap.at(sideToIndex(Side::SELL)) / std::abs(m_position) : 0) <<
				"] " <<
				(m_bbo ? m_bbo->toString() : "") << "}";
			return ss.str();
		}

		void addFill(const Exchange::MEClientResponse* client_response, Logger* logger) noexcept {
			const auto old_position = m_position;
			const auto side_index = sideToIndex(client_response->m_side);
			const auto opp_side_index = sideToIndex(
				client_response->m_side == Side::BUY ? Side::SELL : Side::BUY);
			const auto side_value = sideToValue(client_response->m_side);
			m_position += client_response->m_exec_qty * side_value;
			m_volume += client_response->m_exec_qty;

			if (old_position * side_value >= 0) {
				m_open_vwap[side_index] += client_response->m_price * client_response->m_exec_qty;
			}
			else {
				const auto opp_side_vwap = m_open_vwap[opp_side_index] / std::abs(old_position);
				m_real_pnl += std::min(static_cast<int32_t>(client_response->m_exec_qty),
					std::abs(old_position)) * (opp_side_vwap - client_response->m_price) *
					sideToValue(client_response->m_side);
				if (m_position * old_position < 0) {
					m_open_vwap[side_index] = client_response->m_price * std::abs(m_position);
					m_open_vwap[opp_side_index] = 0;
				}
			}

			if (m_position == 0) {
				m_open_vwap[sideToIndex(Side::BUY)] = m_open_vwap[sideToIndex(Side::SELL)] = 0;
				m_unreal_pnl = 0;
			}
			else {
				if (m_position > 0) {
					m_unreal_pnl = (client_response->m_price - m_open_vwap[sideToIndex(Side::BUY)]
						/ std::abs(m_position)) * std::abs(m_position);
				}
				else {
					m_unreal_pnl = (m_open_vwap[sideToIndex(Side::SELL)] / std::abs(m_position) -
						client_response->m_price) * std::abs(m_position);
				}
				m_total_pnl = m_unreal_pnl + m_real_pnl;

				std::string time_str;
				logger->log("%:% %() % % %\n", __FILE__, __LINE__, __FUNCTION__,
					Common::getCurrentTimeStr(&time_str), toString(),
					client_response->toString().c_str());
			}
		}

		void updateBBO(const BBO* bbo, Logger* logger) noexcept {
			std::string time_str;
			m_bbo = bbo;

			if (m_position && bbo->m_bid_price != Price_INVALID &&
				bbo->m_ask_price != Price_INVALID) {
				const auto mid_price = (bbo->m_bid_price + bbo->m_ask_price) * 0.5;

				if (m_position > 0) {
					m_unreal_pnl = (mid_price - m_open_vwap[sideToIndex(Side::BUY)]
						/ std::abs(m_position)) * std::abs(m_position);
				}
				else {
					m_unreal_pnl = (m_open_vwap[sideToIndex(Side::SELL)] / std::abs(m_position) -
						mid_price) * std::abs(m_position);
				}
				const auto old_total_pnl = m_total_pnl;
				m_total_pnl = m_unreal_pnl + m_real_pnl;

				if (m_total_pnl != old_total_pnl) {
					logger->log("%:% %() % % %\n", __FILE__, __LINE__, __FUNCTION__, 
						Common::getCurrentTimeStr(&time_str), toString(), bbo->toString());
				}
			}
		}
	};

	class PositionKeeper {

		std::string m_time_str;
		Common::Logger* m_logger = nullptr;

		std::array<PositionInfo, ME_MAX_TICKERS> m_ticker_position;

	public:
		PositionKeeper(Common::Logger* logger) : m_logger(logger) {};

		auto getPositionInfo(TickerId ticker_id) const noexcept {
			return &(m_ticker_position.at(ticker_id));
		}

		auto toString() const {
			double total_pnl = 0;
			Qty total_vol = 0;

			std::stringstream ss;
			for (TickerId i = 0; i < m_ticker_position.size(); i++) {
				ss << "TickerId: " << tickerIdToString(i) << " " << 
					m_ticker_position.at(i).toString() << "\n";
				total_pnl += m_ticker_position.at(i).m_total_pnl;
				total_vol += m_ticker_position.at(i).m_volume;
			}
			ss << "Total PnL: " << total_pnl << " Vol: " << total_vol << "\n";

			return ss.str();
		}

		void addFill(const Exchange::MEClientResponse* client_response) noexcept {
			m_ticker_position.at(client_response->m_ticker_id).
				addFill(client_response, m_logger);
		}

		auto updateBBO(TickerId ticker_id, const BBO* bbo) noexcept {
			m_ticker_position.at(ticker_id).updateBBO(bbo, m_logger);
		}
	};
}