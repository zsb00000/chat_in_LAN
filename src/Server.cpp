#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <algorithm>

#pragma comment(lib, "ws2_32.lib")

class ChatServer
{
	private:
		SOCKET server_socket;
		std::atomic<bool> running;

		struct ClientInfo
		{
			SOCKET socket;
			std::string username;
			std::thread thread;
		};

		std::vector<ClientInfo> clients;
		std::mutex clients_mutex;

	public:
		ChatServer() : server_socket(INVALID_SOCKET), running(false) {}

		~ChatServer()
		{
			stop();
		}

		bool start(int port)
		{
			// 初始化Winsock
			WSADATA wsa_data;
			if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
			{
				std::cerr << "WSAStartup failed: " << WSAGetLastError() << std::endl;
				return false;
			}

			// 创建服务器socket
			server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (server_socket == INVALID_SOCKET)
			{
				std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
				WSACleanup();
				return false;
			}

			// 设置SO_REUSEADDR
			int opt = 1;
			if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
			               (char *)&opt, sizeof(opt)) == SOCKET_ERROR)
			{
				std::cerr << "Setsockopt failed: " << WSAGetLastError() << std::endl;
			}

			// 绑定地址
			sockaddr_in server_addr{};
			server_addr.sin_family = AF_INET;
			server_addr.sin_addr.s_addr = INADDR_ANY;
			server_addr.sin_port = htons(port);

			if (bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr)) == SOCKET_ERROR)
			{
				std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
				closesocket(server_socket);
				WSACleanup();
				return false;
			}

			// 开始监听
			if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR)
			{
				std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
				closesocket(server_socket);
				WSACleanup();
				return false;
			}

			std::cout << "服务器启动成功，监听端口: " << port << std::endl;
			running = true;

			// 启动接受连接线程
			std::thread accept_thread(&ChatServer::acceptConnections, this);
			accept_thread.detach();

			return true;
		}

		void stop()
		{
			if (!running) return;

			running = false;

			// 关闭服务器socket
			if (server_socket != INVALID_SOCKET)
			{
				closesocket(server_socket);
				server_socket = INVALID_SOCKET;
			}

			// 关闭所有客户端连接
			{
				std::lock_guard<std::mutex> lock(clients_mutex);
				for (auto &client : clients)
				{
					if (client.socket != INVALID_SOCKET)
					{
						shutdown(client.socket, SD_BOTH);
						closesocket(client.socket);
					}
					if (client.thread.joinable())
					{
						client.thread.join();
					}
				}
				clients.clear();
			}

			WSACleanup();
			std::cout << "服务器已关闭" << std::endl;
		}

	private:
		void acceptConnections()
		{
			std::cout << "开始接受客户端连接..." << std::endl;

			while (running)
			{
				sockaddr_in client_addr{};
				int client_addr_len = sizeof(client_addr);

				SOCKET client_socket = accept(server_socket,
				                              (sockaddr *)&client_addr,
				                              &client_addr_len);

				if (client_socket == INVALID_SOCKET)
				{
					if (running)
					{
						std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
					}
					continue;
				}

				// 获取客户端IP地址
				char client_ip[INET_ADDRSTRLEN];
				inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
				std::cout << "新的客户端连接: " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;

				// 创建客户端处理线程
				std::thread client_thread(&ChatServer::handleClient, this, client_socket);

				{
					std::lock_guard<std::mutex> lock(clients_mutex);
					clients.push_back({client_socket, "", std::move(client_thread)});
				}
			}
		}

		void handleClient(SOCKET client_socket)
		{
			char buffer[1024];
			std::string username;

			try
			{
				// 接收用户名
				int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
				if (bytes_received <= 0)
				{
					throw std::runtime_error("接收用户名失败");
				}

				buffer[bytes_received] = '\0';
				username = std::string(buffer);

				// 更新客户端信息中的用户名
				{
					std::lock_guard<std::mutex> lock(clients_mutex);
					for (auto &client : clients)
					{
						if (client.socket == client_socket)
						{
							client.username = username;
							break;
						}
					}
				}

				std::string welcome_msg = "用户 " + username + " 加入了聊天室";
				std::cout << welcome_msg << std::endl;
				broadcastMessage(welcome_msg, client_socket);

				// 主消息循环
				while (running)
				{
					bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

					if (bytes_received == 0)
					{
						std::cout << "客户端 " << username << " 断开连接" << std::endl;
						break;
					}
					else if (bytes_received == SOCKET_ERROR)
					{
						int error = WSAGetLastError();
						if (error != WSAEWOULDBLOCK)
						{
							std::cerr << "接收数据错误: " << error << std::endl;
							break;
						}
						continue;
					}

					buffer[bytes_received] = '\0';
					std::string message(buffer);

					// 处理退出命令
					if (message == "/quit" || message == "/exit")
					{
						std::cout << "用户 " << username << " 请求退出" << std::endl;
						break;
					}

					std::string full_message = username + ": " + message;
					std::cout << full_message << std::endl;
					broadcastMessage(full_message, client_socket);
				}
			}
			catch (const std::exception &e)
			{
				std::cerr << "处理客户端错误: " << e.what() << std::endl;
			}

			// 清理客户端
			removeClient(client_socket);
			if (!username.empty())
			{
				std::string leave_msg = "用户 " + username + " 离开了聊天室";
				std::cout << leave_msg << std::endl;
				broadcastMessage(leave_msg);
			}
		}

		void broadcastMessage(const std::string &message, SOCKET exclude_socket = INVALID_SOCKET)
		{
			std::lock_guard<std::mutex> lock(clients_mutex);

			for (const auto &client : clients)
			{
				if (client.socket != exclude_socket && client.socket != INVALID_SOCKET)
				{
					send(client.socket, message.c_str(), message.length(), 0);
				}
			}
		}

		void removeClient(SOCKET client_socket)
		{
			std::lock_guard<std::mutex> lock(clients_mutex);

			auto it = std::remove_if(clients.begin(), clients.end(),
			                         [client_socket](const ClientInfo & client)
			{
				return client.socket == client_socket;
			});

			if (it != clients.end())
			{
				if (it->thread.joinable())
				{
					it->thread.detach();
				}
				if (it->socket != INVALID_SOCKET)
				{
					closesocket(it->socket);
				}
				clients.erase(it, clients.end());
			}
		}
};

int main()
{
	std::cout << "=== C++ 多线程聊天室服务器 ===" << std::endl;

	ChatServer server;

	// 启动服务器，监听8080端口
	if (!server.start(8080))
	{
		std::cerr << "服务器启动失败!" << std::endl;
		return 1;
	}

	std::cout << "服务器运行中..." << std::endl;
	std::cout << "输入 'quit' 或按 Ctrl+C 退出" << std::endl;

	// 等待用户输入退出命令
	std::string command;
	while (std::getline(std::cin, command))
	{
		if (command == "quit" || command == "exit")
		{
			break;
		}
	}

	server.stop();
	return 0;
}
