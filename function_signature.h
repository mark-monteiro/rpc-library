#ifndef FUNCTION_SIG_H
#define FUNCTION_SIG_H

#include <string>
#include <vector>

#include "arg_type.h"

struct FunctionSignature {
    std::string name;
    std::vector<ArgType> argTypes;

// <<<<<<< HEAD
    FunctionSignature();
//     FunctionSignature(char *name, int *argTypes);
//     FunctionSignature(char *name, std::vector<int> argTypes);
    FunctionSignature(char *name, std::vector<ArgType> argTypes);
// =======
    FunctionSignature(std::string name, int *argTypes);
    FunctionSignature(std::string name, std::vector<int> argTypes);
// >>>>>>> ace5dd934ade45182c1936b97e35515b5e5c5ea8
    bool operator==(FunctionSignature &other);
    bool operator<(const FunctionSignature &other) const;
    void print() const;
};

#endif