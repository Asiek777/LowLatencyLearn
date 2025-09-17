#include "market_data_consumer.hpp"


namespace Trading {

	MarketDataConsumer::MarketDataConsumer(Common::ClientId client_id,
		Exchange::MEMarketUpdateLFQueue* market_updates, const std::string& iface,
		const std::string& snapshot_ip, int snapshot_port,
		const std::string& incremental_ip, int incremental_port) :
		m_incoming_md_updates(market_updates), m_run(false), m_logger(
			"trading_market_data_consumer_" + std::to_string(client_id) + ".log"),
		m_incremental_mcast_socket(m_logger), m_snapshot_mcast_socket(m_logger),
		m_iface(iface), m_snapshot_ip(snapshot_ip), m_snapshot_port(snapshot_port) {

		auto recv_callback = [this](auto socket) { recvCallback(socket); };

		m_incremental_mcast_socket.m_recv_callback = recv_callback;
		ASSERT(m_incremental_mcast_socket.init(
			incremental_ip, iface, incremental_port, /*is_listening*/ true) >= 0,
			"Unable to create incremental mcast socket. error:" +
			std::string(std::strerror(errno)));

		ASSERT(m_incremental_mcast_socket.join(incremental_ip), "Join failed on: " +
			std::to_string(m_incremental_mcast_socket.m_socket_fd) + " error: " +
			std::string(std::strerror(errno)));

		m_snapshot_mcast_socket.m_recv_callback = recv_callback;
	}

	MarketDataConsumer::~MarketDataConsumer() {
		stop();

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(5s);
	}

	void MarketDataConsumer::start() {
		m_run = true;

		ASSERT(Common::createAndStartThread(-1, "Trading/MarketDataConsumer",
			[this]() { run(); }) != nullptr, "Failed to start MarketData thread.");
	}
	void MarketDataConsumer::stop() {
		m_run = false;
	}

	void MarketDataConsumer::run() noexcept {
		m_logger.log("%: % %() %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str));

		while (m_run) {
			m_incremental_mcast_socket.sendAndRecv();
			m_snapshot_mcast_socket.sendAndRecv();
		}
	}

	void MarketDataConsumer::recvCallback(Common::McastSocket* socket) noexcept {
		const auto is_snapshot = (socket->m_socket_fd == m_snapshot_mcast_socket.m_socket_fd);

		if (is_snapshot && !m_in_recovery) [[unlikely]] {
			socket->m_next_recv_valid_index = 0;
			m_logger.log("%: % %() % WARN Not expecting snapshot messages.\n", __FILE__,
				__LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str));

			return;
		}

		if (socket->m_next_recv_valid_index >= sizeof(Exchange::MDPMarketUpdate)) {
			size_t i = 0;
			for (; i + sizeof(Exchange::MDPMarketUpdate) <= socket->m_next_recv_valid_index;
				i += sizeof(Exchange::MDPMarketUpdate)) {
				auto request = reinterpret_cast<const Exchange::MDPMarketUpdate*>(
					socket->m_inbound_data.data() + i);
				m_logger.log("%: % %() % Received % socket len: % %\n", __FILE__, __LINE__,
					__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
					(is_snapshot ? "snapshot" : "incremental"),
					sizeof(Exchange::MDPMarketUpdate), request->toString());

				const bool already_in_recovery = m_in_recovery;
				m_in_recovery = already_in_recovery ||
					(request->m_seq_num != m_next_exp_inc_seq_num);

				if (m_in_recovery) [[unlikely]] {
					if (!already_in_recovery) [[unlikely]] {
						m_logger.log("%: % %() % Packet drops on % socket."
							" SeqNum expected: % received: %\n", __FILE__, __LINE__,
							__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
							(is_snapshot ? "snapshot" : "incremental"),
							m_next_exp_inc_seq_num, request->m_seq_num);
						startSnapshotSync();
					}
					queueMessage(is_snapshot, request);
				}
				else if (!is_snapshot) {
					m_logger.log("%: % %() % %\n", __FILE__, __LINE__, __FUNCTION__,
						Common::getCurrentTimeStr(&m_time_str), request->toString());
					m_next_exp_inc_seq_num++;

					auto next_write = m_incoming_md_updates->getNextToWriteTo();
					*next_write = std::move(request->m_me_market_update);
					m_incoming_md_updates->updateWriteIndex();
				}
			}
			memcpy(socket->m_inbound_data.data(), socket->m_inbound_data.data() + i,
				socket->m_next_recv_valid_index - i);
			socket->m_next_recv_valid_index -= i;
		}

	}

	void MarketDataConsumer::startSnapshotSync() {
		m_snapshot_queued_msgs.clear();
		m_incremental_queued_msgs.clear();

		ASSERT(m_incremental_mcast_socket.init(
			m_snapshot_ip, m_iface, m_snapshot_port, /*is_listening*/ true) >= 0,
			"Unable to create snapshot mcast socket. error:" +
			std::string(std::strerror(errno)));

		ASSERT(m_incremental_mcast_socket.join(m_snapshot_ip), "Join failed on: " +
			std::to_string(m_incremental_mcast_socket.m_socket_fd) + " error: " +
			std::string(std::strerror(errno)));
	}

