#include "exchange/order_server/order_server.hpp"

namespace Exchange {

	OrderServer::OrderServer(ClientRequestLFQueue* client_requests,
		ClientResponseLFQueue* client_responses, const std::string& iface, int port) :
		m_iface(iface), m_port(port), m_outgoing_responses(client_responses),
		m_logger("exchange_order_server.log"), m_tcp_server(m_logger),
		m_fifo_sequencer(client_requests, &m_logger) {

		m_cid_next_outgoing_seq_num.fill(1);
		m_cid_next_exp_seq_num.fill(1);
		m_cid_tcp_socket.fill(nullptr);

		m_tcp_server.m_recv_callback = [this](auto socket, auto rx_time) {
			recvCallback(socket, rx_time);	};
		m_tcp_server.m_recv_finished_callback = [this]() {recvFinishedCallback(); };

	}

	OrderServer::~OrderServer() {
		stop();

		using namespace std::literals::chrono_literals;
		std::this_thread::sleep_for(1s);
	}

	void OrderServer::start() {
		m_run = true;
		m_tcp_server.listen(m_iface, m_port);

		ASSERT(Common::createAndStartThread(-1, "Exchange/OrderServer", [this]() {run(); }) !=
			nullptr, "Failed to start OrderServer thread.");
	}

	void OrderServer::stop() {
		m_run = false;
	}

	void OrderServer::run() noexcept {
		m_logger.log("%: % %() %\n", __FILE__, __LINE__,
			__FUNCTION__, Common::getCurrentTimeStr(&m_time_str));

		while (m_run) {
			m_tcp_server.poll();
			m_tcp_server.sendAndRecv();

			for (auto client_response = m_outgoing_responses->getNextToRead();
				m_outgoing_responses->size() && client_response;
				client_response = m_outgoing_responses->getNextToRead()) {
				auto& client_id = client_response->m_client_id;
				auto& next_outgoing_seq_num = m_cid_next_outgoing_seq_num[client_id];
				m_logger.log("%: % %() % Processing cid: % seq: % %\n", __FILE__,
					__LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
					client_id, next_outgoing_seq_num,
					client_response->toString());

				ASSERT(m_cid_tcp_socket[client_id] != nullptr,	
					"Don't have a TCPSocket for ClientId:" + std::to_string(client_id));

				m_cid_tcp_socket[client_id]->send(&next_outgoing_seq_num, sizeof(next_outgoing_seq_num));
				m_cid_tcp_socket[client_id]->send(client_response, sizeof(MEClientResponse));

				m_outgoing_responses->updateReadIndex();
				next_outgoing_seq_num++;
			}
		}
	}

	void OrderServer::recvCallback(Common::TCPSocket* socket, Nanos rx_time) noexcept {
		m_logger.log("%: % %() % Received socket: % len: % rx: %\n", __FILE__,
			__LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
			socket->m_fd, socket->m_next_recv_valid_index, rx_time);

		if (socket->m_next_recv_valid_index >= sizeof(OMClientRequest)) {
			size_t i = 0;
			for (; i + sizeof(OMClientRequest) <= socket->m_next_recv_valid_index;
				i += sizeof(OMClientRequest)) {
				auto request = reinterpret_cast<const OMClientRequest*> (socket->m_recv_buffer + i);
				m_logger.log("% :% %() % Received %\n", __FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str), request->toString());

				auto& client_id = request->m_me_client_request.m_client_id;
				if (m_cid_tcp_socket[client_id] == nullptr) [[unlikely]] {
					m_cid_tcp_socket[client_id] = socket;
				}

				if (m_cid_tcp_socket[client_id] != socket) {
					m_logger.log("%: % %() % Received ClientRequest from ClientId:"
						" % on different socket: % expected: % \n", __FILE__, __LINE__,
						__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), client_id,
						socket->m_fd, m_cid_tcp_socket[client_id]->m_fd);
					continue;
				}

				auto& next_exp_seq_num = m_cid_next_exp_seq_num[client_id];
				if (request->m_seq_num != next_exp_seq_num) {
					m_logger.log("%:% %() % Incorrect sequence number. ClientId: %"
						" SeqNum expected: % received: % \n", __FILE__, __LINE__,
						__FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
						request->m_me_client_request.m_client_id, next_exp_seq_num,
						request->m_seq_num);
					continue;
				}

				next_exp_seq_num++;

				m_fifo_sequencer.addClientRequest(rx_time, request->m_me_client_request);
			}
			memcpy(socket->m_recv_buffer, socket->m_recv_buffer + i,
				socket->m_next_recv_valid_index - i);
			socket->m_next_recv_valid_index -= i;
		}
	}

	void OrderServer::recvFinishedCallback() noexcept {
		m_fifo_sequencer.sequenceAndPublish();
	}
}