#include "common/tcp_socket.hpp"

namespace Common {

	void TCPSocket::destroy() noexcept {
		close(m_fd);
		m_fd = -1;
	}

	int TCPSocket::connect(const std::string& ip, const std::string& iface,
		int port, bool is_listening) {
		destroy();
		m_fd = createSocket(m_logger, ip, iface, port, false, false, is_listening, 0, true);

		inInAddr.sin_addr.s_addr = INADDR_ANY;
		inInAddr.sin_port = htons(port);
		inInAddr.sin_family = AF_INET;

		return m_fd;
	}

	void TCPSocket::send(const void* data, size_t len) noexcept {
		if (len > 0) {
			memcpy(m_send_buffer + m_next_send_valid_index, data, len);
			m_next_send_valid_index += len;
		}
	}

	bool TCPSocket::sendAndRecv() noexcept {
		char ctrl[CMSG_SPACE(sizeof(struct timeval))];
		struct cmsghdr* cmsg = (struct cmsghdr*)&ctrl;

		struct iovec iov;
		iov.iov_base = m_recv_buffer + m_next_recv_valid_index;
		iov.iov_len = TCPBufferSize - m_next_recv_valid_index;

		msghdr msg;
		msg.msg_control = ctrl;
		msg.msg_controllen = sizeof(ctrl);
		msg.msg_name = &inInAddr;
		msg.msg_namelen = sizeof(inInAddr);
		msg.msg_iov = &iov;
		msg.msg_iovlen = 1;

		const auto n_recv = recvmsg(m_fd, &msg, MSG_DONTWAIT);
		if (n_recv > 0) {
			m_next_recv_valid_index += n_recv;

			Nanos kernel_time = 0;
			struct timeval time_kernel;
			if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_TIMESTAMP &&
				cmsg->cmsg_len == CMSG_LEN(sizeof(time_kernel))) {
				memcpy(&time_kernel, CMSG_DATA(cmsg), sizeof(time_kernel));
				kernel_time = time_kernel.tv_sec * NANOS_TO_SECS +
					time_kernel.tv_usec * NANOS_TO_MICROS;
			}

			const auto user_time = getCurrentNanos();

			m_logger.log("%: % %() % read socket: % len: % utime: % ktime: % diff: % \n",
				__FILE__, __LINE__, __FUNCTION__, Common::getCurrentTimeStr(&m_time_str), m_fd,
				m_next_recv_valid_index, user_time, kernel_time, user_time - kernel_time);
			m_recv_callback(this, kernel_time);
		}

		ssize_t n_send = std::min(TCPBufferSize, m_next_send_valid_index);
		while (n_send > 0) {
			auto n_send_this_msg = std::min(
				static_cast<ssize_t>(m_next_send_valid_index), n_send);
			const int flags = MSG_DONTWAIT | MSG_NOSIGNAL |
				(n_send_this_msg < n_send ? MSG_MORE : 0);
			auto n = ::send(m_fd, m_send_buffer, n_send_this_msg, flags);

			if (n < 0) [[unlikely]] {
				if (!wouldBlock())
					m_send_disconnected = true;
				break;
			}
			m_logger.log("%: % %() % send socket: % len: %\n", __FILE__, __LINE__,
				__FUNCTION__, Common::getCurrentTimeStr(&m_time_str), m_fd, n);

			n_send -= n;

			ASSERT(n == n_send_this_msg, "Don't support partial send lengths yet.");
		}
		m_next_send_valid_index = 0;

		return n_recv > 0;

	}
}

