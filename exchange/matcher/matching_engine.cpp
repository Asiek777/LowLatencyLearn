#include "exchange/matcher/matching_engine.hpp"


namespace Exchange {

	MatchingEngine::MatchingEngine(ClientRequestLFQueue* client_requests,
		ClientResponseLFQueue* client_responses, MEMarketUpdateLFQueue* market_updates) :
		m_incoming_requests(client_requests), m_outgoing_ogw_responses(client_responses),
		m_outgoing_md_updates(market_updates), m_logger("exchange_matching_engine.log") {

		for (size_t i = 0; i < m_ticker_order_book.size(); i++) {
			m_ticker_order_book[i] = new MEOrderBook(i, &m_logger, this);
		}
	}

	MatchingEngine::~MatchingEngine() {
		m_run = false;

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(1s);

		m_incoming_requests = nullptr;
		m_outgoing_ogw_responses = nullptr;
		m_outgoing_md_updates = nullptr;

		for (auto& order_book : m_ticker_order_book) {
			delete order_book;
			order_book = nullptr;
		}
	}

	void MatchingEngine::start() {
		m_run = true;
		ASSERT(Common::createAndStartThread(-1, "Exchange/MatchingEngine",
			[this]() { run(); }) != nullptr, "Failed to start MatchingEngine thread.");
	}

	void MatchingEngine::stop() {
		m_run = false;
	}

	void MatchingEngine::run() noexcept {
		m_logger.log("%: % %() %\n", __FILE__, __LINE__,
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str));

		while (m_run) {
			const auto me_client_request = m_incoming_requests->getNextToRead();

			if (me_client_request) [[likely]] {
				m_logger.log("%:% %() % Processing %\n", __FILE__, __LINE__, __FUNCTION__,
					Common::getCurrentTimeStr(&m_time_str), me_client_request->toString());
				processClientRequest(me_client_request);
				m_incoming_requests->updateReadIndex();
			}
		}
	}

	void MatchingEngine::processClientRequest(const MEClientRequest* client_request) noexcept {
		auto order_book = m_ticker_order_book[client_request->m_ticker_id];

		switch (client_request->m_type)
		{
		case ClientRequestType::NEW:
			order_book->add(client_request->m_client_id, client_request->m_order_id,
				client_request->m_ticker_id, client_request->m_side,
				client_request->m_price, client_request->m_qty);
			break;
		case ClientRequestType::CANCEL:
			order_book->cancel(client_request->m_client_id, client_request->m_order_id,
				client_request->m_ticker_id);
		default:
			FATAL("Receive invalid client-request-type: " +
				clientRequestTypeToString(client_request->m_type));
			break;
		}
	}

	void MatchingEngine::sendClientResponse(const MEClientResponse* client_response) noexcept {
		m_logger.log("%:% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), client_response->toString());

		auto next_write = m_outgoing_ogw_responses->getNextToWriteTo();
		*next_write = std::move(*client_response);
		m_outgoing_ogw_responses->updateWriteIndex();
	}

	void MatchingEngine::sendMarketUpdate(const MEMarketUpdate* market_update) noexcept {
		m_logger.log("%:% %() % Sending %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), market_update->toString());

		auto next_write = m_outgoing_md_updates->getNextToWriteTo();
		*next_write = *market_update;
		m_outgoing_md_updates->updateWriteIndex();
	}


}
