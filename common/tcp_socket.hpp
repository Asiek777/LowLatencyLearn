#pragma once

#include <functional>

#include "common/socket_utils.hpp"
#include "common/logging.hpp"

namespace Common {

	constexpr size_t TCPBufferSize = 64 * 1024 * 1024;

	struct TCPSocket {
		int m_fd = 01;

		char* m_send_buffer = nullptr;
		size_t m_next_send_valid_index = 0;
		char* m_recv_buffer = nullptr;
		size_t m_next_recv_valid_index = 0;

		bool m_send_disconnected = false;
		bool m_recv_disconnected = false;

		struct sockaddr_in inInAddr;

		std::function<void(TCPSocket* s, Nanos rx_time)> m_recv_callback;

		std::string m_time_str;
		Logger& m_logger;

		auto defaultRecvCallback(TCPSocket* socket, Nanos rx_time) noexcept {
			m_logger.log("%: % %() % TCPSocket::defaultRecvCallback() "
				"socket: % len: % rx: %\n", Common::getCurrentTimeStr(&m_time_str),
				socket->m_fd, socket->m_next_recv_valid_index, rx_time);
		}


		explicit TCPSocket(Logger& logger) : m_logger(logger) {
			m_send_buffer = new char[TCPBufferSize];
			m_recv_buffer = new char[TCPBufferSize];
			m_recv_callback = [this](auto socket, auto rx_time) {
				defaultRecvCallback(socket, rx_time); };
		}

		void destroy() noexcept;

		~TCPSocket() {
			destroy();
			delete[] m_send_buffer;
			m_send_buffer = nullptr;
			delete[] m_recv_buffer;
			m_recv_buffer = nullptr;
		}

		TCPSocket() = delete;
		TCPSocket(const TCPSocket&) = delete;
		TCPSocket(const TCPSocket&&) = delete;
		TCPSocket& operator=(const TCPSocket&) = delete;
		TCPSocket& operator=(const TCPSocket&&) = delete;


		int connect(const std::string& ip, const std::string& iface, int port, bool is_listening);
		void send(const void* data, size_t len) noexcept;
		bool sendAndRecv() noexcept;
	};

}
