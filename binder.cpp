#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>     //HOST_NAME_MAX
#include <netinet/in.h> //sockadd_in
#include <netdb.h>      //gethostbyname
#include <algorithm>    //std::max

#include "debug.h"
#include "rpc_helpers.h"
#include "message.h"
#include "serialize.h"

using namespace std;

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
    fd_set master;    // master file descriptor list
    fd_set read_fds;  // temp file descriptor list for select()
    int fdmax;        // maximum file descriptor number

    // Initialize the master set with the listener
    FD_ZERO(&master);
    FD_SET(listener, &master);
    fdmax = listener;

    // main loop
    while(true) {
        read_fds = master; // copy it
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

                    // Add socket to master set
                    FD_SET(newfd, &master); // add to master set
                    fdmax = max(fdmax, newfd);
                    debug_print(("New connection accepted on socket %d\n", newfd));
                }
                // Handle data from a client
                else {
                    if(process_port(fd) == false) {
                        debug_print(("process_port failed; closing socket %d\n", fd));
                        close(fd);
                        FD_CLR(fd, &master);
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END main loop

    return 0;
}

bool register_server(Message message, int sock) {
    Message response;
    char *identifier, *name;
    int port, *argTypes;
    vector<char>::iterator index = message.data.begin();

    identifier = deserializeString(index);
    debug_print(("identifier deserialized: %s\n", identifier));
    port = deserializeInt(index);
    debug_print(("port deserialized: %d\n", port));
    name = deserializeString(index);
    debug_print(("name deserialized: %s\n", name));
    argTypes = deserializeArgTypes(index);

    //TODO: deserialize argTypes and add it to the database

    response.type = REGISTER_SUCCESS;
    response.data.push_back(htonl(0));
    return response.send(sock);
}

bool locate_server(Message message, int sock) {
    //TODO: search the database and return the server address
    return false;
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
        case REGISTER: return register_server(recv_message, sock);
        case LOC_REQUEST: return locate_server(recv_message, sock);

        default:
        debug_print(("Invalid message type sent to binder: %d\n", recv_message.type));
        return false;
    }
    
    return true;
}