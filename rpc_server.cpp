#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <vector>
#include <map>

#include "debug.h"
#include "message.h"
#include "rpc_helpers.h"
#include "serialize.h"
#include "rpc.h"
#include "function_signature.h"

using namespace std;

//TODO: maybe these shouldn't be at global scope....
bool terminate = false;
int binder_sock, listener_sock;
map<FunctionSignature, skeleton> function_database;

// Forward declarations
bool processPort(int sock);
bool terminateServer();
bool executeRpc(Message message, int sock);

int rpcInit() {
    // Create listening socket for clients
    if((listener_sock = open_connection()) == -1) {
        debug_print(("Failed to create listener socket"));
        return -1;
    }
    debug_print(("rpcInit created listener socket on port %d\n", getPort(listener_sock)));

    // Connect to binder
    if((binder_sock = connect_to_binder()) == -1) {
        debug_print(("Failed to create binder socket"));
        return -1;
    }
    debug_print(("rpcInit connected to binder on socket %d\n", binder_sock));

    return binder_sock;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
    // Add function name and skeleton to local database (overwrite if existing)
    function_database[FunctionSignature(name, argTypes)] = f;

    Message send_message, recv_message;

    // Create REGISTER message
    send_message.type = REGISTER;
    send_message.addData(serializeString(getHostname()));
    send_message.addData(serializeInt(getPort(listener_sock)));
    send_message.addData(serializeString(name));
    send_message.addData(serializeArgTypes(argTypes));

    // Send message to binder and get response
    if(send_message.send(binder_sock) == false) return -1;
    if(Message::recv(binder_sock, &recv_message) == false) return -1;

    if(recv_message.type != REGISTER_RESPONSE) {
        debug_print(("binder did not send correct response type; expected REGISTER_RESPONSE\n"));
        return -1;
    }

    // Get return value from message and return it
    vector<char>::iterator index = recv_message.data.begin();
    return deserializeInt(index);
}

int rpcExecute() {
    // select() variables
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    // Initialize the master set with the listener and binder sockets
    FD_ZERO(&master);
    FD_SET(binder_sock, &master);
    FD_SET(listener_sock, &master);
    fdmax = max(listener_sock, binder_sock);

    // main loop
    while(!terminate) {
        read_fds = master; // copy it
        if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            exit(4);
        }

        // run through the existing connections looking for data to read
        for(int fd = 0; fd <= fdmax; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                // Handle new connections
                if (fd == listener_sock) {
                    struct sockaddr_storage remoteaddr;
                    socklen_t addr_len = sizeof remoteaddr;

                    // Accept the connection
                    int newfd = accept(listener_sock, (struct sockaddr *)&remoteaddr, &addr_len);
                    if (newfd == -1) {
                        perror("accept");
                        continue;
                    }

                    // Add socket to master set
                    FD_SET(newfd, &master); // add to master set
                    fdmax = max(fdmax, newfd);
                    debug_print(("New connection accepted on socket %d\n", newfd));
                }
                // Handle data from a client
                else {
                    if(processPort(fd) == false) {
                        debug_print(("processPort failed; closing socket %d\n", fd));
                        close(fd);
                        FD_CLR(fd, &master);
                        //TODO: exit gracefully
                        if(fd == binder_sock) exit(-1);
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END main loop

    // TODO: wait for all execution threads to finish
    // TODO: close all sockets
    // TODO: send termination response to binder? Do we need to?

    return 0;
}

bool processPort(int sock) {
    Message recv_message;
    debug_print(("--------------------------------------\n"));
    debug_print(("Processing request on socket %d\n", sock));

    // Receive message
    if(Message::recv(sock, &recv_message) == false) return false;

    if(sock == binder_sock && recv_message.type == EXECUTE) return terminateServer();
    // TODO: execution should happen on a new thread
    else if(recv_message.type == EXECUTE) return executeRpc(recv_message, sock);
    else {
        debug_print(("Invalid message type sent to server on socket %d: %s\n", sock, recv_message.typeToString()));
        return false;
    }
}

bool terminateServer() {
    // Set flag for accept loop to exit
    terminate = true;
    return true;
}

bool executeRpc(Message message, int sock) {
    return -1;
}
