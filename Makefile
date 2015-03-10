FLAGS=-Wall
DEBUG_FLAGS=-Wall -DDEBUG -g -O0

CXX = g++
all:
	# g++ -c $(FLAGS) message.cpp
	# g++ -c $(FLAGS) rpc_client.cpp
	# g++ -c $(FLAGS) rpc_server.cpp
	# ar rc rpc.a rpc_server.o rpc_client.o
	g++ -o binder rpc_helpers.cpp message.cpp binder.cpp 

#TODO: separate into message, helpers, rpc actions

debug:
	$(CXX) -c $(DEBUG_FLAGS) serialize.cpp rpc_client.cpp rpc_server.cpp rpc_helpers.cpp message.cpp
	ar rc librpc.a serialize.o rpc_helpers.o message.o rpc_server.o rpc_client.o
	ranlib librpc.a 
	$(CXX) -o binder $(DEBUG_FLAGS) serialize.cpp rpc_helpers.cpp message.cpp binder.cpp

test:
	$(CXX) -c server_functions.c server_function_skels.c
	$(CXX) -L. client1.c -lrpc -o client
	$(CXX) -L. server_functions.o server_function_skels.o server.c -lrpc -o server	

clean:
	-rm *.o
	-rm librpc.a binder client server
