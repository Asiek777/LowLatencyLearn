#include "exchange/market_data/market_data_publisher.hpp"
#include "common/time_utils.hpp"


namespace Exchange {

	MarketDataPublisher::MarketDataPublisher(MEMarketUpdateLFQueue* market_updates,
		const std::string& iface, const std::string& snapshot_ip, int snapshot_port,
		const std::string& incremental_ip, int incremental_port) :
		m_outgoing_md_updates(market_updates), m_snapshot_md_updates(ME_MAX_MARKET_UPDATES),
		m_run(false), m_logger("exchange_market_data_publisher.log"),
		m_incremental_socket(m_logger) {
		ASSERT(m_incremental_socket.init(incremental_ip, iface, incremental_port,
			false) >= 0, "Unable to create incremental mcast socket. error:" +
			std::string(std::strerror(errno)));
		m_snapshot_synthesizer = new SnapshotSynthesizer(&m_snapshot_md_updates,
			iface, snapshot_ip, snapshot_port);
	}

	MarketDataPublisher::~MarketDataPublisher() {
		stop();

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(5s);

		delete m_snapshot_synthesizer;
		m_snapshot_synthesizer = nullptr;
	}

	void MarketDataPublisher::start() {
		m_run = true;

		ASSERT(Common::createAndStartThread(-1, "Exchange/MarketDataPublisher",
			[this]() { run(); }) != nullptr, "Failed to start MarketData thread.");

		m_snapshot_synthesizer->start();
	}

	void MarketDataPublisher::stop() {
		m_run = false;
		m_snapshot_synthesizer->stop();
	}

	void MarketDataPublisher::run() noexcept {
		m_logger.log("%: % %() %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str));
		while (m_run) {
			for (auto market_update = m_outgoing_md_updates->getNextToRead();
				m_outgoing_md_updates->size() && market_update;
				market_update = m_outgoing_md_updates->getNextToRead()) {
				m_logger.log("%:% %() % Sending seq:% %\n", __FILE__, __LINE__, 
					__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), 
					m_next_inc_seq_num, market_update->toString().c_str());
				
				m_incremental_socket.send(&m_next_inc_seq_num, sizeof(m_next_inc_seq_num));
				m_incremental_socket.send(&market_update, sizeof(MEMarketUpdate));
				m_outgoing_md_updates->updateReadIndex();

				auto next_write = m_snapshot_md_updates.getNextToWriteTo();
				next_write->m_seq_num = m_next_inc_seq_num;
				next_write->m_me_market_update = *market_update;
				m_snapshot_md_updates.updateWriteIndex();

				m_next_inc_seq_num++;
			}

			m_incremental_socket.sendAndRecv();
		}
	}
}
