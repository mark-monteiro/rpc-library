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
#include "error_code.h"
#include "rpc_helpers.h"
#include "message.h"
#include "serialize.h"
#include "function_signature.h"
#include "arg_type.h"
#include "server.h"
#include "function_signature_and_server.h"

using namespace std;

// Keep track of registered servers and registered functions 
vector<struct Server> registeredServers;
vector<FunctionSignatureAndServer> registeredFunctions;

// Round-robin server navigation  
vector<struct Server>::iterator currentServer;

// Termination state
bool terminateBinder = false;

// Increment currentServer iterator
// Will break if currentServer points to registeredServers.end()
void nextServer(){
    currentServer++;
    if (currentServer == registeredServers.end()){
        currentServer = registeredServers.begin();
    }
}

bool register_server_and_function(Message message, int sock) {
    Server newServer;
    string newFunctionName;
    vector<ArgType> newFunctionArgTypes;
    Message response;
    int response_code = 0;
    
    // Deserialize server information
    vector<char>::iterator index = message.data.begin();
    newServer.id = deserializeString(index);
    debug_print(("identifier deserialized: %s\n", newServer.id.c_str()));
    newServer.port = deserializeInt(index);
    debug_print(("port deserialized: %d\n", newServer.port));
    newServer.sock = sock;
    
    // Try to find newServer in registeredServers (compare by socket only)
    if (find(registeredServers.begin(), registeredServers.end(), newServer) == registeredServers.end()){
        // Server not already registered; add server to registeredServers
        registeredServers.push_back(newServer);
        if (registeredServers.size() == 1){
            currentServer = registeredServers.begin();
        }
        debug_print(("registered server on socket %d\n", registeredServers.back().sock));
    }
    else debug_print(("server already registered\n"));

    // Deserialize function signature
    newFunctionName = deserializeString(index);
    debug_print(("name deserialized: %s\n", newFunctionName.c_str()));
    newFunctionArgTypes = deserializeArgTypesIntoArgTypeVector(index);
    FunctionSignatureAndServer newFnSignatureAndServer(newFunctionName, newFunctionArgTypes, newServer);

    // Try to find newFnSignatureAndServer in registeredFunctions
    if (find(registeredFunctions.begin(), registeredFunctions.end(), newFnSignatureAndServer) == registeredFunctions.end()){
        // Not found; Push FunctionSignatureAndServer to registeredFunctions vector
        registeredFunctions.push_back(newFnSignatureAndServer);
        // debug_print(("registered function:"));
        // registeredFunctions.back().print();
    }
    else {
        // This function was already registered, return a warning
        response_code = ALREADY_REGISTERED;
        debug_print(("attempt to register already registered function"));
    }

    // send register response to server
    response.type = REGISTER_RESPONSE;
    response.addData(serializeInt(response_code));
    return response.send(sock);
}

// Searches the database and return the server address
bool locate_method_on_server(Message message, int sock) {
    string functionName;
    vector<ArgType> functionArgTypes;
    Message response;
    Server currentServerInit = *currentServer;

    // Get name and argTypes from message
    vector<char>::iterator index = message.data.begin();
    functionName = deserializeString(index);
    debug_print(("name deserialized: %s\n", functionName.c_str()));
    functionArgTypes = deserializeArgTypesIntoArgTypeVector(index);

    // Loop through the servers round robin
    while(true) {
        // Make sure we have at least one server registered
        if (registeredServers.empty()) break;

        // Increment server iterator (round-robin)
        nextServer();

        // Create the query object we are looking for 
        FunctionSignatureAndServer query(functionName, functionArgTypes, *currentServer);
        
        // Search for a match in the database
        if(find(registeredFunctions.begin(), registeredFunctions.end(), query) != registeredFunctions.end()) {
            // Found a match
            debug_print(("Found a matching server for the function signature:\n"));
            currentServer->print();

            // Send success response to server
            response.type = LOC_SUCCESS;
            response.addData(serializeString(currentServer->id.c_str()));
            response.addData(serializeInt(currentServer->port));

            return response.send(sock); 
        }

        //  Function or server not found, break
        if (*currentServer == currentServerInit) {
            break;
        }
    }

    // If it gets to here, it means function was not registered
    response.type = LOC_FAILURE;
    response.addData(serializeInt(NOT_REGISTERED_ON_BINDER));
    return response.send(sock);
}

bool send_terminate_message_to_servers() {
    // Send termination message to each server
    for (int i = 0 ; i < registeredServers.size() ; i++) { 
        Message response;
        response.type = EXECUTE;
        response.send(registeredServers[i].sock);
        debug_print(("Sent termination message on socket %d\n", registeredServers[i].sock));
    }   

    // Set termination flag
    terminateBinder = true;

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
        case TERMINATE: return send_terminate_message_to_servers();

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
                // TODO: should we accept new connections while terminating?
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

                    // If all the servers have gone away, we are done terminating
                    // TODO: some clients might not have closed their connections yet...
                    if (terminateBinder && registeredServers.size() == 0) {
                        debug_print(("Termination complete...exiting\n"));
                        return 0;
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END main loop

    return 0;
}
