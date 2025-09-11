#include "exchange/market_data/snapshot_sythesizer.hpp"

namespace Exchange {

	SnapshotSynthesizer::SnapshotSynthesizer(MDPMarketUpdateLFQueue* market_updates,
		const std::string& iface, const std::string& snapshot_ip, int snapshot_port) :
		m_snapshot_md_updates(market_updates), m_logger("exchange_snapshot_synthesizer.log"),
		m_snapshot_socket(m_logger), m_order_pool(ME_MAX_ORDER_IDS) {

		ASSERT(m_snapshot_socket.init(snapshot_ip, iface, snapshot_port, false) >= 0,
			"Unable to create snapshot mcast socket. error:" +
			std::string(std::strerror(errno)));
	}
	SnapshotSynthesizer::~SnapshotSynthesizer() {
		stop();
	}

	void SnapshotSynthesizer::start() {
		m_run = true;
		ASSERT(Common::createAndStartThread(-1, "Exchange/SnapshotSynthesizer",
			[this]() { run(); }) != nullptr, "Failed to start SnapshotSynthesizer thread.");
	}

	void SnapshotSynthesizer::stop() {
		m_run = false;
	}

	void SnapshotSynthesizer::run() {
		m_logger.log("%:% %() %\n", __FILE__, __LINE__,
			__FUNCTION__, getCurrentTimeStr(&m_time_str));

		while (m_run) {
			for (auto market_update = m_snapshot_md_updates->getNextToRead();
				m_snapshot_md_updates->size() && market_update;
				market_update = m_snapshot_md_updates->getNextToRead()) {
				m_logger.log("%: % %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__, 
					getCurrentTimeStr(&m_time_str), market_update->toString().c_str());

				addToSnapshot(market_update);

				m_snapshot_md_updates->updateReadIndex();
			}

			if (getCurrentNanos() - m_last_snapshot_time > 60 * NANOS_TO_SECS) {
				m_last_snapshot_time = getCurrentNanos();
				publishSnapshot();
			}
		}
	}

	void SnapshotSynthesizer::addToSnapshot(const MDPMarketUpdate* market_update) {
		const auto& me_market_update = market_update->m_me_market_update;
		auto* orders = &m_ticker_orders.at(me_market_update.m_ticker_id);

		switch (me_market_update.m_type)
		{
		case MarketUpdateType::ADD: {
			auto order = orders->at(me_market_update.m_order_id);
			ASSERT(order == nullptr, "Received: " + me_market_update.toString() +
				" but order already exists: " + (order ? order->toString() : ""));
			orders->at(me_market_update.m_ticker_id) = m_order_pool.allocate(me_market_update);
		}
			break;

		case MarketUpdateType::MODIFY: {
			auto order = orders->at(me_market_update.m_order_id);
			ASSERT(order != nullptr, "Received: " + me_market_update.toString() +
				" but order does not exist.");
			ASSERT(order->m_order_id == me_market_update.m_order_id,
				"Expecting existing order to match new one.");
			ASSERT(order->m_side == me_market_update.m_side,
				"Expecting existing order to match new one.");
			order->m_qty = me_market_update.m_qty;
			order->m_price = me_market_update.m_price;
		}
			break;

		case MarketUpdateType::CANCEL: {
			auto order = orders->at(me_market_update.m_order_id);
			ASSERT(order != nullptr, "Received: " + me_market_update.toString() +
				" but order does not exist.");
			ASSERT(order->m_order_id == me_market_update.m_order_id,
				"Expecting existing order to match new one.");
			ASSERT(order->m_side == me_market_update.m_side,
				"Expecting existing order to match new one.");
			m_order_pool.deallocate(order);
			orders->at(me_market_update.m_order_id) = nullptr;
		}
			break;

		case MarketUpdateType::SNAPSHOT_START:
		case MarketUpdateType::SNAPSHOT_END:
		case MarketUpdateType::CLEAR:
		case MarketUpdateType::TRADE:
		case MarketUpdateType::INVALID:
		default:
			break;
		}

		ASSERT(market_update->m_seq_num == m_last_inc_seq_num + 1,
			"Expected incremental seq_nums to increase.");
		m_last_inc_seq_num = market_update->m_seq_num;
	}

	void SnapshotSynthesizer::publishSnapshot() {
		size_t snapshot_size = 0;

		const MDPMarketUpdate start_market_update{ snapshot_size++,
			{MarketUpdateType::SNAPSHOT_START, m_last_inc_seq_num} };
		m_logger.log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			getCurrentTimeStr(&m_time_str), start_market_update.toString());
		m_snapshot_socket.send(&start_market_update, sizeof(MDPMarketUpdate));

		for (size_t ticker_id = 0; ticker_id < m_ticker_orders.size(); ticker_id++) {
			const auto& orders = m_ticker_orders.at(ticker_id);

			MEMarketUpdate me_market_update;
			me_market_update.m_type = MarketUpdateType::CLEAR;
			me_market_update.m_ticker_id = ticker_id;

			const MDPMarketUpdate clear_market_update{ snapshot_size++, me_market_update };
			m_logger.log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
				getCurrentTimeStr(&m_time_str), clear_market_update.toString());
			m_snapshot_socket.send(&clear_market_update, sizeof(MDPMarketUpdate));

			for (const auto order : orders) {
				if (order) {
					const MDPMarketUpdate market_update{ snapshot_size++, *order };
					m_logger.log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
						getCurrentTimeStr(&m_time_str), market_update.toString());
					m_snapshot_socket.send(&market_update, sizeof(MDPMarketUpdate));
					m_snapshot_socket.sendAndRecv();
				}
			}
		}

		const MDPMarketUpdate end_market_update{ snapshot_size++,
		{MarketUpdateType::SNAPSHOT_END, m_last_inc_seq_num} };
		m_logger.log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
			getCurrentTimeStr(&m_time_str), end_market_update.toString());
		m_snapshot_socket.send(&end_market_update, sizeof(MDPMarketUpdate));
		m_snapshot_socket.sendAndRecv();

		m_logger.log("%:% %() % Published snapshot of % orders.\n", __FILE__,
			__LINE__, __FUNCTION__, getCurrentTimeStr(&m_time_str), snapshot_size - 1);
	}
}
