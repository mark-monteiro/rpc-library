#include "debug.h"
#include "message.h"
#include "rpc_helpers.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <vector>

using namespace std;

// The length of the message is defined by the size of the vector
uint32_t Message::length() {
    return data.size();
}

// Append new vector to data buffer
void Message::addData(vector<char> newData) {
    data.insert(data.end(), newData.begin(), newData.end());
}

void Message::addData(char newData) {
    data.push_back(newData);
}

// Send the message over the specified socket
bool Message::send(int sock) {
    uint32_t network_byte_order;

    // Write message length
    network_byte_order = htonl(length());
    if(send_all(sock, (char*)&network_byte_order, 4) < 0) return false;

    // Write message type
    network_byte_order = htonl(type);
    if(send_all(sock, (char*)&network_byte_order, 4) < 0) return false;

    // Write Data
    if(length() > 0 && send_all(sock, (char*)&data[0], length()) < 0) return false;

    debug_print(("Sent message successfully:\n"));
    print();
    return true;
}

// Create a message by reading from  the specified socket and return it
// Return null pointer on failure
bool Message::recv(int sock, Message *message) {
    char header[4];

    // Read message length
    if(recv_all(sock, header, 4) <= 0) return false;
    message->data.resize(ntohl(*(uint32_t*)header));

    // Read message type
    if(recv_all(sock, header, 4) <= 0) return false;
    message->type = (Msg_Type)ntohl(*(uint32_t*)header);

    // Read request body
    if(message->length() > 0) {
        if(recv_all(sock, &(message->data[0]), message->length()) <= 0) return false;
    }

    debug_print(("Received message successfully:\n"));
    message->print();
    return true;
}

string Message::typeToString() {
    switch(type) {
        case REGISTER: return string("REGISTER");
        case REGISTER_RESPONSE: return string("REGISTER_RESPONSE");
        case LOC_REQUEST: return string("LOC_REQUEST");
        case LOC_SUCCESS: return string("LOC_SUCCESS");
        case LOC_FAILURE: return string("LOC_FAILURE");
        case EXECUTE: return string("EXECUTE");
        case EXECUTE_SUCCESS: return string("EXECUTE_SUCCESS");
        case EXECUTE_FAILURE: return string("EXECUTE_FAILURE");
        case TERMINATE: return string("TERMINATE");
        default: return string("<unkown type>");
    }
}

string Message::dataToString(int startIndex) {
    vector<char> data_copy = data;
    //replace null terminators with '|'
    for(unsigned int i = startIndex ; i < length() ; i++)
        if(data_copy[i] == '\0') data_copy[i] = '|';
    return std::string(data_copy.begin(), data_copy.end());
}

void Message::print() {
    debug_print(("\tlength: %d\n", length()));
    debug_print(("\ttype: %s\n", typeToString().c_str()));
    //TODO: print data better
    debug_print(("\tdata: %s\n", dataToString().c_str()));
}
