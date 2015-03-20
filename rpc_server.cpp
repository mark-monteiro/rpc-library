#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <vector>
#include <map>
#include <set>
#include <algorithm>    //for_each

#include "debug.h"
#include "error_code.h"
#include "message.h"
#include "arg_type.h"
#include "rpc_helpers.h"
#include "serialize.h"
#include "rpc.h"
#include "function_signature.h"

using namespace std;

//TODO: maybe these shouldn't be at global scope....
bool terminate_server = false;
int binder_sock, listener_sock;
map<FunctionSignature, skeleton> function_database;

// Forward declarations
bool processPort(int sock);
bool terminateServer(int sock);
bool executeRpcCall(Message message, int sock);

int rpcInit() {
    // Create listening socket for clients
    if((listener_sock = open_connection()) < 0) {
        debug_print(("Failed to create listener socket\n"));
        return listener_sock;
    }
    debug_print(("rpcInit created listener socket on port %d\n", getPort(listener_sock)));

    // Connect to binder
    if((binder_sock = connect_to_binder()) < 0) {
        debug_print(("Failed to create binder socket\n"));
        return binder_sock;
    }
    debug_print(("rpcInit connected to binder on socket %d\n", binder_sock));

    return binder_sock;
}

int rpcRegister(char* name, int* argTypes, skeleton f) {
    // Add function name and skeleton to local database (overwrite if existing)
    // No need to check for duplicate registration here, the binder can handle that
    function_database[FunctionSignature(name, argTypes)] = f;

    Message send_message, recv_message;

    // Create REGISTER message
    send_message.type = REGISTER;
    send_message.addData(serializeString(getHostname()));
    send_message.addData(serializeInt(getPort(listener_sock)));
    send_message.addData(serializeString(name));
    send_message.addData(serializeArgTypes(argTypes));

    // Send message to binder and get response
    if(send_message.send(binder_sock) == false) return MSG_SEND_ERROR;
    if(Message::recv(binder_sock, &recv_message) == false) return MSG_RECV_ERROR;

    if(recv_message.type != REGISTER_RESPONSE) {
        debug_print(("binder did not send correct response type; expected REGISTER_RESPONSE\n"));
        return WRONG_MESSAGE_TYPE;
    }

    // Get return value from message and return it
    vector<char>::iterator index = recv_message.data.begin();
    return deserializeInt(index);
}

