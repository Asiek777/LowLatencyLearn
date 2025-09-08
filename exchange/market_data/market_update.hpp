#pragma once

#include <sstream>

#include "common/types.hpp"
#include "common/lf_queue.hpp"

using namespace Common;

namespace Exchange {

#pragma pack(push, 1)
	enum class MarketUpdateType : int8_t {
		INVALID = 0,
		ADD = 1,
		MODIFY = 2,
		CANCEL = 3,
		TRADE = 4,
	};

	inline std::string marketUpdateTypeToString(MarketUpdateType marketUpdateType) {
		switch (marketUpdateType) {
		case MarketUpdateType::ADD:
			return "ADD";
		case MarketUpdateType::MODIFY:
			return "MODIFY";
		case MarketUpdateType::CANCEL:
			return "CANCEL";
		case MarketUpdateType::TRADE:
			return "TRADE";
		case MarketUpdateType::INVALID:
			return "INVALID";
		}
		return "UNKNOWN";
	}

	struct MEMarketUpdate {
		MarketUpdateType m_type = MarketUpdateType::INVALID;

		OrderId m_order_id = OrderId_INVALID;
		TickerId m_ticker_id = TickerId_INVALID;
		Side m_side = Side::INVALID;
		Price m_price = Price_INVALID;
		Qty m_qty = Qty_INVALID;
		Priority m_priority = Priority_INVALID;

		auto toString() const {
			std::stringstream ss;
			ss << "MEMarketUpdate" <<
				" [" <<
				"type:" << marketUpdateTypeToString(m_type) <<
				" ticker:" << tickerIdToString(m_ticker_id) <<
				" oid:" << orderIdToString(m_order_id) <<
				" side:" << sideToString(m_side) <<
				" qty:" << qtyToString(m_qty) <<
				" price:" << priceToString(m_price) <<
				" priority:" << priorityToString(m_priority) <<
				"]";
			return ss.str();
		}
	};

#pragma pack(pop)

		typedef LFQueue<MEMarketUpdate> MEMarketUpdateLFQueue;
}