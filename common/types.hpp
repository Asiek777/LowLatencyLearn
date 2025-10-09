#pragma once	

#include <limits>
#include <cstdint>
#include <sstream>

#include "common/macros.hpp"


namespace Common {

	constexpr size_t ME_MAX_TICKERS = 8;

	constexpr size_t  ME_MAX_CLIENT_UPDATES = 256 * 1024;
	constexpr size_t  ME_MAX_MARKET_UPDATES = 256 * 1024;

	constexpr size_t ME_MAX_NUM_CLIENTS = 256;
	constexpr size_t ME_MAX_ORDER_IDS = 1024 * 1042;
	constexpr size_t ME_MAX_PRICE_LEVELS = 256;

	typedef uint64_t OrderId;
	constexpr auto OrderId_INVALID = std::numeric_limits<OrderId>::max();

	inline std::string orderIdToString(OrderId order_id) {
		if (order_id == OrderId_INVALID) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(order_id);
	}

	typedef uint32_t TickerId;
	constexpr auto TickerId_INVALID = std::numeric_limits<TickerId>::max();

	inline std::string tickerIdToString(TickerId ticker_id) {
		if (ticker_id == TickerId_INVALID) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(ticker_id);
	}

	typedef uint32_t ClientId;
	constexpr auto ClientId_INVALID = std::numeric_limits<ClientId>::max();

	inline std::string clientIdToString(ClientId client_id) {
		if (client_id == ClientId_INVALID) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(client_id);
	}

	typedef int64_t Price;
	constexpr auto Price_INVALID = std::numeric_limits<Price>::max();

	inline std::string priceToString(Price price) {
		if (price == Price_INVALID) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(price);
	}

	typedef uint32_t Qty;
	constexpr auto Qty_INVALID = std::numeric_limits<Qty>::max();

	inline std::string qtyToString(Qty qty) {
		if (qty == Qty_INVALID) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(qty);
	}

	typedef uint64_t Priority;
	constexpr auto Priority_INVALID = std::numeric_limits<Priority>::max();

	inline std::string priorityToString(Priority priority) {
		if (priority == Priority_INVALID) [[unlikely]] {
			return "INVALID";
		}

		return std::to_string(priority);
	}

	enum class Side : int8_t {
		INVALID = 0,
		BUY = 1,
		SELL = -1,
	};

	inline std::string sideToString(Side side) {
		switch (side) {
		case Side::BUY:
			return "BUY";
		case Side::SELL:
			return "SELL";
		case Side::INVALID:
			return "INVALID";
		}

		return "UNKNOWN";
	}

	inline constexpr auto sideToIndex(Side side) noexcept {
		return static_cast<size_t>(side) + 1;
	}

	inline constexpr auto sideToValue(Side side) noexcept {
		return static_cast<int>(side);
	}

	struct RiskCfg {
		Qty m_max_order_size = 0;
		Qty m_max_position = 0;
		double m_max_loss = 0;

		auto toString() const {
			std::stringstream ss;

			ss << "RiskCfg{" <<
				"max-order-size:" << qtyToString(m_max_order_size) << " " <<
				"max-position:" << qtyToString(m_max_position) << " " <<
				"max-loss:" << m_max_loss <<
				"}";
			return ss.str();
		}
	};

	struct TradeEngineCfg {
		Qty m_clip = 0;
		double m_threshold = 0;
		RiskCfg m_risk_cfg;

		auto toString() const {
			std::stringstream ss;
			ss << "TradeEngineCfg{" <<
				"clip:" << qtyToString(m_clip) << " " <<
				"thresh:" << m_threshold << " " <<
				"risk:" << m_risk_cfg.toString() <<
				"}";
			return ss.str();
		}
	};
	typedef std::array<TradeEngineCfg, ME_MAX_TICKERS> TradeEngineCfgHashMap;

	enum class AlgoType :int8_t {
		INVALID = 0,
		RANDOM = 1,
		MAKER = 2,
		TAKER = 3,
		MAX = 4,
	};

	inline std::string algoTypeToString(AlgoType type) {
		switch (type)
		{
		case Common::AlgoType::INVALID:
			return "INVALID";
		case Common::AlgoType::RANDOM:
			return "RANDOM";
		case Common::AlgoType::MAKER:
			return "MAKER";
		case Common::AlgoType::TAKER:
			return "TAKER";
		case Common::AlgoType::MAX:
			return "MAX";
		}
		return "UNKNOWN";
	};

	inline AlgoType stringToAlgoType(const std::string& str) {
		for (auto i = static_cast<int>(AlgoType::INVALID);
			i <= static_cast<int>(AlgoType::MAX); i++) {
			const auto algo_type = static_cast<AlgoType>(i);
			if (algoTypeToString(algo_type) == str)
				return algo_type;
		}
		return AlgoType::INVALID;
	}

}
