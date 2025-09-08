#pragma once

#include "common/lf_queue.hpp"
#include "common/macros.hpp"
#include "common/logging.hpp"
#include "common/time_utils.hpp"

#include "exchange/order_server/client_request.hpp"
#include "exchange/order_server/client_respons.hpp"
#include "exchange/market_data/market_update.hpp"

#include "exchange/matcher/me_order.hpp"
#include "exchange/matcher/me_order_book.hpp"


namespace Exchange {

	class MatchingEngine final {

		OrderBookHashMap m_ticker_order_book;

		ClientRequestLFQueue* m_incoming_requests = nullptr;
		ClientResponseLFQueue* m_outgoing_ogw_responses = nullptr;
		MEMarketUpdateLFQueue* m_outgoing_md_updates = nullptr;

		volatile bool m_run = false;

		std::string m_time_str;
		Common::Logger m_logger;

	public:
		MatchingEngine(ClientRequestLFQueue* client_requests,
			ClientResponseLFQueue* client_responses, MEMarketUpdateLFQueue* market_updates);
		~MatchingEngine();

		MatchingEngine() = delete;
		MatchingEngine(const MatchingEngine&) = delete;
		MatchingEngine(const MatchingEngine&&) = delete;
		MatchingEngine& operator=(const MatchingEngine&) = delete;
		MatchingEngine& operator=(const MatchingEngine&&) = delete;

		void start();
		void stop();

		void run() noexcept;

		void processClientRequest(const MEClientRequest* client_request) noexcept;
		void sendClientResponse(const MEClientResponse* client_response) noexcept;
		void sendMarketUpdate(const MEMarketUpdate* market_update) noexcept;
	};
}