	void MarketDataConsumer::queueMessage(bool is_snapshot,
		const Exchange::MDPMarketUpdate* request) {
		if (is_snapshot) {
			if (m_snapshot_queued_msgs.find(request->m_seq_num) !=
				m_snapshot_queued_msgs.end()) {
				m_logger.log("%: % %() % Packet drops on snapshot socket."
					"Received for a 2nd time : % \n", __FILE__, __LINE__, __FUNCTION__,
					Common::getCurrentTimeStr(&m_time_str), request->toString());
				m_snapshot_queued_msgs.clear();
			}
			m_snapshot_queued_msgs[request->m_seq_num] = request->m_me_market_update;
		}
		else {
			m_incremental_queued_msgs[request->m_seq_num] = request->m_me_market_update;
		}
		m_logger.log("%: % %() % size snapshot: % incremental: % % => % \n", __FILE__,
			__LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
			m_snapshot_queued_msgs.size(), m_incremental_queued_msgs.size(),
			request->m_seq_num, request->toString());
		checkSnapshotSync();
	}

	void MarketDataConsumer::checkSnapshotSync() {
		if (m_snapshot_queued_msgs.empty()) {
			return;
		}

		const auto& first_snapshot_msg = m_snapshot_queued_msgs.begin()->second;
		if (first_snapshot_msg.m_type != Exchange::MarketUpdateType::SNAPSHOT_START) {
			m_logger.log("%: % %() % Returning because have not seen a"
				" SNAPSHOT_START yet.\n", __FILE__, __LINE__, __FUNCTION__,
				Common::getCurrentTimeStr(&m_time_str));
			m_snapshot_queued_msgs.clear();
			return;
		}

		std::vector<Exchange::MEMarketUpdate> final_events;
		auto have_complete_snapshot = true;
		size_t next_snapshot_seq = 0;
		for (auto& snapshot_itr : m_snapshot_queued_msgs) {
			m_logger.log("%: % %() % % => %\n", __FILE__, __LINE__, __FUNCTION__,
				Common::getCurrentTimeStr(&m_time_str), snapshot_itr.first,
				snapshot_itr.second.toString());

			if (snapshot_itr.first != next_snapshot_seq) {
				have_complete_snapshot = false;
				m_logger.log("%: % %() % Detected gap in snapshot stream expected:"
					" % found: % %.\n", __FILE__, __LINE__, __FUNCTION__,
					Common::getCurrentTimeStr(&m_time_str), next_snapshot_seq,
					snapshot_itr.first, snapshot_itr.second.toString());
				break;
			}

			if (snapshot_itr.second.m_type != Exchange::MarketUpdateType::SNAPSHOT_START &&
				snapshot_itr.second.m_type != Exchange::MarketUpdateType::SNAPSHOT_END) {
				final_events.push_back(snapshot_itr.second);
			}
			next_snapshot_seq++;
		}

		if (!have_complete_snapshot) {
			m_logger.log("%: % %() % Returning because found gaps in snapshot stream.\n",
				__FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str));
			m_snapshot_queued_msgs.clear();
			return;
		}

		const auto& last_snapshot_msg = m_snapshot_queued_msgs.rbegin()->second;
		if (last_snapshot_msg.m_type != Exchange::MarketUpdateType::SNAPSHOT_END) {
			m_logger.log("%: % %() % Returning because have not seen a SNAPSHOT_END yet.\n",
				__FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str));
			return;
		}

		auto have_complete_incremental = true;
		size_t num_incrementals = 0;
		m_next_exp_inc_seq_num = last_snapshot_msg.m_order_id + 1;

		for (auto inc_itr = m_incremental_queued_msgs.begin();
			inc_itr != m_incremental_queued_msgs.end(); ++inc_itr) {
			m_logger.log("%: % %() % Checking next_exp: % vs. seq: % % .\n", __FILE__,
				__LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
				m_next_exp_inc_seq_num, inc_itr->first, inc_itr->second.toString());

			if (inc_itr->first < m_next_exp_inc_seq_num)
				continue;

			if (inc_itr->first != m_next_exp_inc_seq_num) {
				m_logger.log("%: % %() % Detected gap in incremental stream expected:"
					" % found: % %.\n", __FILE__, __LINE__, __FUNCTION__,
					Common::getCurrentTimeStr(&m_time_str), m_next_exp_inc_seq_num,
					inc_itr->first, inc_itr->second.toString());

				have_complete_incremental = false;
				break;
			}
			m_logger.log("%: % %() % % => %\n", __FILE__, __LINE__, __FUNCTION__,
				Common::getCurrentTimeStr(&m_time_str), inc_itr->first,
				inc_itr->second.toString());

			if (inc_itr->second.m_type != Exchange::MarketUpdateType::SNAPSHOT_START &&
				inc_itr->second.m_type != Exchange::MarketUpdateType::SNAPSHOT_END) {
				final_events.push_back(inc_itr->second);

				m_next_exp_inc_seq_num++;
				num_incrementals++;
			}
		}

		if (!have_complete_incremental) {
			m_logger.log("%: % %() % Returning because have gaps in queued incrementals.\n",
				__FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str));
			m_snapshot_queued_msgs.clear();
			return;
		}

		for (const auto& itr : final_events) {
			auto next_write = m_incoming_md_updates->getNextToWriteTo();
			*next_write = itr;
			m_incoming_md_updates->updateWriteIndex();
		}

		m_logger.log("%: % %() % Recovered % snapshot and % incremental orders.\n", 
			__FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str), 
			m_snapshot_queued_msgs.size() - 2, num_incrementals);

		m_snapshot_queued_msgs.clear();
		m_incremental_queued_msgs.clear();
		m_in_recovery = false;

		m_snapshot_mcast_socket.leave(m_snapshot_ip, m_snapshot_port);
	}

}