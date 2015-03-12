#ifndef FUNCTION_SIG_H
#define FUNCTION_SIG_H

#include <string>
#include <vector>

#include "arg_type.h"

struct FunctionSignature {
    std::string name;
    std::vector<ArgType> argTypes;

    FunctionSignature(std::string name, int *argTypes);
    FunctionSignature(std::string name, std::vector<int> argTypes);
    // bool operator==(FunctionSignature &other);
    bool operator<(const FunctionSignature &other) const;
    void print() const;
};

#endif