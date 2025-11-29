#include <atomic>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using std::cerr;
using std::endl;

void check_cin()
{
    if (std::cin.eof())
    {
        cerr << "无效的输入！" << endl;
        cerr << "程序已结束。" << endl;
        throw std::runtime_error("Input error!");
        exit(1);
    }
}

class ChatClient
{
  private:
    SOCKET client_socket;
    std::atomic<bool> connected;
    std::thread receive_thread;
    std::mutex console_mutex;

  public:
    ChatClient() : client_socket(INVALID_SOCKET), connected(false) {}

    ~ChatClient() { disconnect(); }

    // bool connectToServer(const std::string &server_ip, int port)
    // {
    // 	// 初始化Winsock
    // 	WSADATA wsa_data;
    // 	if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
    // 	{
    // 		std::cerr << "WSAStartup failed: " << WSAGetLastError() <<
    // std::endl; 		return false;
    // 	}

    // 	// 创建客户端socket
    // 	client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    // 	if (client_socket == INVALID_SOCKET)
    // 	{
    // 		std::cerr << "Socket creation failed: " << WSAGetLastError() <<
    // std::endl; 		WSACleanup(); 		return false;
    // 	}

    // 	// 设置服务器地址
    // 	sockaddr_in server_addr{};
    // 	server_addr.sin_family = AF_INET;
    // 	server_addr.sin_port = htons(port);

    // 	if (inet_pton(AF_INET, server_ip.c_str(), &server_addr.sin_addr) <= 0)
    // 	{
    // 		std::cerr << "无效的IP地址: " << server_ip << std::endl;
    // 		closesocket(client_socket);
    // 		WSACleanup();
    // 		return false;
    // 	}

    // 	// 连接服务器
    // 	std::cout << "正在连接到服务器 " << server_ip << ":" << port << "..." <<
    // std::endl;

    // 	if (::connect(client_socket, (sockaddr *)&server_addr,
    // sizeof(server_addr)) == SOCKET_ERROR)
    // 	{
    // 		std::cerr << "连接服务器失败: " << WSAGetLastError() << std::endl;
    // 		closesocket(client_socket);
    // 		WSACleanup();
    // 		return false;
    // 	}

    // 	connected = true;
    // 	std::cout << "成功连接到服务器!" << std::endl;

    // 	return true;
    // }
    bool connectToServer(const std::string &server_ip, int port)
    {
        // 初始化Winsock
        WSADATA wsa_data;
        if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0)
        {
            std::cerr << "WSAStartup failed: " << WSAGetLastError()
                      << std::endl;
            return false;
        }

        // 使用 getaddrinfo 解析地址
        struct addrinfo hints, *result, *rp;
        memset(&hints, 0, sizeof(hints));
        hints.ai_family = AF_UNSPEC; // 同时支持 IPv4 和 IPv6
        hints.ai_socktype = SOCK_STREAM;
        hints.ai_protocol = IPPROTO_TCP;

        std::string port_str = std::to_string(port);
        int status =
            getaddrinfo(server_ip.c_str(), port_str.c_str(), &hints, &result);
        if (status != 0)
        {
            std::cerr << "getaddrinfo failed: " << gai_strerror(status)
                      << std::endl;
            WSACleanup();
            return false;
        }

        // 尝试所有返回的地址
        for (rp = result; rp != nullptr; rp = rp->ai_next)
        {
            client_socket =
                socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
            if (client_socket == INVALID_SOCKET)
            {
                continue;
            }

            // 设置 SO_REUSEADDR
            int opt = 1;
            setsockopt(client_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,
                       sizeof(opt));

            std::cout << "正在连接到服务器 " << server_ip << ":" << port
                      << "..." << std::endl;

            if (::connect(client_socket, rp->ai_addr, (int)rp->ai_addrlen) !=
                SOCKET_ERROR)
            {
                // 连接成功
                break;
            }

            // 连接失败，关闭socket继续尝试下一个地址
            closesocket(client_socket);
            client_socket = INVALID_SOCKET;
        }

        freeaddrinfo(result);

        if (client_socket == INVALID_SOCKET)
        {
            std::cerr << "连接服务器失败，所有地址尝试均失败" << std::endl;
            WSACleanup();
            return false;
        }

        connected = true;
        std::cout << "成功连接到服务器!" << std::endl;

