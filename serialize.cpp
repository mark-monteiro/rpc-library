#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string>
#include <vector>

#include "debug.h"

using namespace std;

// ** int ** //
vector<char> serializeInt(int data) {
    vector<char> serialized = vector<char>(4);
    uint32_t network_byte_order = htonl((uint32_t)data);

    serialized[0] = (network_byte_order >> 24) & 0xFF;
    serialized[1] = (network_byte_order >> 16) & 0xFF;
    serialized[2] = (network_byte_order >> 8) & 0xFF;
    serialized[3] = network_byte_order & 0xFF;
    debug_print(("serialized bytes %x,%x,%x,%x\n", serialized[0],serialized[1],serialized[2],serialized[3]));
    return serialized;
}
int deserializeInt(vector<char>::iterator &buffer) {
    uint32_t network_byte_order = (buffer[0] << 24) + (buffer[1] << 16) + (buffer[2] << 8) + buffer[3];
    debug_print(("deserializing bytes %x,%x,%x,%x\n", buffer[0],buffer[1],buffer[2],buffer[3]));
    buffer += 4;
    debug_print(("deserialized int %d", ntohl(network_byte_order)));
    return (int)ntohl(network_byte_order);
}

// ** string ** //
vector<char> serializeString(char *data) {
    // NOTE: must include null terminator for deserialization
    return vector<char>(data, data + strlen(data) + 1);
}
const char* deserializeString(vector<char>::iterator &buffer){
    // NOTE: this assumes the string is null terminated!
    string deserialized = std::string(&*buffer);
    buffer += deserialized.length() + 1;
    return deserialized.c_str();
}

// ** argTypes ** //
// Prepend array length to simplify allocating memory when deserializing
// Note: this method will break for more than INT_MAX arguments
//       since we serialize the array length with only one byte
vector<char> serializeArgTypes(int *data) {
    uint32_t size = 0;
    vector<char> buffer, serializedSize, serializedData;

    // Determine argType size
    while(data[size] != 0) size++;
    size += 2;   //add space for prepended length and trailing zero
    serializedSize = serializeInt(size);
    buffer = vector<char>(size * 4);

    // Serialize data
    debug_print(("size: %X, serializedSize: %X\n", size, *((int*)&serializedSize)));
    buffer.insert(buffer.end(), serializedSize.begin(), serializedSize.end()); //include array length first
    for(int i = 1 ; i < size ; i++) {
        serializedData = serializeInt(data[i-1]);
        buffer.insert(serializedData.end(), serializedData.begin(), serializedData.end());
    }

    return buffer;
}
int* deserializeArgTypes(vector<char>::iterator &buffer) {
    // Get length and create vector
    int length = deserializeInt(buffer);
    vector<int> argTypes = vector<int>(length);

    debug_print(("deserializzing %d argTypes\n", length));

    // Deserialize array
    for(int i = 0 ; i < length ; i++) {
        argTypes[i] = deserializeInt(buffer);
    }

    assert(length > 0 && argTypes[length-1] == 0);
    return &argTypes[0];
}

vector<char> serializeArgs(int **data);
int** deserializeArgs(vector<char>::iterator &buffer);

#endif