#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>     //HOST_NAME_MAX
#include <netinet/in.h> //sockadd_in
#include <netdb.h>      //gethostbyname
#include <algorithm>    //max
#include <map>          //map
#include <utility>      //std::std::get, std::pair
#include <stack>        // std::stack

#include "debug.h"
#include "rpc_helpers.h"
#include "message.h"
#include "serialize.h"
#include "binder.h"
// #include "debug.h"
#include "function_signature.h"

# define debug_print(x) do {} while (0)

using namespace std;
vector<struct server*> registeredServers;
vector< pair<struct FunctionSignature, struct server*> > registeredFunctions;
std::stack<struct server*> recentlyCalledServers;

bool process_port(int sock);

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
    // char *identifier, *name;
    // int *argTypes;
    
    struct server newServer;

    // Register server if it is not already registered
    for (vector<struct server*>::iterator it = registeredServers.begin(); it != registeredServers.end(); ++it) {
        if (sock == (*it)->sock){
            debug_print(("server previously registered, sockfd: %s\n", sock));
            break;
        }

         // Server not already registered, add server to registeredServers
        if (it == registeredServers.end()){
            // Get id, port and sock from message
            vector<char>::iterator index = message.data.begin();
            newServer.id = deserializeString(index);
            debug_print(("identifier deserialized: %s\n", newServer.id));
            newServer.port = deserializeInt(index);
            debug_print(("port deserialized: %d\n", newServer.port));
            newServer.sock = sock;

            // Add server to the registeredServers vector
            registeredServers.push_back(&newServer);
        }
    }
        
    struct FunctionSignature newFunctionSig = FunctionSignature(NULL, NULL); // TODO: replace with (deserializeString(index), deserializeArgTypes(index))
    debug_print(("name deserialized: %s\n", newFunctionSig.name));

    // If (FunctionSignature, server) pair does not exist in registeredFunctions vector
    // push pair into registeredFunctions vector
    for (vector< pair<struct FunctionSignature, struct server*> >::iterator it = registeredFunctions.begin(); it != registeredFunctions.end(); ++it) {
        // TODO: fix comparison of functions
        char *tmpName = const_cast<char*> (((*it).first).name.c_str());
        int *tmpArgTypes = &(((*it).first).argTypes)[0];
        if ((&(newFunctionSig.name)[0] == tmpName) && (&(newFunctionSig.argTypes)[0] == tmpArgTypes)) {
            debug_print(("attempt to register already registered function"));
            break;
        }
        // Push (FunctionSignature, server) pair to registeredFunctions vector
        if (it == registeredFunctions.end()) registeredFunctions.push_back(make_pair(newFunctionSig, &newServer));
    }

    // send register response to server
    response.type = REGISTER_RESPONSE;
    response.addData(serializeInt(0));
    return response.send(sock);
}

bool locate_method_on_server(Message message, int sock) {
    // Searches the database and return the server address
    struct server* serverPtr = NULL;
    char *name;
    int *argTypes;
    Message response;

    // Loop through registered servers to find the one with socket in message
    for (vector<struct server*>::iterator it = registeredServers.begin(); it != registeredServers.end(); ++it) {
        if (sock == (*it)->sock) {serverPtr = (server*)(&(*it)); break;}
    }

     // Server not found
    if (!serverPtr) return false;

    // Server found - Get name and argTypes from message
    vector<char>::iterator index = message.data.begin();
    name = deserializeString(index);
    debug_print(("name deserialized: %s\n", name));
    argTypes = deserializeArgTypes(index);

    // Locate method in registeredFunctions vectpr
    for (vector< pair<struct FunctionSignature, struct server*> >::iterator it = registeredFunctions.begin(); it != registeredFunctions.end(); ++it) {
        char *tmpName = const_cast<char*>(((*it).first).name.c_str());
        int *tmpArgTypes = &(((*it).first).argTypes)[0];

        if ((name == tmpName) && (argTypes == (int *)tmpArgTypes)) {
            // Push to recentlyCalledServers queue
            // recentlyCalledServers.push(serverPtr);
            
            // Send success response to server
            response.type = LOC_SUCCESS;
            response.addData(serializeString(serverPtr->id));
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
