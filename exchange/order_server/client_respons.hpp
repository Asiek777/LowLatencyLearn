#pragma once

#include <sstream>

#include "common/types.hpp"
#include "common/lf_queue.hpp"

using namespace Common;

namespace Exchange {

#pragma pack(push, 1)
	enum class ClientResponseType : int8_t {
		INVALID = 0,
		ACCEPTED = 1,
		CANCELED = 2,
		FILLED = 3,
		CANCEL_REJECTED = 4,
	};

	inline std::string clientResponseTypeToString(ClientResponseType clientResponseType) {
		switch (clientResponseType) {
		case ClientResponseType::ACCEPTED:
			return "ACCEPTED";
		case ClientResponseType::CANCELED:
			return "CANCELED";
		case ClientResponseType::FILLED:
			return "FILLED";
		case ClientResponseType::CANCEL_REJECTED:
			return "CANCEL_REJECTED";
		case ClientResponseType::INVALID:
			return "INVALID";
		}
		return "UNKNOWN";
	}

	struct MEClientResponse {
		ClientResponseType m_type = ClientResponseType::INVALID;

		ClientId m_client_id = ClientId_INVALID;
		TickerId m_ticker_id = TickerId_INVALID;
		OrderId m_client_order_id = OrderId_INVALID;
		OrderId m_market_order_id = OrderId_INVALID;
		Side m_side = Side::INVALID;
		Price m_price = Price_INVALID;
		Qty m_exec_qty = Qty_INVALID;
		Qty m_leaves_qty = Qty_INVALID;

		auto toString() const {
			std::stringstream ss;
			ss << "MEClientResponse" <<
				" [" <<
				"type:" << clientResponseTypeToString(m_type) <<
				" client:" << clientIdToString(m_client_id) <<
				" ticker:" << tickerIdToString(m_ticker_id) <<
				" coid:" << orderIdToString(m_client_order_id) <<
				" moid:" << orderIdToString(m_market_order_id) <<
				" side:" << sideToString(m_side) <<
				" exec_qty:" << qtyToString(m_exec_qty) <<
				" leaves_qty:" << qtyToString(m_leaves_qty) <<
				" price:" << priceToString(m_price) <<
				"]";
			return ss.str();
		}
	};

#pragma pack(pop)

	typedef LFQueue<MEClientResponse> ClientResponseLFQueue;
}