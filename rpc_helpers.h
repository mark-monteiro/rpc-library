#ifndef RPC_HELPERS_H
#define RPC_HELPERS_H

// TODO: get rid of this
#define MAX_CLIENTS 5

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

char* getHostname();
int getPort(int sock);

// Connects to the binder and returns a socket file descriptor
int connect_to_remote(char *hostname, char *port);

// Get the binder address from environment variables then call connect_to_remote()
int connect_to_binder();

// Create a socket, bind to it and listen for connections
// Return the socket file descriptor
int open_connection();

// wrappers for send/recieve
// retries until all 'len' bytes are sent/received
// returns 0 if the remote peer closes the connection when receiving
int send_all(int sock, char *buf, int len);

int recv_all(int sock, char *buf, int len);

#endif