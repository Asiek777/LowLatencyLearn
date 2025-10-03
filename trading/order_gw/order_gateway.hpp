#pragma once

#include <functional>
#include <cstring>

#include "common/thread_utils.hpp"
#include "common/macros.hpp"
#include "common/tcp_server.hpp"

#include "exchange/order_server/client_request.hpp"
#include "exchange/order_server/client_response.hpp"

namespace Trading {

	class OrderGateway {

		const ClientId m_client_id;

		std::string m_ip;
		const std::string m_iface;
		const int m_port = 0;

		Exchange::ClientRequestLFQueue* m_outgoing_requests = nullptr;
		Exchange::ClientResponseLFQueue* m_incoming_responses = nullptr;

		volatile bool m_run = false;

		std::string m_time_str;
		Logger m_logger;

		size_t m_next_outgoing_seq_num = 1;
		size_t m_next_exp_seq_num = 1;
		Common::TCPSocket m_tcp_socket;

		void run() noexcept;
		void recvCallback(Common::TCPSocket* socket, Nanos rx_time) noexcept;

	public:


		OrderGateway(const ClientId& client_id, Exchange::ClientRequestLFQueue* client_requests, 
			Exchange::ClientResponseLFQueue* client_responses, std::string ip, 
			const std::string& iface, int port);
		~OrderGateway();

		void start();
		void stop();
	};
}