        return true;
    }

    void startChat()
    {
        if (!connected)
        {
            std::cerr << "未连接到服务器!" << std::endl;
            return;
        }

        // 输入用户名
        std::string username;
        std::cout << "请输入用户名: ";
        std::getline(std::cin, username);
        check_cin();

        // 发送用户名到服务器
        if (send(client_socket, username.c_str(), username.length(), 0) ==
            SOCKET_ERROR)
        {
            std::cerr << "发送用户名失败: " << WSAGetLastError() << std::endl;
            return;
        }

        // 启动接收消息线程
        receive_thread = std::thread(&ChatClient::receiveMessages, this);

        std::cout << "\n=== 聊天室已就绪 ===" << std::endl;
        std::cout << "输入消息开始聊天，输入 '/quit' 或者 '/exit' 退出"
                  << std::endl;
        //			std::cout << "信息允许多行，键入Ctrl+q并回车来发送消息。" <<
        // std::endl;
        std::cout << "输入 '/help' 查看帮助" << std::endl;
        std::cout << "=================================" << std::endl;

        // 主线程处理用户输入
        std::string message;
        while (connected)
        {
            message = "";
            //				while (1)
            //				{
            //					char x;
            //					x = std::cin.get();
            //					check_cin();
            //					if (x == 17)break;
            //					message += x;
            //				}
            //				message[message.size()-1]='\0';
            //				cerr<<"here:"<<endl;
            std::getline(std::cin, message);

            if (!connected)
                break;

            if (message.empty())
            {
                continue;
            }

            if (message.size() >= 5 && (message.substr(0, 5) == "/quit" ||
                                        message.substr(0, 5) == "/exit"))
            {
                std::cout << "退出聊天室..." << std::endl;
                break;
            }

            if (message == "/help")
            {
                showHelp();
                continue;
            }

            // 发送消息到服务器
            if (send(client_socket, message.c_str(), message.length(), 0) ==
                SOCKET_ERROR)
            {
                std::cerr << "发送消息失败: " << WSAGetLastError() << std::endl;
                break;
            }
        }

        disconnect();
    }

    void disconnect()
    {
        connected = false;

        // 关闭socket
        if (client_socket != INVALID_SOCKET)
        {
            shutdown(client_socket, SD_BOTH);
            closesocket(client_socket);
            client_socket = INVALID_SOCKET;
        }

        // 等待接收线程结束
        if (receive_thread.joinable())
        {
            receive_thread.join();
        }

        WSACleanup();
        std::cout << "已断开服务器连接" << std::endl;
    }

  private:
    void receiveMessages()
    {
        char buffer[1024];

        while (connected)
        {
            int bytes_received =
                recv(client_socket, buffer, sizeof(buffer) - 1, 0);

            if (bytes_received == 0)
            {
                // 服务器主动关闭连接
                std::cout << "\n服务器已断开连接" << std::endl;
                connected = false;
                break;
            }
            else if (bytes_received == SOCKET_ERROR)
            {
                int error = WSAGetLastError();
                if (error != WSAEWOULDBLOCK)
                {
                    if (connected) // 只在仍然连接时显示错误
                    {
                        std::cerr << "\n接收数据错误: " << error << std::endl;
                    }
                    connected = false;
                    break;
                }
                continue;
            }

            // 安全地处理接收到的消息
            buffer[bytes_received] = '\0';
            std::string message(buffer);

            // 使用互斥锁保护控制台输出，避免输入和输出交错
            std::lock_guard<std::mutex> lock(console_mutex);
            std::cout << "\r" << message << std::endl;

            // 重新显示输入提示
            std::cout << "> " << std::flush;
        }
    }

    void showHelp()
    {
        std::cout << "\n=== 聊天室帮助 ===" << std::endl;
        std::cout << "直接输入消息      - 发送消息到聊天室" << std::endl;
        std::cout << "/quit 或 /exit   - 退出聊天室" << std::endl;
        std::cout << "/help            - 显示此帮助信息" << std::endl;
        std::cout << "==============================\n" << std::endl;
    }
};

int main()
{
    std::cout << "=== C++ 多线程聊天室客户端 ===" << std::endl;

    ChatClient client;

    // 获取服务器信息
    std::string server_ip;
    int port = 8080;

    std::cout << "请输入服务器IP地址 (默认: 127.0.0.1): ";
    std::getline(std::cin, server_ip);
    check_cin();

    if (server_ip.empty())
    {
        server_ip = "127.0.0.1";
    }

    std::cout << "请输入服务器端口 (默认: 8080): ";
    std::string port_str;
    std::getline(std::cin, port_str);
    check_cin();

    if (!port_str.empty())
    {
        try
        {
            port = std::stoi(port_str);
        }
        catch (const std::exception &e)
        {
            std::cout << "无效的端口号，使用默认端口 8080" << std::endl;
            port = 8080;
        }
    }

    // 连接服务器
    if (client.connectToServer(server_ip, port))
    {
        client.startChat();
    }
    else
    {
        std::cout << "连接服务器失败，请检查:" << std::endl;
        std::cout << "1. 服务器IP和端口是否正确" << std::endl;
        std::cout << "2. 服务器是否正在运行" << std::endl;
        std::cout << "3. 防火墙设置" << std::endl;
        std::cout << "按回车键退出..." << std::endl;
        std::cin.get();
    }

    return 0;
}
