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
		Qty m_qty = Qty_INVALID;
		Priority m_priority = Priority_INVALID;

		MEOrder* m_prev_order = nullptr;
		MEOrder* m_next_order = nullptr;

		MEOrder() {}

		MEOrder(ClientId client_id, TickerId ticker_id, OrderId client_order_id,
			OrderId market_order_id, Side side, Price price, Qty exec_qty,
			Priority priority, MEOrder* prev_order, MEOrder* next_order) :
			m_client_id(client_id), m_ticker_id(ticker_id), m_client_order_id(client_order_id),
			m_market_order_id(market_order_id), m_side(side), m_price(price), m_qty(exec_qty),
			m_priority(priority), m_prev_order(prev_order), m_next_order(next_order) {
		}

		std::string toString() const;
	};

	typedef std::array<MEOrder*, ME_MAX_ORDER_IDS> OrderHashMap;
	typedef std::array <OrderHashMap, ME_MAX_NUM_CLIENTS> ClientOrderHashMap;


	struct MEOrderAtPrice {

		Side m_side = Side::INVALID;
		Price m_price = Price_INVALID;

		MEOrder* m_first_me_order = nullptr;

		MEOrderAtPrice* m_prev_entry = nullptr;
		MEOrderAtPrice* m_next_entry = nullptr;


		MEOrderAtPrice() = default;

		MEOrderAtPrice(Side side, Price price, MEOrder* first_me_order,
			MEOrderAtPrice* prev_entry, MEOrderAtPrice* next_entry) :
			m_side(side), m_price(price), m_first_me_order(first_me_order),
			m_prev_entry(prev_entry), m_next_entry(next_entry) {
		}

		auto toString() const {
			std::stringstream ss;
			ss << "MEOrdersAtPrice["
				<< "side:" << sideToString(m_side) << " "
				<< "price:" << priceToString(m_price) << " "
				<< "first_me_order:" << (m_first_me_order ? m_first_me_order->toString() : "null") << " "
				<< "prev:" << priceToString(m_prev_entry ? m_prev_entry->m_price : Price_INVALID) << " "
				<< "next:" << priceToString(m_next_entry ? m_next_entry->m_price : Price_INVALID) 
				<< "]";
			return ss.str();
		}

	};

	typedef std::array<MEOrderAtPrice*, ME_MAX_PRICE_LEVELS> OrdersAtPriceHashMap;
}
