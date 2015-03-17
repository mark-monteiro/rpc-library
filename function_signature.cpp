#include <string>
#include <vector>

#include "arg_type.h"
// #include "debug.h"

#include "function_signature.h"

# define debug_print(x) do {} while (0)

using namespace std;

FunctionSignature::FunctionSignature(char *name, int *argTypes) {
    this->name = string(name);

    int numArgs = 0;
    while(argTypes[numArgs] != 0) numArgs++;
    this->argTypes = vector<ArgType>(argTypes, argTypes + numArgs);
}

FunctionSignature::FunctionSignature(char *name, vector<int> argTypes) {
    this->name = string(name);
    this->argTypes = vector<ArgType>(argTypes.begin(), argTypes.end());
}

FunctionSignature::FunctionSignature(char *name, vector<ArgType> argTypes) {
    this->name = string(name);
    this->argTypes = vector<ArgType>(argTypes.begin(), argTypes.end());
}

FunctionSignature::FunctionSignature(){

}

// bool FunctionSignature::operator==(FunctionSignature &other) {
//     return name == other.name && argTypes == other.argTypes;
// }

bool FunctionSignature::operator<(const FunctionSignature &other) const {
    if(name != other.name) return name < other.name;
    else return argTypes < other.argTypes;
}

void FunctionSignature::print() const {
    debug_print(("%s", name.c_str()));
    for(int i = 0 ; i < argTypes.size() ; i++) {
        argTypes[i].print();
    }
    debug_print(("\n"));
}