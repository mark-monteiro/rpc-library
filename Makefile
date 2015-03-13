FLAGS=-Wall
DEBUG_FLAGS=-Wall -DDEBUG -ggdb -O0

default:
	g++ -c $(FLAGS) arg_type.cpp function_signature.cpp serialize.cpp rpc_client.cpp rpc_server.cpp rpc_helpers.cpp message.cpp
	ar rc librpc.a arg_type.o function_signature.o serialize.o rpc_helpers.o message.o rpc_server.o rpc_client.o
	ranlib librpc.a 
	g++ -o binder $(FLAGS) arg_type.cpp serialize.cpp rpc_helpers.cpp message.cpp binder.cpp

debug:
	clang++ -c $(DEBUG_FLAGS) arg_type.cpp function_signature.cpp serialize.cpp rpc_client.cpp rpc_server.cpp rpc_helpers.cpp message.cpp
	ar rc librpc.a arg_type.o function_signature.o serialize.o rpc_helpers.o message.o rpc_server.o rpc_client.o
	ranlib librpc.a 
	clang++ -o binder $(DEBUG_FLAGS) arg_type.cpp serialize.cpp rpc_helpers.cpp message.cpp binder.cpp

test:
	clang++ -c server_functions.c server_function_skels.c
	clang++ -L. client1.c -lrpc -o client
	clang++ -L. server_functions.o server_function_skels.o server.c -lrpc -o server	

clean:
	-rm *.o
	-rm librpc.a binder client server

# Run this before submitting to remove any unecessary files
submit_clean: clean
	-rm server*
	-rm client*
	-rm test*
	-rm .gitignore