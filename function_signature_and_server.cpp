#include <iostream>
#include <vector>

#include "function_signature_and_server.h"
#include "function_signature.h"
#include "arg_type.h"
#include "server.h"


bool FunctionSignatureAndServer::operator==(const FunctionSignatureAndServer &other) const {
    return (other.functionSignature.name == functionSignature.name) && (other.functionSignature.argTypes == functionSignature.argTypes);
}


FunctionSignatureAndServer::FunctionSignatureAndServer(char *name, vector<ArgType> argTypes, Server* s) {
    functionSignature = FunctionSignature(name, argTypes);
    server = s;
}