#pragma once

#include <functional>
#include <cstring>

#include "common/logging.hpp"
#include "common/lf_queue.hpp"
#include "common/time_utils.hpp"
#include "common/mcast_socket.hpp"
#include "exchange/market_data/market_update.hpp"
#include "exchange/market_data/snapshot_sythesizer.hpp"

namespace Exchange {

	class MarketDataPublisher {

		size_t m_next_inc_seq_num = 1;
		MEMarketUpdateLFQueue* m_outgoing_md_updates = nullptr;

		MDPMarketUpdateLFQueue m_snapshot_md_updates;

		volatile bool m_run = false;

		std::string m_time_str;
		Common::Logger m_logger;

		Common::McastSocket m_incremental_socket;

		SnapshotSynthesizer* m_snapshot_synthesizer = nullptr;
		

		void run() noexcept;

	public:
		MarketDataPublisher(MEMarketUpdateLFQueue* market_updates, 
			const std::string& iface, const std::string& snapshot_ip, 
			int snapshot_port, const std::string& incremental_ip, int incremental_port);
		~MarketDataPublisher();

		void start();
		void stop();
	};
}
