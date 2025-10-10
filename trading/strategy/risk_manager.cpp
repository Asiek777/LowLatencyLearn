#include "trading/strategy/risk_manager.hpp"

namespace Trading {

	std::string RiskInfo::toString() const {
		std::stringstream ss;
		ss << "RiskInfo" << "[" <<
			"pos:" << m_position_info->toString() << " " <<
			m_risk_cfg.toString() <<
			"]";
		return ss.str();
	}

	RiskCheckResult RiskInfo::checkPreTradeRisk(Side side, Qty qty) const noexcept {

		if (qty > m_risk_cfg.m_max_order_size) [[unlikely]]
			return RiskCheckResult::ORDER_TOO_LARGE;

		if (std::abs(m_position_info->m_position + 
			sideToValue(side) * static_cast<int32_t>(qty)) > 
			static_cast<int32_t>(m_risk_cfg.m_max_position)) [[unlikely]]
			return RiskCheckResult::POSITION_TOO_LARGE;

		if (m_position_info->m_total_pnl < m_risk_cfg.m_max_loss) [[unlikely]]
			return RiskCheckResult::LOSS_TOO_LARGE;

		return RiskCheckResult::ALLOWED;
	}

	RiskManager::RiskManager(Common::Logger* logger, const PositionKeeper* position_keeper, 
		const TradeEngineCfgHashMap& ticker_cfg) : m_logger(logger) {
		
		for (TickerId i = 0; i < ME_MAX_TICKERS; i++) {
			m_ticker_risk.at(i).m_position_info = position_keeper->getPositionInfo(i);
			m_ticker_risk.at(i).m_risk_cfg = ticker_cfg[i].m_risk_cfg;
		}
	}
	RiskCheckResult RiskManager::checkPreTradeRisk(TickerId ticker_id, 
		Side side, Qty qty) const noexcept {
		return m_ticker_risk.at(ticker_id).checkPreTradeRisk(side, qty);
	}
}