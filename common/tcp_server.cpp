#include "common/tcp_server.hpp"


namespace Common {

	void TCPServer::destroy() {
		close(m_efd);
		m_efd = -1;
		m_listener_socket.destroy();
	}

	auto TCPServer::epoll_add(TCPSocket* socket) {
		epoll_event ev{};
		ev.events = EPOLLET | EPOLLIN;
		ev.data.ptr = reinterpret_cast<void*>(socket);
		return epoll_ctl(m_efd, EPOLL_CTL_ADD, socket->m_fd, &ev) != -1;
	}

	void TCPServer::listen(const std::string& iface, int port) {
		destroy();

		m_efd = epoll_create(1);
		ASSERT(m_efd >= 0, "epoll_create() failed error:" +
			std::string(strerror(errno)));

		ASSERT(m_listener_socket.connect("", iface, port, true) >= 0,
			"Listener socket failed to connect. iface:" + iface + " port:" +
			std::to_string(port) + "error:" + std::string(strerror(errno)));

		ASSERT(epoll_add(&m_listener_socket), "epoll_ctl()failed.error:" +
			std::string(strerror(errno)));
	}

	auto TCPServer::epoll_del(TCPSocket* socket) {
		return epoll_ctl(m_efd, EPOLL_CTL_DEL, socket->m_fd, nullptr) != -1;
	}

	auto TCPServer::del(TCPSocket* socket) {
		epoll_del(socket);
		m_sockets.erase(std::remove(m_sockets.begin(), m_sockets.end(), socket),
			m_sockets.end());
		m_receive_sockets.erase(std::remove(m_receive_sockets.begin(),
			m_receive_sockets.end(), socket), m_receive_sockets.end());
		m_send_sockets.erase(std::remove(m_send_sockets.begin(),
			m_send_sockets.end(), socket), m_send_sockets.end());
	}

	void TCPServer::poll() noexcept {
		const int max_events = 1 + m_sockets.size();

		for (auto socket : m_disconnected_sockets) {
			del(socket);
		}

		const int n = epoll_wait(m_efd, m_events, max_events, 0);
		bool have_new_connections = false;

		for (int i = 0; i < n; i++) {
			epoll_event& event = m_events[i];
			auto socket = reinterpret_cast<TCPSocket*>(event.data.ptr);

			if (event.events & EPOLLIN) {
				if (socket == &m_listener_socket) {
					m_logger.log("%: % %() % EPOLLIN listener_socket: % \n",
						__FILE__, __LINE__, __FUNCTION__,
						Common::getCurrentTimeStr(&m_time_str), socket->m_fd);
					have_new_connections = true;
					continue;
				}
				m_logger.log("%: % %() % EPOLLIN socket: %\n", __FILE__, __LINE__,
					__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), socket->m_fd);
				if (std::find(m_receive_sockets.begin(), m_receive_sockets.end(),
					socket) == m_receive_sockets.end()) {
					m_receive_sockets.push_back(socket);
				}
			}

			if (event.events & EPOLLOUT) {
				m_logger.log("%: % %() % EPOLLOUT socket: %\n", __FILE__, __LINE__,
					__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), socket->m_fd);
				if (std::find(m_send_sockets.begin(), m_send_sockets.end(),
					socket) == m_send_sockets.end()) {
					m_send_sockets.push_back(socket);
				}
			}
			if (event.events & (EPOLLERR | EPOLLHUP)) {
				m_logger.log("%: % %() % EPOLLERR socket: %\n", __FILE__, __LINE__,
					__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), socket->m_fd);
				if (std::find(m_disconnected_sockets.begin(), m_disconnected_sockets.end(),
					socket) == m_disconnected_sockets.end()) {
					m_disconnected_sockets.push_back(socket);
				}
			}
		}

		while (have_new_connections) {
			m_logger.log("%: % %() % have_new_connection\n", __FILE__, __LINE__,
				__FUNCTION__, Common::getCurrentTimeStr(&m_time_str));

			sockaddr_storage addr;
			socklen_t addr_len = sizeof(addr);
			int fd = accept(m_listener_socket.m_fd,
				reinterpret_cast<sockaddr*>(&addr), &addr_len);

			if (fd == -1)
				break;

			ASSERT(setNonBlocking(fd) && setNoDelay(fd),
				"Failed to set non - blocking or no - delay on socket : " +
				std::to_string(fd));

			m_logger.log("%: % %() % accepted socket: %\n", __FILE__, __LINE__,
				__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), fd);

			TCPSocket* socket = new TCPSocket(m_logger);
			socket->m_fd = fd;
			socket->m_recv_callback = m_recv_callback;
			ASSERT(epoll_add(socket), "Unable to add socket. erro: " +
				std::string(strerror(errno)));
			
			if (std::find(m_sockets.begin(), m_sockets.end(),
				socket) == m_sockets.end()) {
				m_sockets.push_back(socket);
			}
			if (std::find(m_receive_sockets.begin(), m_receive_sockets.end(),
				socket) == m_receive_sockets.end()) {
				m_receive_sockets.push_back(socket);
			}
		}
	}

	void TCPServer::sendAndRecv() noexcept {
		auto recv = false;

		for (auto socket : m_receive_sockets) {
			if (socket->sendAndRecv())
				recv = true;
		}
		if (recv)
			m_recv_finished_callback();

		for (auto socket : m_send_sockets) {
			socket->sendAndRecv();
		}
	}
}