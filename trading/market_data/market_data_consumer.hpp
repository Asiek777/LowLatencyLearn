#pragma once

#include <functional>
#include <map>
#include <cstring>

#include "common/thread_utils.hpp"
#include "common/lf_queue.hpp"
#include "common/macros.hpp"
#include "common/mcast_socket.hpp"

#include "exchange/market_data/market_update.hpp"

namespace Trading {

	class MarketDataConsumer {

		size_t m_next_exp_inc_seq_num = 1;
		Exchange::MEMarketUpdateLFQueue* m_incoming_md_updates = nullptr;

		volatile bool m_run = false;

		std::string m_time_str;
		Logger m_logger;
		Common::McastSocket m_incremental_mcast_socket;
		Common::McastSocket m_snapshot_mcast_socket;

		bool m_in_recovery = true;
		const std::string m_iface, m_snapshot_ip;
		const int m_snapshot_port;

		typedef std::map<size_t, Exchange::MEMarketUpdate> QueuedMarketUpdates;
		QueuedMarketUpdates m_snapshot_queued_msgs;
		QueuedMarketUpdates m_incremental_queued_msgs;

		void run() noexcept;
		void recvCallback(Common::McastSocket* socket) noexcept;
		void startSnapshotSync();
		void queueMessage(bool is_snapshot, const Exchange::MDPMarketUpdate* request);
		void checkSnapshotSync();

	public:
		MarketDataConsumer(Common::ClientId client_id, Exchange::MEMarketUpdateLFQueue
			*market_updates, const std::string& iface, const std::string& snapshot_ip,
			int snapshot_port, const std::string& incremental_ip, int incremental_port);
		~MarketDataConsumer();


		void start();
		void stop();
	};
}