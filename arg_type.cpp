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

// Note: ignores array length
bool ArgType::operator<(const ArgType &other) const {
    if(input != other.input) return input < other.input;
    else if (output != other.output) return output < other.output;
    else return type < other.type;
}

void ArgType::print() const {
    debug_print(("\tinput:%d\n", input));
    debug_print(("\toutput:%d\n", output));
    debug_print(("\targType:%d\n", type));
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
