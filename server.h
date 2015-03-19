#ifndef SERVER_H
#define SERVER_H

#include <iostream>

#include <string.h>

using namespace std;

struct Server{
	int sock;
	int port;
	string id;

	bool operator==(const Server &other) const;
};

#endif