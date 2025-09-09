#pragma once

#include <sstream>

#include "common/types.hpp"
#include "common/lf_queue.hpp"

using namespace Common;

namespace Exchange {

#pragma pack(push, 1)
	enum class ClientRequestType : int8_t {
		INVALID = 0,
		NEW = 1,
		CANCEL = 2,
	};

	inline std::string clientRequestTypeToString(ClientRequestType clientRequestType) {
		switch (clientRequestType) {
		case ClientRequestType::NEW:
			return "NEW";
		case ClientRequestType::CANCEL:
			return "CANCEL";
		case ClientRequestType::INVALID:
			return "INVALID";
		}
		return "UNKNOWN";
	}

	struct MEClientRequest {
		ClientRequestType m_type = ClientRequestType::INVALID;

		ClientId m_client_id = ClientId_INVALID;
		TickerId m_ticker_id = TickerId_INVALID;
		OrderId m_order_id = OrderId_INVALID;
		Side m_side = Side::INVALID;
		Price m_price = Price_INVALID;
		Qty m_qty = Qty_INVALID;

		auto toString() const {
			std::stringstream ss;
			ss << "MEClientRequest" << 
				" [" <<
				"type:" << clientRequestTypeToString(m_type) <<
				" client:" << clientIdToString(m_client_id) <<
				" ticker:" << tickerIdToString(m_ticker_id) <<
				" oid:" << orderIdToString(m_order_id) <<
				" side:" << sideToString(m_side) <<
				" qty:" << qtyToString(m_qty) <<
				" price:" << priceToString(m_price) <<
				"]";
			return ss.str();
		}

	};

	struct OMClientRequest {
		size_t m_seq_num = 0;
		MEClientRequest m_me_client_request;

		auto toString() const {
			std::stringstream ss;
			ss << "OMClientRequest" <<
				" [" <<
				" seq:" << m_seq_num <<
				" :" << m_me_client_request.toString() <<
				"]";
			return ss.str();
		}
	};

#pragma pack(pop)

	typedef LFQueue<MEClientRequest> ClientRequestLFQueue;
}