#ALL : Server.exe Client.exe
#Server.exe : ./src/Server.cpp
#	g++ -std=c++11 -O2 ./src/Server.cpp -lws2_32 -o Server.exe
#Client.exe : ./src/Client.cpp
#	g++ -std=c++11 -O2 ./src/Client.cpp -lws2_32 -o Client.exe
CXX = g++
CXX_FLAG = -std=c++11 -O2 
LDFLAG = -lws2_32

SRC_DIR = ./src

ALL : Server.exe Client.exe

Server.exe : $(SRC_DIR)/Server.cpp
	$(CXX) $(CXX_FLAG) $< $(LDFLAG) -o $@
Client.exe : $(SRC_DIR)/Client.cpp
	$(CXX) $(CXX_FLAG) $< $(LDFLAG) -o $@

clean:
	rm -f Server.exe Client.exe

.PHONY: ALL clean