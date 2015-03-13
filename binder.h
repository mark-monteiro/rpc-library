#ifndef BINDER_H
#define BINDER_H

#include <string.h>
#include <iostream>
#include "function_signature.h"

using namespace std;


const int SERVER_NAME_LEN = 400;

struct Server{
	int sock;
	int port;
	string id;
};

struct FunctionSignatureAndServer{
	FunctionSignature functionSignature;
	Server* server;
	FunctionSignatureAndServer(char*, vector<int>, Server*);
};

#endif