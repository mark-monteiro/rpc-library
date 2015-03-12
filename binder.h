#ifndef BINDER_H
#define BINDER_H

const int SERVER_NAME_LEN = 400;

struct server{
	int sock;
	int port;
	char* id;
};

#endif