#!/bin/bash

# Pass binder port as first argument
export SERVER_ADDRESS=localhost
export SERVER_PORT=$1

./client
#valgrind --leak-check=yes ./server
