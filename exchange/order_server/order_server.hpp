#pragma once

#include <functional>
#include <array>

#include "common/thread_utils.hpp"
#include "common/macros.hpp"
#include "common/tcp_server.hpp"

#include "exchange/order_server/client_request.hpp"
#include "exchange/order_server/client_response.hpp"
#include "exchange/order_server/fifo_sequencer.hpp"


namespace Exchange {

	class OrderServer {
		
		const std::string m_iface;
		const int m_port = 0;

		ClientResponseLFQueue* m_outgoing_responses;

		volatile bool m_run = false;

		std::string m_time_str;
		Logger m_logger;

		std::array<size_t, ME_MAX_NUM_CLIENTS> m_cid_next_outgoing_seq_num;
		std::array<size_t, ME_MAX_NUM_CLIENTS> m_cid_next_exp_seq_num;
		std::array<Common::TCPSocket*, ME_MAX_NUM_CLIENTS> m_cid_tcp_socket;

		Common::TCPServer m_tcp_server;

		FIFOSequencer m_fifo_sequencer;

		void recvCallback(Common::TCPSocket* socket, Nanos rx_time) noexcept;
		void recvFinishedCallback() noexcept;

	public:
		OrderServer(ClientRequestLFQueue* client_requests,
			ClientResponseLFQueue* client_responses, const std::string& iface, int port);
		~OrderServer();

		void start();
		void stop();

		void run() noexcept;
	};
}