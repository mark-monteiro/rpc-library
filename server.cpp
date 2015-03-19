#include "server.h"

bool Server::operator==(const Server &other) const {
    return sock == other.sock;
}