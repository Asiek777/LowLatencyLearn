#include "market_order_book.hpp"

namespace Trading {

	MarketOrderBook::MarketOrderBook(TickerId ticker_id, Logger* logger) :
		m_ticker_id(ticker_id), m_orders_at_price_pool(ME_MAX_ORDER_IDS),
		m_order_pool(ME_MAX_ORDER_IDS), m_logger(logger) {
	}
	MarketOrderBook::~MarketOrderBook() {
		m_logger->log("%: % %() % OrderBook\n%\n", __FILE__, __LINE__,
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
			/*toString(false, true)*/ "TODO: implement MarketOrderBook toString()");
		m_trade_engine = nullptr;
		m_bids_by_price = m_asks_by_price = nullptr;
		m_oid_to_order.fill(nullptr);
	}

	void MarketOrderBook::onMarketUpdate(const Exchange::MEMarketUpdate* market_update) noexcept {
		const auto bid_updated = (m_bids_by_price && market_update->m_side == Side::BUY
			&& market_update->m_price >= m_bids_by_price->m_price);
		const auto ask_updated = (m_asks_by_price && market_update->m_side == Side::SELL
			&& market_update->m_price <= m_asks_by_price->m_price);

		switch (market_update->m_type)
		{
		case Exchange::MarketUpdateType::ADD:
		{
			auto order = m_order_pool.allocate(market_update->m_order_id, market_update->m_side,
				market_update->m_price, market_update->m_qty, market_update->m_priority,
				nullptr, nullptr);
			addOrder(order);
		}
		break;
		case Exchange::MarketUpdateType::MODIFY:
		{
			auto order = m_oid_to_order.at(market_update->m_order_id);
			order->m_qty = market_update->m_qty;
		}
		break;
		case Exchange::MarketUpdateType::CANCEL:
		{
			auto order = m_oid_to_order.at(market_update->m_order_id);
			removeOrder(order);
		}
		break;
		case Exchange::MarketUpdateType::TRADE:
		{
			m_trade_engine->onTradeUpdate(market_update, this);
			return;
		}
		case Exchange::MarketUpdateType::CLEAR:
		{
			for (auto& order : m_oid_to_order) {
				if (order)
					m_order_pool.deallocate(order);
			}
			m_oid_to_order.fill(nullptr);

			if (m_bids_by_price) {
				for (auto bid = m_bids_by_price->m_next_entry; bid != m_bids_by_price;
					bid = bid->m_next_entry) {
					m_orders_at_price_pool.deallocate(bid);
				}
				m_orders_at_price_pool.deallocate(m_bids_by_price);
			}
			if (m_asks_by_price) {
				for (auto ask = m_asks_by_price->m_next_entry; ask != m_asks_by_price;
					ask = ask->m_next_entry) {
					m_orders_at_price_pool.deallocate(ask);
				}
				m_orders_at_price_pool.deallocate(m_asks_by_price);
			}
			m_bids_by_price = m_asks_by_price = nullptr;
		}
		break;
		case Exchange::MarketUpdateType::INVALID:
		case Exchange::MarketUpdateType::SNAPSHOT_START:
		case Exchange::MarketUpdateType::SNAPSHOT_END:
		default:
			break;
		}

		updateBBO(bid_updated, ask_updated);

		m_trade_engine->onOrderBookUpdate(market_update->m_ticker_id,
			market_update->m_price, market_update->m_side);

		m_logger->log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str),
			/*toString(false, true)*/ "TODO: implement MarketOrderBook toString()");
	}

	unsigned long MarketOrderBook::priceToIndex(Price price) const noexcept {
		return price % ME_MAX_PRICE_LEVELS;
	}

	MarketOrdersAtPrice* MarketOrderBook::getOrdersAtPrice(Price price) const noexcept {
		return m_price_orders_at_price.at(priceToIndex(price));
	}

	void MarketOrderBook::addOrder(MarketOrder* order) noexcept {
		const auto orders_at_price = getOrdersAtPrice(order->m_price);
		if (!orders_at_price) {
			order->m_next_order = order->m_prev_order = order;

			auto new_orders_at_price = m_orders_at_price_pool.allocate(
				order->m_side, order->m_price, order, nullptr, nullptr);
			addOrdersAtPrice(new_orders_at_price);
		}
		else {
			auto first_order = orders_at_price ? orders_at_price->m_first_mkt_order : nullptr;

			first_order->m_prev_order->m_next_order = order;
			order->m_prev_order = first_order->m_prev_order;
			order->m_next_order = first_order;
			first_order->m_prev_order = order;
		}
		m_oid_to_order.at(order->m_order_id) = order;
	}

	void MarketOrderBook::addOrdersAtPrice(MarketOrdersAtPrice* new_orders_at_price) noexcept {
		m_price_orders_at_price.at(
			priceToIndex(new_orders_at_price->m_price)) = new_orders_at_price;

		const auto best_orders_by_price = (new_orders_at_price->m_side == Side::BUY ?
			m_bids_by_price : m_asks_by_price);

		if (!best_orders_by_price) [[unlikely]] {
			(new_orders_at_price->m_side == Side::BUY ?
				m_bids_by_price : m_asks_by_price) = new_orders_at_price;
			new_orders_at_price->m_prev_entry =
				new_orders_at_price->m_next_entry = new_orders_at_price;
		}
		else {
			auto target = best_orders_by_price;
			bool add_after = ((new_orders_at_price->m_side == Side::SELL &&
				new_orders_at_price->m_price > target->m_price) ||
				(new_orders_at_price->m_side == Side::BUY && new_orders_at_price->m_price));

			if (add_after) {
				target = target->m_next_entry;
				add_after = ((new_orders_at_price->m_side == Side::SELL &&
					new_orders_at_price->m_price > target->m_price) ||
					(new_orders_at_price->m_side == Side::BUY &&
						new_orders_at_price->m_price));
			}
			while (add_after && target != best_orders_by_price) {
				add_after = ((new_orders_at_price->m_side == Side::SELL &&
					new_orders_at_price->m_price > target->m_price) ||
					(new_orders_at_price->m_side == Side::BUY &&
						new_orders_at_price->m_price));
				if (add_after)
					target = target->m_next_entry;
			}

			if (add_after) {
				if (target == best_orders_by_price) {
					target = best_orders_by_price->m_prev_entry;
				}
				new_orders_at_price->m_prev_entry = target;
				target->m_next_entry->m_prev_entry = new_orders_at_price;
				new_orders_at_price->m_next_entry = target->m_next_entry;
				target->m_next_entry = new_orders_at_price;
			}
			else {
				new_orders_at_price->m_prev_entry = target->m_next_entry;
				new_orders_at_price->m_next_entry = target;
				target->m_prev_entry->m_next_entry = new_orders_at_price;
				if ((new_orders_at_price->m_side == Side::BUY &&
					new_orders_at_price->m_price > best_orders_by_price->m_price) ||
					(new_orders_at_price->m_side == Side::SELL &&
						new_orders_at_price->m_price < best_orders_by_price->m_price)) {
					target->m_next_entry = (target->m_next_entry == best_orders_by_price ?
						new_orders_at_price : target->m_next_entry);
					(new_orders_at_price->m_side == Side::BUY ?
						m_bids_by_price : m_asks_by_price) = new_orders_at_price;
				}
			}
		}
	}

	void MarketOrderBook::removeOrder(MarketOrder* order) noexcept {
		auto orders_at_price = getOrdersAtPrice(order->m_price);

		if (order->m_prev_order == order) {
			removeOrdersAtPrice(order->m_side, order->m_price);
		}
		else {
			const auto order_before = order->m_prev_order;
			const auto order_after = order->m_next_order;
			order_before->m_next_order = order_after;
			order_after->m_prev_order = order_before;

			if (orders_at_price->m_first_mkt_order == order) {
				orders_at_price->m_first_mkt_order = order_after;
			}

			order->m_prev_order = order->m_next_order = nullptr;
		}

		m_oid_to_order.at(order->m_order_id) = nullptr;
		m_order_pool.deallocate(order);
	}

	void MarketOrderBook::removeOrdersAtPrice(Side side, Price price) noexcept {
		const auto best_orders_by_price = (side == Side::BUY ?
			m_bids_by_price : m_asks_by_price);
		auto orders_at_price = getOrdersAtPrice(price);

		if (orders_at_price->m_next_entry == orders_at_price) [[unlikely]] {
			(side == Side::BUY ? m_bids_by_price : m_asks_by_price) = nullptr;
		}
		else {
			orders_at_price->m_prev_entry->m_next_entry = orders_at_price->m_next_entry;
			orders_at_price->m_next_entry->m_prev_entry = orders_at_price->m_prev_entry;

			if (orders_at_price == best_orders_by_price) {
				(side == Side::BUY ? m_bids_by_price : m_asks_by_price) =
					orders_at_price->m_next_entry;
			}

			orders_at_price->m_prev_entry = orders_at_price->m_next_entry = nullptr;
		}

		m_price_orders_at_price.at(priceToIndex(price)) = nullptr;

		m_orders_at_price_pool.deallocate(orders_at_price);
	}

	void MarketOrderBook::updateBBO(bool update_bid, bool update_ask) noexcept {
		if (update_bid) {
			if (m_bids_by_price) {
				m_bbo.m_bid_price = m_bids_by_price->m_price;
				m_bbo.m_bid_qty = m_bids_by_price->m_first_mkt_order->m_qty;
				for (auto order = m_bids_by_price->m_first_mkt_order->m_next_order;
					order != m_bids_by_price->m_first_mkt_order; order = order->m_next_order) {
					m_bbo.m_bid_qty += order->m_qty;
				}
			}
			else {
				m_bbo.m_bid_price = Price_INVALID;
				m_bbo.m_bid_qty = Qty_INVALID;
			}
		}
		if (update_ask) {
			if (m_asks_by_price) {
				m_bbo.m_ask_price = m_asks_by_price->m_price;
				m_bbo.m_ask_qty = m_asks_by_price->m_first_mkt_order->m_qty;
				for (auto order = m_asks_by_price->m_first_mkt_order->m_next_order;
					order != m_asks_by_price->m_first_mkt_order; order = order->m_next_order) {
					m_bbo.m_ask_qty += order->m_qty;
				}
			}
			else {
				m_bbo.m_ask_price = Price_INVALID;
				m_bbo.m_ask_qty = Qty_INVALID;
			}
		}

	}

}