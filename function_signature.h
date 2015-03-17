#ifndef FUNCTION_SIG_H
#define FUNCTION_SIG_H

#include <string>
#include <vector>

#include "arg_type.h"

struct FunctionSignature {
    std::string name;
    std::vector<ArgType> argTypes;

    FunctionSignature();
    FunctionSignature(char *name, int *argTypes);
    FunctionSignature(char *name, std::vector<int> argTypes);
    FunctionSignature(char *name, std::vector<ArgType> argTypes);
    // bool operator==(FunctionSignature &other);
    bool operator<(const FunctionSignature &other) const;
    void print() const;
};

#endif