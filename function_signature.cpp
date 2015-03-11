#include <string>
#include <vector>

#include "function_signature.h"

using namespace std;

FunctionSignature::FunctionSignature(char *name, int *argTypes) {
    this->name = string(name);
    
    int numArgs = 0;
    while(argTypes[numArgs] != 0) numArgs++;
    this->argTypes = vector<int>(argTypes, argTypes + numArgs);
}

// bool FunctionSignature::operator==(FunctionSignature &other) {
//     return this->name == other.name &&
//         this->argTypes.size() == other.argTypes.size() &&
//         //TODO: use a custom comparer here to ignore array lengths on the arg types
//         equal(this->argTypes.begin(), this->argTypes.end(), other.argTypes.begin());
// }

bool FunctionSignature::operator<(const FunctionSignature &other) const {
    if(this->name < other.name) return true;
    if(this->argTypes.size() < other.argTypes.size()) return true;

    for(int i = 0 ; i < argTypes.size() ; i++) {
        //TODO: use a custom comparer here to ignore array lengths on the arg type
        if(this->argTypes[i] < other.argTypes[i]) return true;
    }

    return false;
}