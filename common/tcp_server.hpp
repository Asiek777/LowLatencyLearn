#pragma once       

#include <sys/epoll.h>

#include "common/tcp_socket.hpp"

namespace Common {

	struct TCPServer {

		int m_efd = -1;
		TCPSocket m_listener_socket;

		epoll_event m_events[1024];

		std::vector<TCPSocket*> m_sockets, m_receive_sockets,
			m_send_sockets, m_disconnected_sockets;

		std::function<void(TCPSocket* s, Nanos rx_time)> m_recv_callback;
		std::function<void()> m_recv_finished_callback;

		std::string m_time_str;
		Logger& m_logger;


		auto defaultRecvCallback(TCPSocket* socket, Nanos rx_time) noexcept {
			m_logger.log("%: % %() % TCPServer::defaultRecvCallback() socket: % len : % rx : % \n",
				__FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str),
				socket->m_fd, socket->m_next_recv_valid_index, rx_time);
		}

		auto defaultRecvFinishedCallback() noexcept {
			m_logger.log("%:% %() % TCPServer::defaultRecvFinishedCallback()\n",
				__FILE__, __LINE__, __FUNCTION__, 
				Common::getCurrentTimeStr(&m_time_str));
		}

		explicit TCPServer(Logger& logger) :
			m_listener_socket(logger), m_logger(logger) {
			m_recv_callback = [this](auto socket, auto rx_time) {
				defaultRecvCallback(socket, rx_time); };
			m_recv_finished_callback = [this]() {defaultRecvFinishedCallback(); };
		}

		TCPServer() = delete;
		TCPServer(const TCPServer&) = delete;
		TCPServer(const TCPServer&&) = delete;
		TCPServer& operator=(const TCPServer&) = delete;
		TCPServer& operator=(const TCPServer&&) = delete;

		void destroy();

		void listen(const std::string& iface, int port);

		auto epoll_add(TCPSocket* socket);
		auto epoll_del(TCPSocket* socket);
		auto del(TCPSocket* socket);

		void poll() noexcept;
		void sendAndRecv() noexcept;
	};
}