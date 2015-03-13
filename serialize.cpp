#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <vector>

// #include "debug.h"

using namespace std;

# define debug_print(x) do {} while (0)


// ** string ** //
vector<char> serializeString(char *data) {
    // NOTE: must include null terminator for deserialization
    return vector<char>(data, data + strlen(data) + 1);
}
string deserializeString(vector<char>::iterator &buffer){
    // NOTE: this assumes the string is null terminated!
    string deserialized = string(&*buffer);
    buffer += deserialized.length() + 1;
    return deserialized;
}

// ** LONG ** //
vector<char> serializeLong(long data) {
    ostringstream ss;
    ss << data;
    return serializeString((char*)(ss.str().c_str()));
}
long deserializeLong(vector<char>::iterator &buffer) {
    return atol(deserializeString(buffer).c_str());
}

// ** INT ** //
vector<char> serializeInt(int data) {
    return serializeLong((long)data);
}
int deserializeInt(vector<char>::iterator &buffer) {
    return atoi(deserializeString(buffer).c_str());
}

// ** SHORT ** //
vector<char> serializeShort(short data) {
    return serializeLong((long)data);
}
short deserializeShort(vector<char>::iterator &buffer) {
    return (short)deserializeInt(buffer);
}

// ** DOUBLE ** //
vector<char> serializeDouble(double data) {
    ostringstream ss;
    ss << data;
    return serializeString((char*)(ss.str().c_str()));
}
double deserializeDouble(vector<char>::iterator &buffer) {
    return atof(deserializeString(buffer).c_str());
}

// ** FLOAT ** //
vector<char> serializeFloat(float data) {
    return serializeDouble((double)data);
}
float deserializeFloat(vector<char>::iterator &buffer) {
    return (float)deserializeDouble(buffer);
}

// ** argTypes ** //
// TODO: prepend length of the array to save time on memory allocation
vector<char> serializeArgTypes(int *data) {
    vector<char> buffer, serializedData;

    // Serialize data
    for(int i = 0 ; i == 0 || data[i-1] != 0 ; i++) {
        serializedData = serializeInt(data[i]);
        buffer.insert(buffer.end(), serializedData.begin(), serializedData.end());
    }

    return buffer;
}
int* deserializeArgTypes(vector<char>::iterator &buffer) {
    vector<int> argTypes;

    // Deserialize array
    while(true) {
        argTypes.push_back(deserializeInt(buffer));
        if(argTypes.back() == 0) break;
    }

    return &argTypes[0];
}
vector<int> deserializeArgTypesIntoVector(vector<char>::iterator &buffer) {
    vector<int> argTypes;

    // Deserialize array
    while(true) {
        argTypes.push_back(deserializeInt(buffer));
        if(argTypes.back() == 0) break;
    }

    return argTypes;
}


// TODO: implement this
vector<char> serializeArgs(int **data);
int** deserializeArgs(vector<char>::iterator &buffer);

#endif