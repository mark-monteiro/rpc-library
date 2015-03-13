#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <limits.h>     //HOST_NAME_MAX
#include <netinet/in.h> //sockadd_in
#include <netdb.h>      //gethostbyname
#include <algorithm>    //max
#include <map>          //map
#include <stack>        // std::stack

#include "debug.h"
#include "rpc_helpers.h"
#include "message.h"
#include "serialize.h"
#include "binder.h"
#include "function_signature.h"

// #include "debug.h"
# define debug_print(x) do {} while (0)

using namespace std;
vector<struct Server*> registeredServers;
vector<FunctionSignatureAndServer> registeredFunctions;
std::stack<struct Server*> recentlyCalledServers;

FunctionSignatureAndServer::FunctionSignatureAndServer(char *name, vector<int> argTypes, Server* s) {
    functionSignature = FunctionSignature(name, argTypes);
    server = s;
}


bool process_port(int sock);

char* string_to_cstring(string s) {
    return &s[0];
}

template<class TYPE>
TYPE* vector_to_array(vector<TYPE> x) {
    return &x[0];
}

bool is_equal(vector<int> args1, vector<ArgType> args2) {
    for (int i = 0; (i < args1.size()) && (i < args2.size()); i++) {
        if (ArgType(args1[i]) != args2[i]) return false;
    }
    return true;
}


int main(void) {
    int listener = open_connection();
    if(listener == -1) {
        debug_print(("Failed to create listening socket"));
        return -1;
    }

    // Print hostname and port
    printf("BINDER_ADDRESS %s\n", getHostname());
    printf("BINDER_PORT %d\n", getPort(listener));
    fflush(stdout);

    debug_print(("server: waiting for connections...\n"));

    // select() variables
    fd_set client_fds;    // client_fds file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    // Initialize the client_fds set with the listener
    FD_ZERO(&client_fds);
    FD_SET(listener, &client_fds);
    fdmax = listener;


    // main loop
    while(true) {
        //// check for and receive messages from clients
        read_fds = client_fds; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(int fd = 0; fd <= fdmax; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                // Handle new connections
                if (fd == listener) { 
                    struct sockaddr_storage remoteaddr;
                    socklen_t addr_len = sizeof remoteaddr;
                    
                    // Accept the connection
                    int newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addr_len);
                    if (newfd == -1) {
                        perror("accept");
                        continue;
                    }

                    // Add socket to client_fds set
                    FD_SET(newfd, &client_fds); // add to client_fds set
                    fdmax = max(fdmax, newfd);
                    debug_print(("New connection accepted on socket %d\n", newfd));
                }
                // Handle data from a connection
                else {
                    if(process_port(fd) == false) {
                        debug_print(("process_port failed; closing socket %d\n", fd));
                        close(fd);
                        FD_CLR(fd, &client_fds);
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END main loop

    return 0;
}

bool register_server_and_function(Message message, int sock) {
    Message response;
    vector<char>::iterator index;
    char* newFunctionName;
    vector<int> newFunctionArgTypes;
    
    struct Server newServer;

    // Register server if currServer is not already registered
    for (vector<struct Server*>::iterator currServer = registeredServers.begin(); currServer != registeredServers.end(); ++currServer) {
        if (sock == (*currServer)->sock){
            debug_print(("server previously registered, sockfd: %s\n", sock));
            break;
        }

         // Server not already registered, add server to registeredServers
        if (currServer == registeredServers.end()){
            // Get id, port and sock from message
            index = message.data.begin();
            newServer.id = deserializeString(index);
            debug_print(("identifier deserialized: %s\n", newServer.id));
            newServer.port = deserializeInt(index);
            debug_print(("port deserialized: %d\n", newServer.port));
            newServer.sock = sock;

            // Add server to the registeredServers vector
            registeredServers.push_back(&newServer);
        }
    }
        
    newFunctionName = string_to_cstring(deserializeString(index));
    newFunctionArgTypes = deserializeArgTypesIntoVector(index);

    debug_print(("name deserialized: %s\n", function_name));


    // If FunctionSignatureAndServer does not exist in registeredFunctions vector
    // push FunctionSignatureAndServer into registeredFunctions vector
    vector<FunctionSignatureAndServer>::iterator currFnServer;
    for (currFnServer = registeredFunctions.begin(); currFnServer != registeredFunctions.end(); ++currFnServer) {
        // TODO: fix comparison of functions
        if ((newFunctionName == (*currFnServer).functionSignature.name) && 
            is_equal(newFunctionArgTypes, (*currFnServer).functionSignature.argTypes)) {
            debug_print(("attempt to register already registered function"));
            break;
        }
        
        // Push FunctionSignatureAndServer to registeredFunctions vector
        FunctionSignatureAndServer newFnSignatureAndServer(newFunctionName, newFunctionArgTypes, &newServer);

        if (currFnServer == registeredFunctions.end()) registeredFunctions.push_back(newFnSignatureAndServer);
    }


    // send register response to server
    response.type = REGISTER_RESPONSE;
    response.addData(serializeInt(0));
    return response.send(sock);
}

bool locate_method_on_server(Message message, int sock) {
    // Searches the database and return the server address
    struct Server* serverPtr = NULL;
    char *function_name;
    vector<int> function_argTypes;
    Message response;

    // Loop through registered servers to find the one with socket in message
    for (vector<struct Server*>::iterator currServer = registeredServers.begin(); 
        currServer != registeredServers.end(); ++currServer) {
        if (sock == (*currServer)->sock) {serverPtr = (Server*)(&(*currServer)); break;}
    }

     // Server not found
    if (!serverPtr) return false;

    // Server found - Get name and argTypes from message
    vector<char>::iterator index = message.data.begin();
    function_name = string_to_cstring(deserializeString(index));
    debug_print(("name deserialized: %s\n", name));
    function_argTypes = deserializeArgTypesIntoVector(index);


    // Locate method in registeredFunctions vectpr
    vector<FunctionSignatureAndServer>::iterator currFnS;
    for (currFnS = registeredFunctions.begin(); currFnS != registeredFunctions.end(); ++currFnS) {
        if ((function_name == (*currFnS).functionSignature.name) && 
            is_equal(function_argTypes, (*currFnS).functionSignature.argTypes)) {
            // Push to recentlyCalledServers queue???
            // recentlyCalledServers.push(serverPtr);
            
            // Send success response to server
            response.type = LOC_SUCCESS;
            response.addData(serializeString(string_to_cstring(serverPtr->id)));
            response.addData(serializeInt(serverPtr->port));

            return response.send(sock); 
        }
    }


    // If it gets to here, it means function was not registered 
    // for that server 
    response.type = LOC_FAILURE;

    // Add failure code
    // response.addData(serializeInt());

    return response.send(sock);
}

// Handle incoming data on a socket
// Receives the next message on that socket,
// and calls a handler based on the message type
bool process_port(int sock) {
    Message recv_message;
    debug_print(("--------------------------------------\n"));
    debug_print(("Processing request on socket %d\n", sock));

    // Receive message
    if(Message::recv(sock, &recv_message) == false) return false;

    // Execute handler based on message type
    switch(recv_message.type) {
        case REGISTER: return register_server_and_function(recv_message, sock);
        case LOC_REQUEST: return locate_method_on_server(recv_message, sock);

        default:
        debug_print(("Invalid message type sent to binder: %s\n", recv_message.typeToString().c_str()));
        return false;
    }
}
