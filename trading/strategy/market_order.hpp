#pragma once

#include <array>
#include <sstream>
#include "common/types.hpp"

using namespace Common;

namespace Trading {

	struct MarketOrder {
		OrderId m_order_id = OrderId_INVALID;
		Side m_side = Side::INVALID;
		Price m_price = Price_INVALID;
		Qty m_qty = Qty_INVALID;
		Priority m_priority = Priority_INVALID;

		MarketOrder* m_prev_order = nullptr;
		MarketOrder* m_next_order = nullptr;

		MarketOrder() = default;

		MarketOrder(const OrderId& order_id, const Side& side, const Price& price, const Qty& qty,
			const Priority& priority, MarketOrder* prev_order, MarketOrder* next_order)
			: m_order_id(order_id), m_side(side), m_price(price), m_qty(qty), m_priority(priority),
			m_prev_order(prev_order), m_next_order(next_order) {
		}

		std::string toString() const {
			std::stringstream ss;
			ss << "Market Order" <<
				" [" <<
				" oid:" << orderIdToString(m_order_id) <<
				" side:" << sideToString(m_side) <<
				" qty:" << qtyToString(m_qty) <<
				" price:" << priceToString(m_price) <<
				" priority:" << priorityToString(m_priority) <<
				" prev:" << orderIdToString(m_prev_order ? m_prev_order->m_order_id : OrderId_INVALID) <<
				" next:" << orderIdToString(m_next_order ? m_next_order->m_order_id : OrderId_INVALID) <<
				"]";
			return ss.str();
		}
	};

	typedef std::array<MarketOrder*, ME_MAX_ORDER_IDS> OrderHashMap;

	struct MarketOrdersAtPrice {
		Side m_side = Side::INVALID;
		Price m_price = Price_INVALID;

		MarketOrder* m_first_mkt_order = nullptr;

		MarketOrdersAtPrice* m_prev_entry = nullptr;
		MarketOrdersAtPrice* m_next_entry = nullptr;

		MarketOrdersAtPrice() = default;

		MarketOrdersAtPrice(const Side& side, const Price& price, MarketOrder* first_mkt_order,
			MarketOrdersAtPrice* prev_entry, MarketOrdersAtPrice* next_entry) :
			m_side(side), m_price(price), m_first_mkt_order(first_mkt_order),
			m_prev_entry(prev_entry), m_next_entry(next_entry) {
		}

		auto toString() const {
			std::stringstream ss;
			ss << "MEOrdersAtPrice["
				<< "side:" << sideToString(m_side) << " "
				<< "price:" << priceToString(m_price) << " "
				<< "first_me_order:" << (m_first_mkt_order ? m_first_mkt_order->toString() : "null") << " "
				<< "prev:" << priceToString(m_prev_entry ? m_prev_entry->m_price : Price_INVALID) << " "
				<< "next:" << priceToString(m_next_entry ? m_next_entry->m_price : Price_INVALID)
				<< "]";
			return ss.str();
		}
	};

	typedef std::array<MarketOrdersAtPrice*, ME_MAX_PRICE_LEVELS> ORdersAtPriceHashMap;

	struct BBO {
		Price m_bid_price = Price_INVALID, m_ask_price = Price_INVALID;
		Qty m_bid_qty = Qty_INVALID, m_ask_qty = Qty_INVALID;

		auto toString() const {
			std::stringstream ss;
			ss << "BBO{" <<
				qtyToString(m_bid_qty) << "@" << priceToString(m_bid_price) << "X" <<
				qtyToString(m_ask_qty) << "@" << priceToString(m_ask_price) <<
				"}";
			return ss.str();
		}
	};

}