int rpcExecute() {
    if(function_database.empty()) return NO_REGISTERED_METHODS;
    int return_val = 0;

    // select() variables
    set<int> read_fd_set;   // ordered set; keeps track of max fd and used to close sockets
    fd_set master;          // master file descriptor list
    fd_set read_fds;        // temp file descriptor list for select()

    // Initialize the master set with the listener and binder sockets
    FD_ZERO(&master);
    FD_SET(binder_sock, &master);
    FD_SET(listener_sock, &master);
    read_fd_set.insert(binder_sock);
    read_fd_set.insert(listener_sock);

    // main loop
    while(!terminate_server) {
        read_fds = master; // copy it
        if (select((*read_fd_set.rbegin())+1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("select");
            return_val = SYS_SELECT_ERROR;
            break;
        }

        // run through the existing connections looking for data to read
        for(set<int>::iterator i = read_fd_set.begin() ; !terminate_server && i != read_fd_set.end() ; ) {
            int fd = *i;
            i++;            //Iterate at start of loop in case this socket is closed

            if (FD_ISSET(fd, &read_fds)) {
                // Handle new connections
                if (fd == listener_sock) {
                    struct sockaddr_storage remoteaddr;
                    socklen_t addr_len = sizeof remoteaddr;

                    // Accept the connection
                    int newfd = accept(listener_sock, (struct sockaddr *)&remoteaddr, &addr_len);
                    if (newfd == -1) {
                        debug_print(("failed to accept connection on socket %d\n", listener_sock));
                        perror("accept");
                        continue;
                    }

                    // Add socket to master set
                    FD_SET(newfd, &master);
                    read_fd_set.insert(newfd);
                    debug_print(("New connection accepted on socket %d\n", newfd));
                }
                // Handle data from a client
                else {
                    if(processPort(fd) == false) {
                        debug_print(("processPort failed; closing socket %d\n", fd));
                        ////close(fd);
                        FD_CLR(fd, &master);
                        read_fd_set.erase(fd);

                        // If the binder connection was lost, just die
                        if(fd == binder_sock) {
                            return_val = BINDER_DIED;
                            terminate_server = true;
                        }
                    }
                } // END handle data from client
            } // END got new incoming connection
        } // END looping through file descriptors
    } // END main loop

    // TODO: wait for all execution threads to finish

    // Close all sockets
    for_each(read_fd_set.begin(), read_fd_set.end(), close);

    return return_val;
}

bool processPort(int sock) {
    Message recv_message;
    debug_print(("--------------------------------------\n"));
    debug_print(("Processing request on socket %d\n", sock));

    // Receive message
    if(Message::recv(sock, &recv_message) == false) return false;

    if(recv_message.type == TERMINATE) return terminateServer(sock);
    // TODO: execution should happen on a new thread
    //       so that long-running methods don't block the entire server
    else if(recv_message.type == EXECUTE) return executeRpcCall(recv_message, sock);
    else {
        debug_print(("Invalid message type sent to server on socket %d: %s\n", sock, recv_message.typeToString().c_str()));
        return false;
    }
}

bool terminateServer(int sock) {
    // If this didn't come from the binder just ignore it
    // TODO: this should really check the hostname and port, in case the binder opened a new connection,
    // but since we implemented the binder we can ignore that (or maybe they will test it?)
    if(sock != binder_sock) {
        debug_print(("TERMINATE request sent from someone other than the binder...ignoring\n"));
        return true;
    }

    // Set flag for accept loop to exit
    terminate_server = true;
    return true;
}

vector<void*> allocateArgsMemory(vector<int> argTypes) {
    argTypes.pop_back();   // Remove argTypes null terminator
    vector<void*> args = vector<void*>(argTypes.size());

    for(unsigned int i = 0 ; i < argTypes.size() ; i++) {
        ArgType type = ArgType(argTypes[i]);

        switch(type.type) {
            case ARG_CHAR: args[i] = (void*)new char[type.memoryLength()]; break;
            case ARG_SHORT: args[i] = (void*)new short[type.memoryLength()]; break;
            case ARG_INT: args[i] = (void*)new int[type.memoryLength()]; break;
            case ARG_LONG: args[i] = (void*)new long[type.memoryLength()]; break;
            case ARG_DOUBLE: args[i] = (void*)new double[type.memoryLength()]; break;
            case ARG_FLOAT: args[i] = (void*)new float[type.memoryLength()]; break;
        }
    }

    return args;
}

void deleteArgsMemory(vector<int> argTypes, vector<void*> args) {
    argTypes.pop_back();   // Remove argTypes null terminator

    for(unsigned int i = 0 ; i < argTypes.size() ; i++) {
        ArgType type = ArgType(argTypes[i]);

        switch(type.type) {
            case ARG_CHAR: delete[] (char*)(args[i]); break;
            case ARG_SHORT: delete[] (short*)(args[i]); break;
            case ARG_INT: delete[] (int*)(args[i]); break;
            case ARG_LONG: delete[] (long*)(args[i]); break;
            case ARG_DOUBLE: delete[] (double*)(args[i]); break;
            case ARG_FLOAT: delete[] (float*)(args[i]); break;
        }
    }
}

bool executeRpcCall(Message recv_message, int sock) {
    string name;
    vector<int> argTypes;
    vector<void*> args, args_copy;
    vector<char>::iterator index = recv_message.data.begin();
    skeleton function_pointer;
    int return_value = 0;
    Message send_message;

    // Deserialize name, argTypes and args
    debug_print(("Deserializing message and allocating memory\n"));
    name = deserializeString(index);
    argTypes = deserializeArgTypes(index);
    args = allocateArgsMemory(argTypes);
    args_copy = args;
    deserializeArgs(&argTypes[0], true, false, &args[0], index);

    // Get function from database if it exists
    debug_print(("Getting function pointer from database\n"));
    map<FunctionSignature, skeleton>::iterator database_result;
    database_result = function_database.find(FunctionSignature((char*)name.c_str(), argTypes));

    if(database_result == function_database.end()) {
        debug_print(("Failed to find function signature in database\n"));
        return_value = NOT_REGISTERED_ON_SERVER;
    }
    else {
        // Execute function
        debug_print(("Executing function\n"));
        function_pointer = database_result->second;
        try {
            return_value = function_pointer(&argTypes[0], &args[0]);

            // Overwrite skeleton method error code with our own
            if(return_value < 0) return_value = SKELETON_ERROR;
            if(return_value > 0) return_value = SKELETON_WARNING;
        } catch (...) {
            debug_print(("skeleton function threw exception\n"));
            return_value = SKELETON_EXCEPTION;
        }
    }

    // Send response message to client
    debug_print(("Sending reply with return value %d\n", return_value));
    send_message.type = EXECUTE_SUCCESS;
    send_message.addData(serializeInt(return_value));
    send_message.addData(serializeArgs(&argTypes[0], false, true, &args[0]));
    send_message.send(sock);

    // Free memory of args; free a copy in case
    // any of the pointers were modified by the server functiion
    debug_print(("Freeing memory\n"));
    deleteArgsMemory(argTypes, args_copy);

    return true;
}
