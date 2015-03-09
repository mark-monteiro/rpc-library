//TODO: rename to socket_helpers

#include "rpc_helpers.h"
#include "debug.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <limits.h>     //HOST_NAME_MAX

#include <errno.h>

//TODO: switch to camelCaps

char* getHostname() {
    // Print this machine's hostname
    char hostname[HOST_NAME_MAX];
    gethostname(hostname, HOST_NAME_MAX);
    return gethostbyname(hostname)->h_name;
}

int getPort(int sock) {
    struct sockaddr_in sa;
    socklen_t sa_len = sizeof(struct sockaddr_in);

    // Print the port number we are listening on
    memset(&sa, 0, sa_len);
    if (getsockname(sock, (struct sockaddr *)&sa, &sa_len) == -1) {
        perror("getsockname");
        exit(2);
    }
    return ntohs(sa.sin_port);
}

// Connects to the binder and returns a socket file descriptor
int connect_to_remote(char *hostname, char *port) {
    int sock;  
    struct addrinfo hints, *servinfo, *p;
    int rv;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    //get list of addresses we can connect to for this destination
    if ((rv = getaddrinfo(hostname, port, &hints, &servinfo)) != 0) {
        debug_print(("connect_to_binder: getaddrinfo: %s\n", gai_strerror(rv)));
        return -1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        //create socket
        if ((sock = socket(p->ai_family, p->ai_socktype,
            p->ai_protocol)) == -1) {
            perror("connect_to_binder: failed to create socket");
            continue;
        }

        //connect to socket
        if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock);
            perror("connect_to_binder: failed to connect socket");
            continue;
        }

        break;
    }

    //make sure we connected to one of the results
    if (p == NULL) {
        debug_print(("failed to connect to binder\n"));
        return -1;
    }

    freeaddrinfo(servinfo); // all done with this structure
    return sock;
}

// Get the binder address from environment variables then call connect_to_remote()
int connect_to_binder() {
    char *binder_hostname = getenv("BINDER_ADDRESS");
    char *binder_port = getenv("BINDER_PORT");
    
    if (binder_hostname == NULL || binder_port == NULL) {
        debug_print(("connect_to_binder: BINDER_ADDRESS and BINDER_PORT environment variables must be set\n"));
        return -1;
    }

    return connect_to_remote(binder_hostname, binder_port);
}

// Create a socket, bind to it and listen for connections
// Return the socket file descriptor
int open_connection() {
    // Port binding code copied from course resources:
    // http://www.cis.temple.edu/~ingargio/old/cis307s96/readings/docs/sockets.html#Establish
    // http://www.faqs.org/docs/gazette/socket.html
    int    listener;
    struct sockaddr_in sa;

    // Set address info
    memset(&sa, 0, sizeof(struct sockaddr_in)); /* clear our address */
    sa.sin_family = AF_INET;                    /* use a internet domain */
    sa.sin_addr.s_addr = INADDR_ANY;            /* use a specific IP of host */
    sa.sin_port= htons(0);                      /* use zero for next available port */

    // Create socket
    if((listener = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    // Bind to socket
    if(bind(listener, (struct sockaddr *)&sa, sizeof(sa)) < 0) {
        perror("bind");
        return -1;
    }

    // Listen for connections
    if(listen(listener, MAX_CLIENTS) == -1) {
        perror("listen");
        return -1;
    }

    return listener;
}

// wrapper for send/recieve
// retries until all 'len' bytes are sent/received
// returns 0 if the remote peer closes the connection when receiving
int process_all(int sock, char *buf, int len, bool receiving) {
    // debug_print(("send/recv %d bytes on socket %d\n", len, sock));
    int bytes_completed = 0;  // how many bytes we've sent/received
    int bytes_left = len;     // how many we have left to send/receive
    int n;

    while(bytes_left > 0) {
        if(receiving)
            n = recv(sock, buf+bytes_completed, bytes_left, 0);
        else
            n = send(sock, buf+bytes_completed, bytes_left, 0);

        // Return on error
        if(n == -1) {
            perror("process_all");
            debug_print(("errno=%d\n", errno));
            return -1;
        }

        // Return if remote port closed connection
        if(receiving && n == 0) {
            debug_print(("Remote port closed connection on socket %d\n", sock));
            return 0;
        }

        bytes_completed += n;
        bytes_left -= n;
        // debug_print(("%s %d bytes\n", receiving ? "received" : "sent", n));
    }

    return bytes_completed;
}

int send_all(int sock, char *buf, int len) {
    return process_all(sock, buf, len, false);
}

int recv_all(int sock, char *buf, int len) {
    return process_all(sock, buf, len, true);
}