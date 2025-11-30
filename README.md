# C++ 局域网聊天系统
一个基于C++实现的多线程局域网聊天系统，包含服务器端和客户端，支持多用户实时聊天。
## 功能特性
### 客户端功能
- 连接指定IP和端口的聊天服务器
- 设置个性化用户名
- 实时发送和接收消息
- 多线程处理（输入和接收分离）
- 命令支持：
  - `/help` - 查看帮助信息
  - `/quit` 或 `/exit` - 退出聊天室
- 线程安全的控制台输出
### 服务器功能
- 多客户端并发连接支持
- 用户加入/离开通知
- 消息广播功能
- 自动处理客户端断开连接
- 线程安全的客户端管理
## 技术栈
- **语言**: C++
- **网络库**: Winsock2 (Windows Socket API)
- **多线程**: std::thread
- **同步机制**: std::mutex, std::atomic
- **编译环境**: 支持C++11标准的编译器
## 编译说明
### 客户端编译
```bash
g++ -std=c++11 src/Client.cpp -lws2_32 -o Client.exe
```
### 服务器编译
```bash
g++ -std=c++11 src/Server.cpp -lws2_32 -o Server.exe
```  
### 或直接使用Makefile  
```bash
make
```
**注意**: 本项目使用Windows Socket API，仅支持Windows平台编译运行。
## 使用方法
### 1. 启动服务器
```bash
./Server.exe
```
服务器默认监听8080端口。
### 2. 启动客户端
```bash
./Client.exe
```
按照提示输入：
- 服务器IP地址（默认：127.0.0.1）
- 服务器端口（默认：8080）
- 用户名
- 目前已同时支持IPv4和IPv6
### 3. 开始聊天
- 直接输入消息并回车发送
- 使用 `/help` 查看可用命令
- 使用 `/quit` 或 `/exit` 退出
## 项目结构
```bash
CHAT_IN_LAN_PROJECT/
├── .vscode/
│   └── settings.json          # VSCode 编辑器配置
├── src/
│   ├── Client.cpp             # 客户端实现
│   └── Server.cpp             # 服务器实现
├── .gitignore                 # Git 忽略文件配置
├── makefile                   # 构建脚本
└── README.md                  # 项目说明文档
```
## 核心类说明
### ChatClient (客户端)
- `connectToServer()` - 连接服务器
- `startChat()` - 开始聊天会话
- `disconnect()` - 断开连接
- `receiveMessages()` - 接收消息线程函数
### ChatServer (服务器)
- `start()` - 启动服务器
- `stop()` - 停止服务器
- `acceptConnections()` - 接受客户端连接
- `handleClient()` - 处理客户端消息
- `broadcastMessage()` - 广播消息给所有客户端
## 注意事项
1. **平台限制**: 目前仅支持Windows系统
2. **防火墙**: 确保防火墙允许程序访问网络
3. **局域网**: 服务器IP应设置为局域网可达的地址
4. **端口占用**: 确保8080端口未被其他程序占用
## 扩展建议
- 添加私聊功能 (`/whisper` 命令)
- 支持文件传输
- 添加用户认证系统
- 实现聊天记录保存
- 支持更多的平台（Linux/macOS）
## 许可证
本项目仅供学习和交流使用。
