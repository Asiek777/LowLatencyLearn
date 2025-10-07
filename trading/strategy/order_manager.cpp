#include "order_manager.hpp"

namespace Trading {

	OrderManager::OrderManager(Common::Logger* logger,
		TradeEngine* trade_engine, RiskManager& risk_manager) :
		m_trade_engine(trade_engine), m_risk_manager(risk_manager), m_logger(logger) {
	}

	void OrderManager::onOrderUpdate(const Exchange::MEClientResponse* client_response)
		noexcept {
		m_logger->log("%:% %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), client_response->toString().c_str());

		auto order = &(m_ticker_side_order.at(client_response->m_ticker_id).
			at(sideToIndex(client_response->m_side)));

		m_logger->log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), order->toString().c_str());

		switch (client_response->m_type) {
		case Exchange::ClientResponseType::ACCEPTED:
		{
			order->m_order_state = OMOrderState::LIVE;
		}
		break;
		case Exchange::ClientResponseType::CANCELED:
		{
			order->m_order_state = OMOrderState::DEAD;
		}
		break;
		case Exchange::ClientResponseType::FILLED:
		{
			order->m_qty = client_response->m_leaves_qty;
			if(!order->m_qty)
				order->m_order_state = OMOrderState::DEAD;
		}
		break;
		case Exchange::ClientResponseType::CANCEL_REJECTED:
		case Exchange::ClientResponseType::INVALID:
			break;
		}
	}

	OMOrderSideHashMap* OrderManager::getOMOrderSideHashMap(TickerId ticker_id) const {
		return &(m_ticker_side_order.at(ticker_id));
	}

	void OrderManager::newOrder(OMOrder* order, TickerId ticker_id,
		Price price, Side side, Qty qty) noexcept {
		const Exchange::MEClientRequest new_request{ Exchange::ClientRequestType::NEW,
			m_trade_engine->clientId(), ticker_id, m_next_order_id, side, price, qty };
		m_trade_engine->sendClientRequest(&new_request);

		*order = { ticker_id, m_next_order_id, side, price, qty,
			OMOrderState::PENDING_NEW };
		m_next_order_id++;

		m_logger->log("%: % %() % Sent new order % for %\n", __FILE__, __LINE__,
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
			new_request.toString().c_str(), order->toString().c_str());
	}

	void OrderManager::cancelOrder(OMOrder* order) noexcept {
		const Exchange::MEClientRequest cancel_request{ Exchange::ClientRequestType::CANCEL,
			m_trade_engine->clientId(), order->ticker_id, order->m_order_id, order->m_side, order->m_price, order->m_qty };
		m_trade_engine->sendClientRequest(&cancel_request);

		order->m_order_state = OMOrderState::PENDING_CANCEL;

		m_logger->log("%: % %() % Sent cancel order % for %\n", __FILE__, __LINE__,
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
			cancel_request.toString().c_str(), order->toString().c_str());
	}
	void OrderManager::moveOrder(OMOrder* order, TickerId ticker_id,
		Price price, Side side, Qty qty) noexcept {
		switch (order->m_order_state) {
		case OMOrderState::LIVE:
		{
			if (order->m_price != price || order->m_qty != qty)
				cancelOrder(order);
		}
		break;
		case OMOrderState::INVALID:
		case OMOrderState::DEAD:
		{
			if (price != Price_INVALID) [[likely]] {
				const auto risk_result = m_risk_manager.checkPreTradeRisk(ticker_id, side, qty);
				if (risk_result == RiskCheckResult::ALLOWED) [[likely]] {
					newOrder(order, ticker_id, price, side, qty);
				}
				else {
					m_logger->log("%: % %() % Ticker: % Side: % Qty: % RiskCheckResult: % \n",
						__FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
						tickerIdToString(ticker_id), sideToString(side), qtyToString(qty),
						riskCheckResultToString(risk_result));
				}
			}
		}
		break;
		case OMOrderState::PENDING_NEW:
		case OMOrderState::PENDING_CANCEL:
			break;

		default:
			break;
		}
	}

	void OrderManager::moveOrders(TickerId ticker_id, Price bid_price,
		Price ask_price, Qty clip) noexcept {

		auto bid_order = &(m_ticker_side_order.at(ticker_id).at(sideToIndex(Side::BUY)));
		moveOrder(bid_order, ticker_id, bid_price, Side::BUY, clip);
		auto ask_order = &(m_ticker_side_order.at(ticker_id).at(sideToIndex(Side::SELL)));
		moveOrder(ask_order, ticker_id, bid_price, Side::SELL, clip);
	}
}
