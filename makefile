ALL : Server.exe Client.exe
Server.exe : ./src/Server.cpp
	g++ -std=c++11 -O2 ./src/Server.cpp -lws2_32 -o Server.exe
Client.exe : ./src/Client.cpp
	g++ -std=c++11 -O2 ./src/Client.cpp -lws2_32 -o Client.exe