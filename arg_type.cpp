#include <algorithm>

#include "debug.h"
#include "rpc.h"

#include "arg_type.h"

ArgType::ArgType(int typeData) {
    input =  ((typeData >>  ARG_INPUT) & 1) == 1;   //shift right 31 bits then get the last bit
    output = ((typeData >> ARG_OUTPUT) & 1) == 1;   //shift right 30 bits then get the last bit
    //TODO: assert bits 29-23 are all zero
    type = (typeData >> 16) & 0xFF;                 //shift right 16 bits then get rightmost byte
    arrayLength = typeData & 0xFFFF;                //get rightmost two bytes
}

int ArgType::toInt() {
    return (input << ARG_INPUT) | (output << ARG_OUTPUT) | type << 16 | arrayLength;
}

// If the array length is set to zero, we have a scalar
// which is the same as array of length one
short ArgType::memoryLength() {
    return std::max(arrayLength, (short)1);
}

bool ArgType::isScalar() const {
    return arrayLength == 0;
}

bool ArgType::isArray() const {
    return arrayLength != 0;
}

// Note: ignores array length, but does differentiate
// between scalar and vector arguments
bool ArgType::operator<(const ArgType &other) const {
    if(input != other.input) return input < other.input;
    else if (output != other.output) return output < other.output;
    else if (type != other.type) return type < other.type;
    else return isScalar() < other.isScalar();
}

bool ArgType::operator==(const ArgType &other) const {
    return (input == other.input) && (output == other.output) && (type == other.type);
}

bool ArgType::operator!=(const ArgType &other) const {
    return (input != other.input) || (output != other.output) || (type != other.type);
}

void ArgType::print() const {
    debug_print(("\tinput:%d", input));
    debug_print(("\toutput:%d", output));
    debug_print(("\targType:%d", type));
    debug_print(("\tarrayLength:%d\n", arrayLength));
}

// ArgTypeList::ArgTypeList(int *args) : List<ArgType>() {
//     // Get the number of arguments
//     numArgs = 0;
//     while(args[numArgs] != 0) numArgs++;

//     // Create the vector and populate it
//     this->resize(numArgs);
//     for(int i = 0 ; i < numArgs ; i++) {
//         (*this)[i] = ArgType(args[i]);
//     }
// }
