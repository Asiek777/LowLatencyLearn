#include "common/time_utils.hpp"
#include "common/tcp_server.hpp"


int main(int, char**) {
	using namespace Common;

	std::string m_time_str;
	Logger logger("socket_example.log");

	auto tcpServerRecvCallback = [&](TCPSocket* socket, Nanos rx_time) noexcept {
		logger.log("TCPServer::defaultRecvCallback() socket: % len : % rx : % \n",
			socket->m_fd, socket->m_next_recv_valid_index, rx_time);
		const std::string reply = "TCPServer received msg:" +
			std::string(socket->m_recv_buffer, socket->m_next_recv_valid_index);
		socket->m_next_recv_valid_index = 0;
		socket->send(reply.data(), reply.length());
		};

	auto tcpServerRecvFinishedCallback = [&]() noexcept {
		logger.log("TCPServer::defaultRecvFinishedCallback()\n");
		};

	auto tcpClientRecvCallback = [&](TCPSocket* socket, Nanos rx_time) noexcept {
		const std::string recv_msg = std::string(
			socket->m_recv_buffer, socket->m_next_recv_valid_index);
		socket->m_next_recv_valid_index = 0;

		logger.log("TCPServer::defaultRecvCallback() socket: % len: % rx: % msg: %\n",
			socket->m_fd, socket->m_next_recv_valid_index, rx_time, recv_msg);
		};

	const std::string iface = "lo";
	const std::string ip = "127.0.0.1";
	const int port = 12345;

	logger.log("Creating TCPServer on iface: % port: %\n", iface, port);

	TCPServer server(logger);
	server.m_recv_callback = tcpServerRecvCallback;
	server.m_recv_finished_callback = tcpServerRecvFinishedCallback;
	server.listen(iface, port);

	std::vector<TCPSocket*> clients{ 5 };

	for (size_t i = 0; i < clients.size(); i++) {
		clients[i] = new TCPSocket(logger);
		clients[i]->m_recv_callback = tcpClientRecvCallback;

		logger.log("Connecting TCPClient-[%] on ip: % iface: % port: % \n",
			i, ip, iface, port);
		clients[i]->connect(ip, iface, port, false);
		server.poll();
	}

	using namespace std::literals::chrono_literals;

	const auto itrCount = 5;
	for (auto itr = 0; itr < itrCount; itr++) {
		for (size_t i = 0; i < clients.size(); i++) {
			const std::string client_msg = "CLIENT-[" + std::to_string(i) +
				"] : Sending " + std::to_string(itr * 100 + i);
			logger.log("Sending TCPClient-[%] %\n", i, client_msg);

			clients[i]->send(client_msg.data(), client_msg.length());
			clients[i]->sendAndRecv();
		}
	}

	for (auto itr = 0; itr < itrCount; itr++) {
		for (auto& client : clients)
			client->sendAndRecv();
		server.poll();
		server.sendAndRecv();
		std::this_thread::sleep_for(500ms);
	}

	return 0;
}