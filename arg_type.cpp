#include "debug.h"
#include "rpc.h"

#include "arg_type.h"

ArgType::ArgType(int typeData) {
    input =  ((typeData >>  ARG_INPUT) & 1) == 1;   //shift right 31 bits then get the last bit
    output = ((typeData >> ARG_OUTPUT) & 1) == 1;   //shift right 30 bits then get the last bit
    type = (typeData >> 16) & 0xFF;                 //shift right 16 bits then get rightmost byte
    arrayLength = typeData & 0xFFFF;                //get rightmost two bytes
}

int ArgType::toInt() {
    return (input << ARG_INPUT) | (output << ARG_OUTPUT) | type << 16 | arrayLength;
}

// Note: ignores array length
bool ArgType::operator<(const ArgType &other) const {
    return
        input < other.input ||
        output < other.output ||
        type < other.type;
}

void ArgType::print() {
    debug_print(("\tinput:%d\n", input));
    debug_print(("\toutput:%d\n", output));
    debug_print(("\targType:%d\n", type));
    debug_print(("\tarrayLength:%d\n", arrayLength));
}