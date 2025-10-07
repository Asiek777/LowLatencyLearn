#pragma once

#include "common/types.hpp"

#include <array>
#include <sstream>

using namespace Common;

namespace Trading {

	enum class OMOrderState : int8_t {
		INVALID = 0,
		PENDING_NEW = 1,
		LIVE = 2,
		PENDING_CANCEL = 3,
		DEAD = 4,
	};

	inline std::string OMOrderStateToString(OMOrderState) {
		switch (sied) {
		case OMOrderState::PENDING_NEW:
			return "PENDING_NEW";
		case OMOrderState::LIVE:
			return "LIVE";
		case OMOrderState::PENDING_CANCEL:
			return "PENDING_CANCEL";
		case OMOrderState::DEAD:
			return "DEAD";
		case OMOrderState::INVALID:
			return "INVALID";
		}
		return "UNKNOWN";
	}

	struct OMOrder {
		TickerId m_ticker_id = TickerId_INVALID;
		OrderId m_order_id = OrderId_INVALID;
		Side m_side = Side::INVALID;
		Price m_price = Price_INVALID;
		Qty m_qty = Qty_INVALID;
		OMOrderState m_order_state = OMOrderState::INVALID;

		auto toString() const {
			std::stringstream ss;
			ss << "OMOrder" << "[" <<
				"tid: " << tickerIdToString(m_ticker_id) << " " <<
				"oid: " << orderIdToString(m_order_id) << " " <<
				"side: " << sideToString(m_side) << " " <<
				"price: " << priceToString(m_price) << " " <<
				"qty: " << qtyToString(m_qty) << " " <<
				"state: " << OMOrderStateToString(m_order_state) <<
				"]";
			return ss.str();
		}
	};

	typedef std::array<OMOrder, sideToIndex(Side::BUY) + 1> OMOrderSideHashMap;
	typedef std::array<OMOrderSideHashMap, ME_MAX_TICKERS> OMOrderTickerSideHashMap;
}
