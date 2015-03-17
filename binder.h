#ifndef BINDER_H
#define BINDER_H

#include <string.h>
#include <iostream>
#include "function_signature.h"
#include "arg_type.h"

using namespace std;


const int SERVER_NAME_LEN = 400;

struct Server{
	int sock;
	int port;
	string id;

	bool operator==(const Server &other) const;
};

struct FunctionSignatureAndServer{
	FunctionSignature functionSignature;
	Server* server;

	FunctionSignatureAndServer(char*, vector<ArgType>, Server*);
	bool operator==(const FunctionSignatureAndServer &other) const;
};

#endif