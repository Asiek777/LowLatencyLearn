#include "common/mcast_socket.hpp"

namespace Common {
    /// Initialize multicast socket to read from or publish to a stream.
    /// Does not join the multicast stream yet.
    int McastSocket::init(const std::string& ip, const std::string& iface, int port, bool is_listening) {
        const SocketCfg socket_cfg{  };
        m_socket_fd = createSocket(m_logger, ip, iface, port, true, false, is_listening, 0, false);
        return m_socket_fd;
    }

    /// Add / Join membership / subscription to a multicast stream.
    bool McastSocket::join(const std::string& ip) {
        return Common::join(m_socket_fd, ip);
    }

    /// Remove / Leave membership / subscription to a multicast stream.
    void McastSocket::leave(const std::string&, int) {
        close(m_socket_fd);
        m_socket_fd = -1;
    }

    /// Publish outgoing data and read incoming data.
    bool McastSocket::sendAndRecv() noexcept {
        // Read data and dispatch callbacks if data is available - non blocking.
        const ssize_t n_rcv = recv(m_socket_fd, m_inbound_data.data() + m_next_recv_valid_index, 
            McastBufferSize - m_next_recv_valid_index, MSG_DONTWAIT);
        if (n_rcv > 0) {
            m_next_recv_valid_index += n_rcv;
            m_logger.log("%:% %() % read socket:% len:%\n", __FILE__, __LINE__, __FUNCTION__, 
                Common::getCurrentTimeStr(&m_time_str), m_socket_fd,
                m_next_recv_valid_index);
            m_recv_callback(this);
        }

        // Publish market data in the send buffer to the multicast stream.
        if (m_next_send_valid_index > 0) {
            ssize_t n = ::send(m_socket_fd, m_outbound_data.data(), 
                m_next_send_valid_index, MSG_DONTWAIT | MSG_NOSIGNAL);

            m_logger.log("%:% %() % send socket:% len:%\n", __FILE__, __LINE__, __FUNCTION__, 
                Common::getCurrentTimeStr(&m_time_str), m_socket_fd, n);
        }
        m_next_send_valid_index = 0;

        return (n_rcv > 0);
    }

    /// Copy data to send buffers - does not send them out yet.
    void McastSocket::send(const void* data, size_t len) noexcept {
        memcpy(m_outbound_data.data() + m_next_send_valid_index, data, len);
        m_next_send_valid_index += len;
        ASSERT(m_next_send_valid_index < McastBufferSize, 
            "Mcast socket buffer filled up and sendAndRecv() not called.");
    }
}