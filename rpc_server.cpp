#include <stdio.h>
#include <arpa/inet.h>
#include <vector>

#include "debug.h"
#include "message.h"
#include "rpc_helpers.h"
#include "serialize.h"
#include "rpc.h"

//TODO: maybe these shouldn't be at global scope....
int binder_sock, listener_sock;

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
    //TODO: save the skeleton to some data structure
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

    // Get return value from message and return it
    std::vector<char>::iterator index = recv_message.data.begin();
    return deserializeInt(index);
}

int rpcExecute() {
    //TODO: accept connections on listener socket
    //      accept connection from binder, waiting for terminate message
    return -1;
}