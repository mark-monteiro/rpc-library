#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdlib.h>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include "debug.h"
#include "arg_type.h"
#include "rpc.h"

#include "serialize.h"

using namespace std;

// ** string ** //
// NOTE: this method only works for null terminated string;
// any string with multiple null character will be cut off at the first one
vector<char> serializeString(char *data) {
    // NOTE: must include null terminator for deserialization
    return vector<char>(data, data + strlen(data) + 1);
}
string deserializeString(vector<char>::iterator &buffer){
    // NOTE: this assumes the string is null terminated!
    string deserialized = string(&buffer[0]);
    buffer += deserialized.length() + 1;
    return deserialized;
}

vector<char> serializeChar(char data) {
    vector<char> buffer = vector<char>(1);
    buffer[0] = data;
    return buffer;
}
char deserializeChar(vector<char>::iterator &buffer) {
    char data = buffer[0];
    buffer++;
    return data;
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
    for(int i = 0 ; ; i++) {
        serializedData = serializeInt(data[i]);
        buffer.insert(buffer.end(), serializedData.begin(), serializedData.end());
        if(data[i] == 0) break;
    }

    return buffer;
}

vector<int> deserializeArgTypes(vector<char>::iterator &buffer) {
    vector<int> argTypes;
    // Deserialize array
    while(true) {
        //TODO: the byte order or the argtype might get fucked up by endianess with this method
        // sol'n: serialize each part of the arg type seperately in its own serialization method

        argTypes.push_back(deserializeInt(buffer));
        if(argTypes.back() == 0) break;
    }

    return argTypes;
}

vector<ArgType> deserializeArgTypesIntoArgTypeVector(vector<char>::iterator &buffer) {
    vector<ArgType> argTypes;
    // Deserialize array
    while(true) {
        //TODO: the byte order or the argtype might get fucked up by endianess with this method
        // sol'n: serialize each part of the arg type seperately in its own serialization method
        argTypes.push_back(ArgType(deserializeInt(buffer)));
        argTypes.push_back(deserializeInt(buffer));
        if(argTypes.back() == 0) break;
    }

    return argTypes;
}

vector<char> serializeArgs(int *argTypes, bool inputs, bool outputs, void **data) {
    vector<char> buffer;

    // Get argTypes size
    int numArgs = 0;
    while(argTypes[numArgs] != 0) numArgs++;

    // Iterate through each argument
    for(int i = 0 ; i < numArgs ; i++) {
        // Skip input or output vars depending on what was specified
        ArgType type = ArgType(argTypes[i]);
        if(!((type.input && inputs) || (type.output && outputs))) continue;

        // Iterate through each array element for the argument
        for(short j = 0 ; j < type.memoryLength() ; j++) {
            vector<char> serializedData;

            // Serialize the value
            switch(type.type) {
                case ARG_CHAR: serializedData = serializeChar(((char*)data[i])[j]); break;
                case ARG_SHORT: serializedData = serializeShort(((short*)data[i])[j]); break;
                case ARG_INT: serializedData = serializeInt(((int*)data[i])[j]); break;
                case ARG_LONG: serializedData = serializeLong(((long*)data[i])[j]); break;
                case ARG_DOUBLE: serializedData = serializeDouble(((double*)data[i])[j]); break;
                case ARG_FLOAT: serializedData = serializeFloat(((float*)data[i])[j]); break;
            }

            // Append it to the buffer
            buffer.insert(buffer.end(), serializedData.begin(), serializedData.end());
        }
    }

    return buffer;
}

void deserializeArgs(int *argTypes, bool inputs, bool outputs, void **args, vector<char>::iterator &buffer) {
    // Get argTypes size
    int numArgs = 0;
    while(argTypes[numArgs] != 0) numArgs++;

    // Iterate through each argument
    for(int i = 0 ; i < numArgs ; i++) {
        ArgType type = ArgType(argTypes[i]);
        if(!((type.input && inputs) || (type.output && outputs))) continue;

        // Deserialize the values and store in previousl allocated memory
        switch(type.type) {
            case ARG_CHAR: {
                char* argArray = (char*)args[i];
                for(short k = 0 ; k < type.memoryLength() ; k++) argArray[k] = deserializeChar(buffer);
                break;
            }
            case ARG_SHORT: {
                short* argArray = (short*)args[i];
                for(short k = 0 ; k < type.memoryLength() ; k++) argArray[k] = deserializeShort(buffer);
                break;
            }
            case ARG_INT: {
                int* argArray = (int*)args[i];
                for(short k = 0 ; k < type.memoryLength() ; k++) {
                    argArray[k] = deserializeInt(buffer);
                }
                break;
            }
            case ARG_LONG: {
                long* argArray = (long*)args[i];
                for(short k = 0 ; k < type.memoryLength() ; k++) argArray[k] = deserializeLong(buffer);
                break;
            }
            case ARG_DOUBLE: {
                double* argArray = (double*)args[i];
                for(short k = 0 ; k < type.memoryLength() ; k++) argArray[k] = deserializeDouble(buffer);
                break;
            }
            case ARG_FLOAT: {
                float* argArray = (float*)args[i];
                for(short k = 0 ; k < type.memoryLength() ; k++) argArray[k] = deserializeFloat(buffer);
                break;
            }
        } //SWITCH
    } //FOR
}
