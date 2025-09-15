#pragma once

#include <functional>
#include <vector>
#include <string>

#include "common/socket_utils.hpp"

#include "common/logging.hpp"

namespace Common {
    /// Size of send and receive buffers in bytes.
    constexpr size_t McastBufferSize = 64 * 1024 * 1024;

    struct McastSocket {
        McastSocket(Logger& logger)
            : m_logger(logger) {
            m_outbound_data.resize(McastBufferSize);
            m_inbound_data.resize(McastBufferSize);
        }

        /// Initialize multicast socket to read from or publish to a stream.
        /// Does not join the multicast stream yet.
        int init(const std::string& ip, const std::string& iface, int port, bool is_listening);

        /// Add / Join membership / subscription to a multicast stream.
        bool join(const std::string& ip);

        /// Remove / Leave membership / subscription to a multicast stream.
        void leave(const std::string& ip, int port);

        /// Publish outgoing data and read incoming data.
        bool sendAndRecv() noexcept;

        /// Copy data to send buffers - does not send them out yet.
        void send(const void* data, size_t len) noexcept;

        int m_socket_fd = -1;

        /// Send and receive buffers, typically only one or the other is needed, not both.
        std::vector<char> m_outbound_data;
        size_t m_next_send_valid_index = 0;
        std::vector<char> m_inbound_data;
        size_t m_next_recv_valid_index = 0;

        /// Function wrapper for the method to call when data is read.
        std::function<void(McastSocket* s)> m_recv_callback = nullptr;

        std::string m_time_str;
        Logger& m_logger;
    };
}