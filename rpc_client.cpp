#include <stdlib.h>
#include <unistd.h>

#include "error_code.h"
#include "rpc_helpers.h"
#include "serialize.h"
#include "message.h"
#include "debug.h"

#include "rpc.h"

using namespace::std;

// Get the address of a suitible server from the binder,
// open a connection to the server and return a socket file descriptor
// or an error code <1 if something went wrong
int rpcLocRequest(char* name, int* argTypes) {
    int binder_sock, server_sock;
    char *server_hostname, *server_port;
    Message send_message, recv_message;

    // Connect to the binder
    if((binder_sock = connect_to_binder()) < 0) {
        debug_print(("Failed to create binder socket"));
        return binder_sock;
    }
    debug_print(("rpcLocRequest connected to binder on socket %d\n", binder_sock));

    // Create LOC_REQUEST message
    send_message.type = LOC_REQUEST;
    send_message.addData(serializeString(name));
    send_message.addData(serializeArgTypes(argTypes));

    // Send message and recv response
    if(send_message.send(binder_sock) == false) return MSG_SEND_ERROR;
    if(Message::recv(binder_sock, &recv_message) == false) return MSG_RECV_ERROR;
    vector<char>::iterator index = recv_message.data.begin();
    close(binder_sock);

    // Check for request failure
    if(recv_message.type == LOC_FAILURE) {
        int reason_code = deserializeInt(index);
        debug_print(("LOC_REQUEST failed with reason code %d\n", reason_code));
        return reason_code;
    }
    else if(recv_message.type != LOC_SUCCESS) {
        debug_print(("Binder responded wih invalid message type to LOC_REQUEST: %s\n", recv_message.typeToString().c_str()));
        return WRONG_MESSAGE_TYPE;
    }

    // Deserialize response
    server_hostname = (char*)deserializeString(index).c_str();
    server_port = (char*)deserializeString(index).c_str();

    // Connect to server
    if((server_sock = connect_to_remote(server_hostname, server_port)) < 0) {
        debug_print(("Failed to create server socket"));
        return server_sock;
    }
    debug_print(("rpcLocRequest connected to server on socket %d\n", server_sock));

    return server_sock;
}

// Note: We create a new socket for every call because then we don't have to worry about
//       messages returning out-of-order for parallel calls to the same server;
//       each call has it's own socket so it can only receive its own response
int rpcCall(char* name, int* argTypes, void** args) {
    int server_sock;
    Message send_message, recv_message;

    // Find a server and open a connection to it
    server_sock = rpcLocRequest(name, argTypes);
    if(server_sock < 0) return server_sock;

    // Create REGISTER message
    send_message.type = EXECUTE;
    send_message.addData(serializeString(name));
    send_message.addData(serializeArgTypes(argTypes));
    send_message.addData(serializeArgs(argTypes, true, false, args));

    // Send message to binder and get response
    if(send_message.send(server_sock) == false) return MSG_SEND_ERROR;
    if(Message::recv(server_sock, &recv_message) == false) return MSG_RECV_ERROR;

    // Deserialize the response
    vector<char>::iterator index = recv_message.data.begin();
    int return_value = deserializeInt(index);
    deserializeArgs(argTypes, false, true, args, index);

    close(server_sock);
    return return_value;
}

int rpcCacheCall(char* name, int* argTypes, void** args) {
    return rpcCall(name, argTypes, args);
}

int rpcTerminate() {

    return -1;
}
