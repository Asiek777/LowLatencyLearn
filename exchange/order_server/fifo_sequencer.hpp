#pragma once 

#include <array>

#include "common/logging.hpp"
#include "common/macros.hpp"
#include "common/thread_utils.hpp"
#include "common/time_utils.hpp"

#include "exchange/order_server/client_request.hpp"

namespace Exchange {

	constexpr size_t ME_MAX_PENDING_REQUESTS = 1024;

	class FIFOSequencer {

		ClientRequestLFQueue* m_incoming_requests = nullptr;
		std::string m_time_str;
		Common::Logger* m_logger = nullptr;

		struct RecvTimeClientRequest {
			Nanos m_recv_time = 0;
			MEClientRequest m_request;
			auto operator<(const RecvTimeClientRequest& rhs) const {
				return m_recv_time < rhs.m_recv_time;
			}
		};

		std::array<RecvTimeClientRequest, ME_MAX_PENDING_REQUESTS> m_pending_client_requests;
		size_t m_pending_size = 0;

	public:
		FIFOSequencer(ClientRequestLFQueue* client_requests, Logger* logger) :
			m_incoming_requests(client_requests), m_logger(logger) {
		}

		auto addClientRequest(Nanos rx_time, const MEClientRequest& request) {
			if (m_pending_size >= m_pending_client_requests.size()) {
				FATAL("Too many pending requests");
			}
			m_pending_client_requests.at(m_pending_size++) =
				std::move(RecvTimeClientRequest{ rx_time, request });
		}

		auto sequenceAndPublish() {
			if (!m_pending_size) [[unlikely]]
				return;

			m_logger->log("%: % %() % Processing % requests.\n", __FILE__, __LINE__,
				__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), m_pending_size);

			std::sort(m_pending_client_requests.begin(),
				m_pending_client_requests.begin() + m_pending_size);

			for (size_t i = 0; i < m_pending_size; i++) {
				const auto& client_request = m_pending_client_requests.at(i);
				m_logger->log("%: % %() % Writing RX: % Req: % to FIFO.\n", __FILE__,
					__LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str), 
					client_request.m_recv_time, client_request.m_request.toString());

				auto next_write = m_incoming_requests->getNextToWriteTo();
				*next_write = std::move(client_request.m_request);
				m_incoming_requests->updateWriteIndex();
			}

			m_pending_size = 0;
		}
	};
}