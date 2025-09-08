#include "exchange/matcher/me_order.hpp"

std::string Exchange::MEOrder::toString() const {
	std::stringstream ss;
	ss << "MEOrder" <<
		" [" <<
		" client:" << clientIdToString(m_client_id) <<
		" ticker:" << tickerIdToString(m_ticker_id) <<
		" coid:" << orderIdToString(m_client_order_id) <<
		" moid:" << orderIdToString(m_market_order_id) <<
		" side:" << sideToString(m_side) <<
		" exec_qty:" << qtyToString(m_qty) <<
		" price:" << priceToString(m_price) <<
		" priority:" << priorityToString(m_priority) <<
		" prev:" << orderIdToString(m_prev_order ? m_prev_order->m_market_order_id : OrderId_INVALID) <<
		" next:" << orderIdToString(m_next_order ? m_next_order->m_market_order_id : OrderId_INVALID) << 
		"]";
	return ss.str();
}
