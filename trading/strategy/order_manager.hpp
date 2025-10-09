#pragma once 

#include "common/macros.hpp"
#include "common/logging.hpp"
#include "common/time_utils.hpp"

#include "exchange/order_server/client_response.hpp"
#include "exchange/order_server/client_request.hpp"

#include "trading/strategy/om_order.hpp"
#include "trading/strategy/risk_manager.hpp"

using namespace Common;

namespace Trading {

	class TradeEngine;

	class OrderManager {
		
		TradeEngine* m_trade_engine = nullptr;
		const RiskManager& m_risk_manager;

		std::string m_time_str;
		Common::Logger* m_logger;

		OMOrderTickerSideHashMap m_ticker_side_order;
		OrderId m_next_order_id = 1;

		OMOrderSideHashMap* getOMOrderSideHashMap(TickerId ticker_id) const;
		void moveOrder(OMOrder* order, TickerId ticker_id, 
			Price price, Side side, Qty qty) noexcept;

	public: 
		OrderManager(Common::Logger* logger, TradeEngine* trade_engine, RiskManager& risk_manager);
		void onOrderUpdate(const Exchange::MEClientResponse* client_response) noexcept;

		void moveOrders(TickerId ticker_id, Price bid_price, Price ask_price, Qty clip) noexcept;
		void OrderManager::newOrder(OMOrder* order, TickerId ticker_id, 
			Price price, Side side, Qty qty) noexcept;
		void cancelOrder(OMOrder* order) noexcept;
	};
}