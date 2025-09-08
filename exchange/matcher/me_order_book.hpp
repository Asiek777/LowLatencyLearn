#pragma once 

#include "common/types.hpp"
#include "common/mem_pool.hpp"
#include "common/logging.hpp"
#include "exchange/order_server/client_response.hpp"
#include "exchange/market_data/market_update.hpp"

#include "common/time_utils.hpp"
#include "exchange/matcher/me_order.hpp"

using namespace Common;

namespace Exchange {

	class MatchingEngine;

	class MEOrderBook final {

		TickerId m_ticker_id = TickerId_INVALID;

		MatchingEngine* m_matching_engine = nullptr;

		ClientOrderHashMap m_cid_oid_to_order;

		MemPool<MEOrderAtPrice> m_orders_at_price_pool;
		MEOrderAtPrice* m_bids_by_price = nullptr;
		MEOrderAtPrice* m_asks_by_price = nullptr;

		OrdersAtPriceHashMap m_price_orders_at_price;

		MemPool<MEOrder> m_order_pool;

		MEClientResponse m_client_response;
		MEMarketUpdate m_market_update;

		OrderId m_next_market_order_id = 1;

		std::string m_time_str;

		Logger* m_logger = nullptr;


		OrderId generateNewMarketOrderId() noexcept { return m_next_market_order_id++; }
		auto priceToIndex(Price price) const noexcept { return price % ME_MAX_PRICE_LEVELS; }
		MEOrderAtPrice* getOrdersAtPrice(Price price) const noexcept {
			return m_price_orders_at_price.at(priceToIndex(price));
		}
		Priority getNextPriority(Price price) noexcept;
		void addOrder(MEOrder* order) noexcept;
		void addOrdersAtPrice(MEOrderAtPrice* new_orders_at_price) noexcept;
		void removeOrder(MEOrder* order) noexcept;
		void removeOrdersAtPrice(Side side, Price price) noexcept;
		Qty checkForMatch(ClientId client_id, OrderId client_order_id, TickerId ticker_id,
			Side side, Price price, Qty qty, Qty new_market_order_id) noexcept;
		void match(TickerId ticker_id, ClientId client_id, Side side, OrderId client_order_id, 
			OrderId new_market_order_id, MEOrder* itr, Qty* leaves_qty) noexcept;


	public:
		MEOrderBook(TickerId ticker_id, Logger* logger, MatchingEngine* matching_engine);
		~MEOrderBook();
		MEOrderBook() = delete;
		MEOrderBook(const MEOrderBook&) = delete;
		MEOrderBook(const MEOrderBook&&) = delete;
		MEOrderBook& operator=(const MEOrderBook&) = delete;
		MEOrderBook& operator=(const MEOrderBook&&) = delete;

		void add(ClientId client_id, OrderId client_order_id,
			TickerId ticker_id, Side side, Price price, Qty qty) noexcept;

		void cancel(ClientId client_id, OrderId order_id, TickerId ticker_id) noexcept;
	};

	typedef std::array<MEOrderBook*, ME_MAX_TICKERS> OrderBookHashMap;
}