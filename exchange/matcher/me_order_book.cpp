#include "me_order_book.hpp"
#include "me_order_book.hpp"
#include "me_order_book.hpp"
#include "me_order_book.hpp"
#include "me_order_book.hpp"
#include "me_order_book.hpp"
#include "me_order_book.hpp"
#include "exchange/matcher/matching_engine.hpp"

namespace Exchange {
	MEOrderBook::MEOrderBook(TickerId ticker_id, Logger* logger,
		MatchingEngine* matching_engine) :
		m_ticker_id(ticker_id), m_matching_engine(matching_engine), m_orders_at_price_pool
		(ME_MAX_PRICE_LEVELS), m_order_pool(ME_MAX_ORDER_IDS), m_logger(logger) {
	}

	MEOrderBook::~MEOrderBook() {
		m_logger->log("%:% %() % OrderBook\n%\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), toString(false, true));

		m_matching_engine = nullptr;
		m_bids_by_price = m_asks_by_price = nullptr;
		for (auto& itr : m_cid_oid_to_order) {
			itr.fill(nullptr);
		}
	}

	void MEOrderBook::add(ClientId client_id, OrderId client_order_id,
		TickerId ticker_id, Side side, Price price, Qty qty) noexcept {
		const auto new_market_order_id = generateNewMarketOrderId();
		m_client_response = { ClientResponseType::ACCEPTED, client_id,
			ticker_id, client_order_id, new_market_order_id, side, price, 0, qty };
		m_matching_engine->sendClientResponse(&m_client_response);

		const auto leaves_qty = checkForMatch(client_id, client_order_id,
			ticker_id, side, price, qty, new_market_order_id);
		if (leaves_qty) [[likely]] {
			const auto priority = getNextPriority(price);
			auto order = m_order_pool.allocate(ticker_id, client_id, client_order_id,
				new_market_order_id, side, price, leaves_qty, priority, nullptr, nullptr);
			addOrder(order);

			m_market_update = { MarketUpdateType::ADD, new_market_order_id,
				ticker_id, side, price, leaves_qty, priority };
			m_matching_engine->sendMarketUpdate(&m_market_update);
		}
	}

	void MEOrderBook::cancel(ClientId client_id, OrderId order_id,
		TickerId ticker_id) noexcept {
		auto is_cancelable = client_id < m_cid_oid_to_order.size();

		MEOrder* exchange_order = nullptr;
		if (is_cancelable) [[likely]] {
			auto& co_itr = m_cid_oid_to_order.at(client_id);
			exchange_order = co_itr.at(order_id);
			is_cancelable = exchange_order != nullptr;
		}
		if (!is_cancelable) [[unlikely]] {
			m_client_response = { ClientResponseType::CANCEL_REJECTED, client_id, ticker_id,
				order_id, OrderId_INVALID, Side::INVALID, Price_INVALID, Qty_INVALID, Qty_INVALID };
		}
		else {
			m_client_response = { ClientResponseType::CANCELED, client_id, ticker_id,
				order_id, exchange_order->m_market_order_id, exchange_order->m_side,
				exchange_order->m_price, Qty_INVALID, exchange_order->m_qty };
			m_market_update = { MarketUpdateType::CANCEL, exchange_order->m_market_order_id,
				ticker_id, exchange_order->m_side, exchange_order->m_price, 0,
				exchange_order->m_priority };

			removeOrder(exchange_order);
			m_matching_engine->sendMarketUpdate(&m_market_update);
		}

		m_matching_engine->sendClientResponse(&m_client_response);
	}

	Priority MEOrderBook::getNextPriority(Price price) noexcept {
		const auto orders_at_price = getOrdersAtPrice(price);
		if (!orders_at_price)
			return 1lu;

		return orders_at_price->m_first_me_order->m_prev_order->m_priority + 1;
	}

	void MEOrderBook::addOrder(MEOrder* order) noexcept {
		const auto orders_at_price = getOrdersAtPrice(order->m_price);
		if (!orders_at_price) {
			order->m_next_order = order->m_prev_order = order;

			auto new_orders_at_price = m_orders_at_price_pool.allocate(
				order->m_side, order->m_price, order, nullptr, nullptr);
			addOrdersAtPrice(new_orders_at_price);
		}
		else {
			auto first_order = orders_at_price ? orders_at_price->m_first_me_order : nullptr;

			first_order->m_prev_order->m_next_order = order;
			order->m_prev_order = first_order->m_prev_order;
			order->m_next_order = first_order;
			first_order->m_prev_order = order;
		}
		m_cid_oid_to_order.at(order->m_client_id).at(order->m_client_order_id) = order;
	}

	void MEOrderBook::addOrdersAtPrice(MEOrderAtPrice* new_orders_at_price) noexcept {
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

	void MEOrderBook::removeOrder(MEOrder* order) noexcept {
		auto orders_at_price = getOrdersAtPrice(order->m_price);

		if (order->m_prev_order == order) {
			removeOrdersAtPrice(order->m_side, order->m_price);
		}
		else {
			const auto order_before = order->m_prev_order;
			const auto order_after = order->m_next_order;
			order_before->m_next_order = order_after;
			order_after->m_prev_order = order_before;

			if (orders_at_price->m_first_me_order == order) {
				orders_at_price->m_first_me_order = order_after;
			}

			order->m_prev_order = order->m_next_order = nullptr;
		}

		m_cid_oid_to_order.at(order->m_client_id).at(order->m_client_order_id) = nullptr;
		m_order_pool.deallocate(order);
	}

	void MEOrderBook::removeOrdersAtPrice(Side side, Price price) noexcept {
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

	Qty MEOrderBook::checkForMatch(ClientId client_id, OrderId client_order_id,
		TickerId ticker_id, Side side, Price price, Qty qty, Qty new_market_order_id) noexcept {
		auto leaves_qty = qty;

		if (side == Side::BUY) {
			while (leaves_qty && m_asks_by_price) {
				const auto ask_itr = m_asks_by_price->m_first_me_order;
				if (price < ask_itr->m_price) [[likely]]
					break;

				match(ticker_id, client_id, side, client_order_id,
					new_market_order_id, ask_itr, &leaves_qty);
			}
		}
		if (side == Side::SELL) {
			while (leaves_qty && m_bids_by_price) {
				const auto bid_itr = m_bids_by_price->m_first_me_order;
				if (price < bid_itr->m_price) [[likely]]
					break;

				match(ticker_id, client_id, side, client_order_id,
					new_market_order_id, bid_itr, &leaves_qty);
			}
		}

		return leaves_qty;
	}
	void MEOrderBook::match(TickerId ticker_id, ClientId client_id,
		Side side, OrderId client_order_id, OrderId new_market_order_id,
		MEOrder* itr, Qty* leaves_qty) noexcept {

		const auto order = itr;
		const auto order_qty = order->m_qty;
		const auto fill_qty = std::min(*leaves_qty, order_qty);

		*leaves_qty -= fill_qty;
		order->m_qty -= fill_qty;

		m_client_response = { ClientResponseType::FILLED, client_id, ticker_id,
			client_order_id, new_market_order_id, side, itr->m_price, fill_qty, *leaves_qty };
		m_matching_engine->sendClientResponse(&m_client_response);

		m_client_response = { ClientResponseType::FILLED, order->m_client_id, ticker_id,
			order->m_client_order_id, order->m_market_order_id, order->m_side,
			itr->m_price, fill_qty, order->m_qty };
		m_matching_engine->sendClientResponse(&m_client_response);

		m_market_update = { MarketUpdateType::TRADE, OrderId_INVALID,
			ticker_id, side, itr->m_price, fill_qty, Priority_INVALID };
		m_matching_engine->sendMarketUpdate(&m_market_update);

		if (!order->m_qty) {
			m_market_update = { MarketUpdateType::CANCEL, order->m_market_order_id,
				ticker_id, order->m_side, order->m_price, order_qty, Priority_INVALID };
			m_matching_engine->sendMarketUpdate(&m_market_update);
			removeOrder(order);
		}
		else {
			m_market_update = { MarketUpdateType::MODIFY, order->m_market_order_id,
				ticker_id, order->m_side, order->m_price, order->m_qty, order->m_priority };
			m_matching_engine->sendMarketUpdate(&m_market_update);
		}
	}

	std::string MEOrderBook::toString(bool detailed, bool validity_check) const {
		std::stringstream ss;
		std::string time_str;

		auto printer = [&](std::stringstream& ss, MEOrderAtPrice* itr, Side side, Price& last_price, bool sanity_check) {
			char buf[4096];
			Qty qty = 0;
			size_t num_orders = 0;

			for (auto o_itr = itr->m_first_me_order;; o_itr = o_itr->m_next_order) {
				qty += o_itr->m_qty;
				++num_orders;
				if (o_itr->m_next_order == itr->m_first_me_order)
					break;
			}
			sprintf(buf, " <px:%3s p:%3s n:%3s> %-3s @ %-5s(%-4s)", 
				priceToString(itr->m_price).c_str(), priceToString(itr->m_prev_entry->m_price).c_str(), 
				priceToString(itr->m_next_entry->m_price).c_str(), priceToString(itr->m_price).c_str(), 
				qtyToString(qty).c_str(), std::to_string(num_orders).c_str());
			ss << buf;
			for (auto o_itr = itr->m_first_me_order;; o_itr = o_itr->m_next_order) {
				if (detailed) {
					sprintf(buf, "[oid:%s q:%s p:%s n:%s] ",
						orderIdToString(o_itr->m_market_order_id).c_str(), qtyToString(o_itr->m_qty).c_str(),
						orderIdToString(o_itr->m_prev_order ?
							o_itr->m_prev_order->m_market_order_id : OrderId_INVALID).c_str(),
						orderIdToString(o_itr->m_next_order ?
							o_itr->m_next_order->m_market_order_id : OrderId_INVALID).c_str());
					ss << buf;
				}
				if (o_itr->m_next_order == itr->m_first_me_order)
					break;
			}

			ss << std::endl;

			if (sanity_check) {
				if ((side == Side::SELL && last_price >= itr->m_price) ||
					(side == Side::BUY && last_price <= itr->m_price)) {
					FATAL("Bids/Asks not sorted by ascending/descending prices last:" + 
						priceToString(last_price) + " itr:" + itr->toString());
				}
				last_price = itr->m_price;
			}
		};

		ss << "Ticker:" << tickerIdToString(m_ticker_id) << std::endl;
		{
			auto ask_itr = m_asks_by_price;
			auto last_ask_price = std::numeric_limits<Price>::min();
			for (size_t count = 0; ask_itr; ++count) {
				ss << "ASKS L:" << count << " => ";
				auto next_ask_itr = (ask_itr->m_next_entry == m_asks_by_price ?
					nullptr : ask_itr->m_next_entry);
				printer(ss, ask_itr, Side::SELL, last_ask_price, validity_check);
				ask_itr = next_ask_itr;
			}
		}

		ss << std::endl << "                          X" << std::endl << std::endl;
		{
			auto bid_itr = m_bids_by_price;
			auto last_bid_price = std::numeric_limits<Price>::max();
			for (size_t count = 0; bid_itr; ++count) {
				ss << "BIDS L:" << count << " => ";
				auto next_bid_itr = (bid_itr->m_next_entry == m_bids_by_price ?
					nullptr : bid_itr->m_next_entry);
				printer(ss, bid_itr, Side::BUY, last_bid_price, validity_check);
				bid_itr = next_bid_itr;
			}
		}

		return ss.str();
	}
}

