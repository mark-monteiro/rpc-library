#!/bin/bash

# Pass binder port as first argument
export SERVER_PORT=$1
export SERVER_ADDRESS=${2-localhost}

./client
#valgrind --leak-check=yes ./server
