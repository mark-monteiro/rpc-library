#include <stdlib.h>
#include <unistd.h>

#include "rpc_helpers.h"
#include "serialize.h"
#include "message.h"
#include "debug.h"

#include "rpc.h"

using namespace::std;

int rpcCall(char* name, int* argTypes, void** args) {
    // NOTE: if the client sends multiple rpc calls in parallel
    // the response messages may not return in-order.
    // The first call should spawn an accepting thread that reads
    // response messages and signals the blocked calling thread when the response arrives.
    // Each rpc call should send an id# (i.e. thread id) with the message,
    // (also returned with the server's response message) that is used to signal
    // the blocked thread

    //TODO: get these from the binder
    char *server_hostname = getenv("SERVER_ADDRESS");
    char *server_port = getenv("SERVER_PORT");
    int server_sock;

    // Connect to server
    if((server_sock = connect_to_remote(server_hostname, server_port)) == -1) {
        debug_print(("Failed to create server socket\n"));
        return -1;
    }
    debug_print(("rpcInit connected to server on socket %d\n", server_sock));

    Message send_message, recv_message;

    // Create REGISTER message
    send_message.type = EXECUTE;
    send_message.addData(serializeString(name));
    send_message.addData(serializeArgTypes(argTypes));
    send_message.addData(serializeArgs(argTypes, true, false, args));

    // Send message to binder and get response
    if(send_message.send(server_sock) == false) return -1;
    if(Message::recv(server_sock, &recv_message) == false) return -1;

    // Deserialize the response
    vector<char>::iterator index = recv_message.data.begin();
    int return_value = deserializeInt(index);
    deserializeArgs(argTypes, false, true, args, index);

    close(server_sock);
    return return_value;
}

int rpcCacheCall(char* name, int* argTypes, void** args) {
    return -1;
}

int rpcTerminate() {
    int binder_sock;
    Message terminate_message;

    // Connect to binder
    if((binder_sock = connect_to_binder()) == -1) {
        debug_print(("Failed to create binder socket\n"));
        return -1;
    }
    debug_print(("rpcTerminate connected to binder on socket %d\n", binder_sock));

    // Create and send message to binder
    terminate_message.type = TERMINATE;
    if(terminate_message.send(binder_sock) == false) return -1;

    // Close socket and return
    close(binder_sock);
    return 0;
}
