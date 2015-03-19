#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>     //HOST_NAME_MAX
#include <netinet/in.h> //sockadd_in
#include <netdb.h>      //gethostbyname
#include <algorithm>    //max
#include <map>          //map

#include "debug.h"
#include "rpc_helpers.h"
#include "message.h"
#include "serialize.h"
#include "function_signature.h"
#include "arg_type.h"
#include "binder_helpers.h"
#include "server.h"
#include "function_signature_and_server.h"

using namespace std;

// Keep track of registered servers and registered functions 
vector<struct Server*> registeredServers;
vector<FunctionSignatureAndServer> registeredFunctions;

// Round-robin server navigation  
vector<struct Server*>::iterator currentServer;

// Termination state
int terminateBinder = 0;

// Increment currentServer iterator
void nextServer(){
    if (currentServer == registeredServers.end()){
        currentServer = registeredServers.begin();
    }
    else{
        currentServer++;
    }
}

bool register_server_and_function(Message message, int sock) {
    Message response;
    vector<char>::iterator index;
    char* newFunctionName;
    vector<ArgType> newFunctionArgTypes;
    
    struct Server newServer;
    index = message.data.begin();
    newServer.id = deserializeString(index);
    debug_print(("identifier deserialized: %s\n", newServer.id.c_str()));
    newServer.port = deserializeInt(index);
    debug_print(("port deserialized: %d\n", newServer.port));

    newServer.sock = sock;
    
    // Try to find newServer in registeredServers
    vector<struct Server*>::iterator findResults = find_if(registeredServers.begin(), registeredServers.end(), bind1st(ptr_fun(server_ptr_eq), &newServer));
    
    // If server is not already registered, add server to registeredServers
    if (findResults == registeredServers.end()){
        // Add server to the registeredServers vector
        registeredServers.push_back(&newServer);
        if (registeredServers.size() == 1){
            currentServer = registeredServers.begin();
        }
    }
    else{
        debug_print(("attempt to register already registered server"));
    }

    newFunctionName = string_to_cstring(deserializeString(index));
    debug_print(("name deserialized: %s\n", newFunctionName));
    newFunctionArgTypes = deserializeArgTypesIntoArgTypeVector(index);


    FunctionSignatureAndServer newFnSignatureAndServer(newFunctionName, newFunctionArgTypes, &newServer);

    // Try to find newFnSignatureAndServer in registeredServers
    vector<struct FunctionSignatureAndServer>::iterator findResults2 = find(registeredFunctions.begin(), registeredFunctions.end(), newFnSignatureAndServer);
    
    // Not found
    if (findResults2 == registeredFunctions.end()){
        // Push FunctionSignatureAndServer to registeredFunctions vector
        registeredFunctions.push_back(newFnSignatureAndServer);
    }
    else{
        debug_print(("attempt to register already registered function"));
    }

    // send register response to server
    response.type = REGISTER_RESPONSE;
    response.addData(serializeInt(0));
    return response.send(sock);
}

bool locate_method_on_server(Message message, int sock) {
    // Searches the database and return the server address
    struct Server* serverPtr = NULL;
    char *functionName;
    vector<ArgType> functionArgTypes;
    Message response;
    Server* currentServerInit;

    // Get name and argTypes from message
    vector<char>::iterator index = message.data.begin();
    functionName = string_to_cstring(deserializeString(index));
    debug_print(("name deserialized: %s\n", functionName));
    functionArgTypes = deserializeArgTypesIntoArgTypeVector(index);

    FunctionSignatureAndServer newFnSignatureAndServer(functionName, functionArgTypes, NULL); // operator== doesn't compare server values
    // Locate method in registeredFunctions vector
    vector<FunctionSignatureAndServer>::iterator currFnS;

    if (registeredServers.size() > 0) {
        currentServerInit = *currentServer;
        //TODO: Not the most efficent, will fix later
        while(true) {
            for (currFnS = registeredFunctions.begin(); currFnS != registeredFunctions.end(); ++currFnS) { 
                if ((*currentServer == (*currFnS).server) &&
                    (newFnSignatureAndServer == *currFnS)) { 
                    
                    // Send success response to server
                    response.type = LOC_SUCCESS;
                    response.addData(serializeString(string_to_cstring(serverPtr->id)));
                    response.addData(serializeInt(serverPtr->port));

                    nextServer();

                    return response.send(sock); 
                }
            }

            // Increment server iterator (round-robin)
            nextServer();

            //  Function or server not found, break
            if (*currentServer == currentServerInit) {
                break;
            }
        }
    }

    // If it gets to here, it means function was not registered 
    // for that server 
    response.type = LOC_FAILURE;

    // Add failure code (that's it?)
    response.addData(serializeInt(-1));

    return response.send(sock);
}

bool send_terminate_message_to_servers() {
    Message response;
    
    // Send termination messag to each server
    for (vector<Server*>::iterator s = registeredServers.begin(); s != registeredServers.end(); ++s) { 
        response.type = EXECUTE;
        response.send((*s)->sock);
    }   
    terminateBinder = 1;

    // Return and wait for all servers to terminate
    // Termination of binder happens in main/select, when registeredServers is empty
    return true;
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
        case TERMINATE: return locate_method_on_server(recv_message, sock);

        default:
        debug_print(("Invalid message type sent to binder: %s\n", recv_message.typeToString().c_str()));
        return false;
    }
}

int main(void) {
    int listener = open_connection();
    if(listener < 0) {
        debug_print(("Failed to create listening socket"));
        return listener;
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
                    //TODO: close client connection?
                    if (terminateBinder == 1 && registeredServers.size() == 0) return 0; 
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END main loop

    return 0;
}
