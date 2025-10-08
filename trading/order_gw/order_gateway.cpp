#include "order_gateway.hpp"

namespace Trading {

	OrderGateway::OrderGateway(const ClientId& client_id,
		Exchange::ClientRequestLFQueue* client_requests,
		Exchange::ClientResponseLFQueue* client_responses, std::string ip,
		const std::string& iface, int port) :
		m_client_id(client_id), m_ip(ip), m_iface(iface), m_port(port),
		m_outgoing_requests(client_requests), m_incoming_responses(client_responses),
		m_logger("trading_order_gateway_" + std::to_string(client_id) + ".log"),
		m_tcp_socket(m_logger) {

		m_tcp_socket.m_recv_callback = [this](auto socket, auto rx_time)
			{recvCallback(socket, rx_time); };
	}

	OrderGateway::~OrderGateway() {
		stop();

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(5s);
	}

	void OrderGateway::start() {
		m_run = true;
		ASSERT(m_tcp_socket.connect(m_ip, m_iface, m_port, false) >= 0,
			"Unable to connect to ip: " + m_ip + " port: " + std::to_string(m_port) +
			" on iface: " + m_iface + " error: " + std::string(std::strerror(errno)));
		ASSERT(Common::createAndStartThread(-1, "Trading/OrderGateway", [this]() {run(); })
			!= nullptr, "Failed to start OrderGateway thread.");
	}

	void OrderGateway::stop() {
		m_run = false;
	}

	void OrderGateway::run() noexcept {
		m_logger.log("%: % %() %\n", __FILE__, __LINE__,
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str));

		while (m_run) {
			m_tcp_socket.sendAndRecv();

			for (auto client_request = m_outgoing_requests->getNextToRead(); client_request;
				client_request = m_outgoing_requests->getNextToRead()) {
				m_logger.log("%: % %() % Sending cid:% seq:% %\n", __FILE__, __LINE__, __FUNCTION__,
					Common::getCurrentTimeStr(&m_time_str), m_client_id, m_next_outgoing_seq_num,
					client_request->toString());
				m_tcp_socket.send(&m_next_outgoing_seq_num, sizeof(m_next_exp_seq_num));
				m_tcp_socket.send(client_request, sizeof(Exchange::MEClientRequest));
				m_outgoing_requests->updateReadIndex();

				m_next_outgoing_seq_num++;
			}
		}
	}

	void OrderGateway::recvCallback(Common::TCPSocket* socket, Nanos rx_time) noexcept {
		m_logger.log("%:% %() % Received socket:% len:% %\n", __FILE__, __LINE__, __FUNCTION__,
			Common::getCurrentTimeStr(&m_time_str), socket->m_fd,
			socket->m_next_recv_valid_index, rx_time);

		if (socket->m_next_recv_valid_index >= sizeof(Exchange::OMClientResponse)) {
			size_t i = 0;
			for (; i + sizeof(Exchange::OMClientResponse) <= socket->m_next_recv_valid_index;
				i += sizeof(Exchange::OMClientResponse)) {
				auto response = reinterpret_cast<Exchange::OMClientResponse*>(
					socket->m_recv_buffer + i);
				m_logger.log("%:% %() % Received %\n", __FILE__, __LINE__, __FUNCTION__,
					Common::getCurrentTimeStr(&m_time_str), response->toString());

				if (response->m_me_client_response.m_client_id != m_client_id) {
					m_logger.log("%: % %() % ERROR Incorrect client id. ClientId expected:"
						" % received: % .\n", __FILE__, __LINE__, __FUNCTION__,
						Common::getCurrentTimeStr(&m_time_str), m_client_id,
						response->m_me_client_response.m_client_id);
					continue;
				}

				if (response->m_seq_num != m_next_exp_seq_num) {
					m_logger.log("%: % %() % ERROR Incorrect sequence number. ClientId: %."
						" SeqNum expected: % received: % .\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str), m_client_id, 
						m_next_exp_seq_num, response->m_seq_num);
					continue;
				}

				m_next_exp_seq_num++;

				auto next_write = m_incoming_responses->getNextToWriteTo();
				*next_write = std::move(response->m_me_client_response);
				m_incoming_responses->updateWriteIndex();
			}
			memcpy(socket->m_recv_buffer, socket->m_recv_buffer + i, socket->m_next_recv_valid_index - i);
			socket->m_next_recv_valid_index -= i;
		}
	}
}
