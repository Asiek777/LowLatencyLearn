#pragma once

#include <array>
#include <sstream>

#include "common/types.hpp"

using namespace Common;

namespace Exchange {

	struct MEOrder {

		ClientId m_client_id = ClientId_INVALID;
		TickerId m_ticker_id = TickerId_INVALID;
		OrderId m_client_order_id = OrderId_INVALID;
		OrderId m_market_order_id = OrderId_INVALID;
		Side m_side = Side::INVALID;
		Price m_price = Price_INVALID;
		Qty m_exec_qty = Qty_INVALID;
		Priority m_priority = Priority_INVALID;

		MEOrder* m_prev_order = nullptr;
		MEOrder* m_next_order = nullptr;


		MEOrder(ClientId client_id, TickerId ticker_id, OrderId client_order_id,
			OrderId market_order_id, Side side, Price price, Qty exec_qty,
			Priority priority, MEOrder* prev_order, MEOrder* next_order) :
			m_client_id(client_id), m_ticker_id(ticker_id), m_client_order_id(client_order_id),
			m_market_order_id(market_order_id), m_side(side), m_price(price), m_exec_qty(exec_qty),
			m_priority(priority), m_prev_order(prev_order), m_next_order(next_order) {
		}

		std::string toString() const;
	};

	typedef std::array<MEOrder*, ME_MAX_ORDER_IDS> OrderHashMap;
	typedef std::array < OrderHashMap, ME_MAX_NUM_CLIENTS> ClientOrderHashMap;

}
