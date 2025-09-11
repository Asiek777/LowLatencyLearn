#pragma once

#include "common/types.hpp"
#include "common/thread_utils.hpp"
#include "common/time_utils.hpp"
#include "common/macros.hpp"
#include "common/lf_queue.hpp"
#include "common/mem_pool.hpp"
#include "common/logging.hpp"
#include "common/mcast_socket.hpp"

#include "exchange/market_data/market_update.hpp"
#include "exchange/matcher/me_order.hpp"

#include <cstring>

using namespace Common;

namespace Exchange {

	class SnapshotSynthesizer {

		MDPMarketUpdateLFQueue* m_snapshot_md_updates = nullptr;

		Logger m_logger;
		volatile bool m_run;
		std::string m_time_str;

		McastSocket m_snapshot_socket;

		std::array<std::array<MEMarketUpdate*, ME_MAX_ORDER_IDS>, ME_MAX_TICKERS> 
			m_ticker_orders;
		size_t m_last_inc_seq_num = 0;
		Nanos m_last_snapshot_time = 0;

		MemPool<MEMarketUpdate> m_order_pool;

		void run();
		void addToSnapshot(const MDPMarketUpdate* market_update);
		void publishSnapshot();

	public:
		SnapshotSynthesizer(MDPMarketUpdateLFQueue* market_updates, 
			const std::string& iface, const std::string& snapshot_ip, int snapshot_port);
		~SnapshotSynthesizer();

		void start();
		void stop();
	};
}