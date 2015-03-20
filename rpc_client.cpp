#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <map>
#include <set>
#include <utility>  //pair

#include "error_code.h"
#include "function_signature.h"
#include "rpc_helpers.h"
#include "serialize.h"
#include "message.h"
#include "debug.h"

#include "rpc.h"

using namespace::std;

// Forward Declarations
int rpcLocRequest(char* name, int* argTypes, bool cache);
int cacheConnect(char* name, int* argTypes);
int rpcExecute(int server_sock, char* name, int* argTypes, void** args);


// TODO: make this global shit thread-safe

// Each server is represented by a pair of strings: <hostname, port>
// Local cache maps a server to a set of functions it supports
// rrIterator iterates over the map to return servers in a round-robin fashion
typedef pair<string, string> server;
typedef map<server, set<FunctionSignature> > cache;
cache serverCache;
cache::iterator rrIterator = serverCache.begin();

// Note: We create a new socket for every call because then we don't have to worry about
//       messages returning out-of-order for parallel calls to the same server;
//       each call has it's own socket so it can only receive its own response
int rpcCall(char* name, int* argTypes, void** args) {
    int server_sock;

    // Find a server and open a connection to it
    server_sock = rpcLocRequest(name, argTypes, false);
    if(server_sock < 0) return server_sock;

    // Execute the method on the server
    return rpcExecute(server_sock, name, argTypes, args);
}

int rpcCacheCall(char* name, int* argTypes, void** args) {
    // Attempt to connect on a cache-hit
    int server_sock = cacheConnect(name, argTypes);

    if(server_sock < 0) {
        // No server in cache, get server from binder
        server_sock = rpcLocRequest(name, argTypes, true);
        if(server_sock < 0) return server_sock;
    }

    // Execute the method on the server
    // TODO: if server returns a NOT_REGISTERED error, (could happen if server restarts on same port)
    //       remove that function from cache and try again (recurse)
    return rpcExecute(server_sock, name, argTypes, args);
}

// Get the location(s) of a suitible server from the binder,
// a connects to the next suitable server returns a socket file descriptor
// or an error code <1 if something went wrong
// If cache = true, the cache is also updated with returned results
int rpcLocRequest(char* name, int* argTypes, bool cache = false) {
    int binder_sock, server_sock, numServers;
    string server_hostname, server_port;
    Message send_message, recv_message;

    // Connect to the binder
    if((binder_sock = connect_to_binder()) < 0) {
        debug_print(("Failed to create binder socket"));
        return binder_sock;
    }
    debug_print(("rpcLocRequest connected to binder on socket %d\n", binder_sock));

    // Create LOC_REQUEST message
    send_message.type = cache ? LOC_CACHE_REQUEST : LOC_REQUEST;
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

    // Get the number of servers returned
    // Note: we should probably check for length=0 here,
    // but for now we trust the binder to return LOC_FAILURE in this case
    if(cache) numServers = deserializeInt(index);
    else numServers = 1;

    // Deserialize the servers
    for(int i = 0 ; i < numServers ; i++) {
        server_hostname = deserializeString(index);
        server_port = deserializeString(index);
        
        // Put the results in the cache
        if(cache) {
            serverCache[make_pair(server_hostname, server_port)].insert(FunctionSignature(string(name), argTypes));
            debug_print(("inserted function to cache: %s\n", name));
        }
    }

    // Connect to the next server in the cache (round-robin)
    if(cache) server_sock = cacheConnect(name, argTypes);
    // Connect to the server returned by the binder
    else server_sock = connect_to_remote((char*)server_hostname.c_str(), (char*)server_port.c_str());

    // Return the socket
    if(server_sock < 0) debug_print(("rpcLocRequest Failed to connect to server\n"));
    else debug_print(("rpcLocRequest connected to server on socket %d\n", server_sock));
    return server_sock;
}

// Seach the cache until we find a hit and that successfuly connects
// Return the connection socket
int cacheConnect(char* name, int* argTypes) {
    //make sure the cache isn't empty and we have a valid iterator
    if(serverCache.empty()) return NOT_REGISTERED_ON_BINDER;
    if(rrIterator == serverCache.end()) rrIterator = serverCache.begin();
    
    FunctionSignature query = FunctionSignature(string(name), argTypes);    
    server searchStart = rrIterator->first;

    // Search the cache
    do {
        // Increment the round-robin iterator (cyclically)
        rrIterator++;
        if(rrIterator == serverCache.end()) rrIterator = serverCache.begin();

        // Check if this server has the required functions in the cache
        set<FunctionSignature> methods = rrIterator->second;
        if(methods.find(query) == methods.end()) continue;

        // Get the server details
        server cacheResult = rrIterator->first;
        string server_hostname = cacheResult.first;
        string server_port = cacheResult.second;

        // Connect to the server and return the socket
        int server_sock = connect_to_remote((char*)server_hostname.c_str(), (char*)server_port.c_str());
        if(server_sock >= 0) {
            debug_print(("cacheConnect connected to %s:%s\n", server_hostname.c_str(), server_port.c_str()));
            return server_sock;
        }

        // Connection failed, remove this server from the cache
        else {
            debug_print(("cacheConnect failed. Removing %s:%s from cache\n", server_hostname.c_str(), server_port.c_str()));

            // If the last server in cache; just reset the entire thing and fail
            if(serverCache.size() == 1) {
                serverCache.clear();
                rrIterator = serverCache.begin();
                return -1;
            }

            if(rrIterator == serverCache.begin()) rrIterator = serverCache.end();
            rrIterator--; //decrement rrIterator

            //delete this server from the cache
            serverCache.erase(make_pair(server_hostname, server_port));
        }
    } while(rrIterator->first != searchStart);

    debug_print(("Method not found in cache\n"));
    return -1;
}

// Sends an execute message to the specified server,
// Deserializes the response, and returns the return code
int rpcExecute(int server_sock, char* name, int* argTypes, void** args) {
    Message send_message, recv_message;

    // Create EXECUTE message
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

int rpcTerminate() {
    int binder_sock;
    Message terminate_message;

    // Connect to binder
    if((binder_sock = connect_to_binder()) < 0) {
        debug_print(("Failed to create binder socket\n"));
        return binder_sock;
    }
    debug_print(("rpcTerminate connected to binder on socket %d\n", binder_sock));

    // Create and send message to binder
    terminate_message.type = TERMINATE;
    if(terminate_message.send(binder_sock) == false) return MSG_SEND_ERROR;

    // Close socket and return
    close(binder_sock);
    return 0;
}
