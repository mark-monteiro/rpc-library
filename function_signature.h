#ifndef FUNCTION_SIG_H
#define FUNCTION_SIG_H

#include <string>
#include <vector>

struct FunctionSignature {
    std::string name;
    std::vector<int> argTypes;

    FunctionSignature(char *name, int *argTypes);
    // bool operator==(FunctionSignature &other);
    bool operator<(const FunctionSignature &other) const;
};

#endif