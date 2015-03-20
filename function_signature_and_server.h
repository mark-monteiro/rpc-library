#ifndef FUNCTION_SIGNATURE_AND_SERVER_H
#define FUNCTION_SIGNATURE_AND_SERVER_H

#include <vector>

#include "function_signature_and_server.h"
#include "function_signature.h"
#include "arg_type.h"
#include "server.h"


struct FunctionSignatureAndServer{
	FunctionSignature functionSignature;
	Server server;

	FunctionSignatureAndServer(string, vector<ArgType>, Server);
	bool operator==(const FunctionSignatureAndServer &other) const;
    void print() const;
};

#endif