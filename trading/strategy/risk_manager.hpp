#pragma once

#include "common/types.hpp"
#include "common/macros.hpp"
#include "common/logging.hpp"

#include "trading/strategy/position_keeper.hpp"
#include "trading/strategy/om_order.hpp"

namespace Trading {

	class OrderManager;

	enum class RiskCheckResult :int8_t {
		INVALID = 0,
		ORDER_TOO_LARGE = 1,
		POSITION_TOO_LARGE = 2,
		LOSS_TOO_LARGE = 3,
		ALLOWED = 4,
	};

	inline auto riskCheckResultToString(RiskCheckResult result) {
		switch (result) {
		case RiskCheckResult::INVALID:
			return "INVALID";
		case RiskCheckResult::ORDER_TOO_LARGE:
			return "ORDER_TOO_LARGE";
		case RiskCheckResult::POSITION_TOO_LARGE:
			return "POSITION_TOO_LARGE";
		case RiskCheckResult::LOSS_TOO_LARGE:
			return "LOSS_TOO_LARGE";
		case RiskCheckResult::ALLOWED:
			return "ALLOWED";
		}
		return "";
	}
	struct RiskInfo {
		const PositionInfo* m_position_info = nullptr;
		RiskCfg m_risk_cfg;

		std::string toString() const;

		RiskCheckResult checkPreTradeRisk(Side side, Qty qty) const noexcept;
	};

	typedef std::array<RiskInfo, ME_MAX_TICKERS> TickerRiskInfoHashMap;


	class RiskManager {
		std::string m_time_str;
		Common::Logger* m_logger = nullptr;

		TickerRiskInfoHashMap m_ticker_risk;

	public:
		RiskManager(Common::Logger* logger, const PositionKeeper* position_keeper,
			const TradeEngineCfgHashMap& ticker_cfg);

		RiskCheckResult checkPreTradeRisk(TickerId ticker_id, Side side, Qty qty) const noexcept;
	};

}