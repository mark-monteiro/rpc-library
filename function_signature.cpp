#include <string>
#include <vector>

#include "arg_type.h"
#include "debug.h"

#include "function_signature.h"

using namespace std;

FunctionSignature::FunctionSignature(){
}

FunctionSignature::FunctionSignature(string name, vector<ArgType> argTypes) {
    this->name = name;
    this->argTypes = vector<ArgType>(argTypes.begin(), argTypes.end());
}

FunctionSignature::FunctionSignature(string name, int *argTypes) {
    this->name = name;

    int numArgs = 0;
    while(argTypes[numArgs] != 0) numArgs++;
    this->argTypes = vector<ArgType>(argTypes, argTypes + numArgs + 1);
}

FunctionSignature::FunctionSignature(string name, vector<int> argTypes) {
    this->name = name;
    this->argTypes = vector<ArgType>(argTypes.begin(), argTypes.end());
}

bool FunctionSignature::operator==(FunctionSignature &other) const {
    return (!(*this < other)) && (!(other < *this));
}

bool FunctionSignature::operator<(const FunctionSignature &other) const {
    if(name != other.name) return name < other.name;
    else return argTypes < other.argTypes;
}

void FunctionSignature::print() const {
    debug_print(("%s", name.c_str()));
    for(unsigned int i = 0 ; i < argTypes.size() ; i++) {
        argTypes[i].print();
    }
    debug_print(("\n"));
}