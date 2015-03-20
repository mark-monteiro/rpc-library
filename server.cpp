#include "server.h"
#include "debug.h"

bool Server::operator==(const Server &other) const {
    return sock == other.sock;
}

void Server::print() const {
    debug_print(("%d", sock));
}