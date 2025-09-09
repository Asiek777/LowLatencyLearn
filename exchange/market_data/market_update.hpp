#pragma once

#include <sstream>

#include "common/types.hpp"
#include "common/lf_queue.hpp"

using namespace Common;

namespace Exchange {

#pragma pack(push, 1)
	enum class MarketUpdateType : int8_t {
		INVALID = 0,
		CLEAR = 1,
		ADD = 2,
		MODIFY = 3,
		CANCEL = 4,
		TRADE = 5,
		SNAPSHOT_START = 6,
		SNAPSHOT_END = 7,		
	};

	inline std::string marketUpdateTypeToString(MarketUpdateType marketUpdateType) {
		switch (marketUpdateType) {
		case MarketUpdateType::ADD:
			return "ADD";
		case MarketUpdateType::CLEAR:
			return "CLEAR";
		case MarketUpdateType::MODIFY:
			return "MODIFY";
		case MarketUpdateType::CANCEL:
			return "CANCEL";
		case MarketUpdateType::TRADE:
			return "TRADE";
		case MarketUpdateType::SNAPSHOT_START:
			return "SNAPSHOT_START";
		case MarketUpdateType::SNAPSHOT_END:
			return "SNAPSHOT_END";
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


	struct MDPMarketUpdate {
		size_t m_seq_num = 0;
		MEMarketUpdate m_me_market_update;

		auto toString() const {
			std::stringstream ss;
			ss << "MDPMarketUpdate" <<
				" [" <<
				" seq:" << m_seq_num <<
				" :" << m_me_market_update.toString() <<
				"]";
			return ss.str();
		}

	};
#pragma pack(pop)

		typedef LFQueue<MEMarketUpdate> MEMarketUpdateLFQueue;
		typedef LFQueue<MDPMarketUpdate> MDPMarketUpdateLFQueue;
}