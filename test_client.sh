#!/bin/bash

# Pass binder port as first argument
export BINDER_PORT=$1
export BINDER_ADDRESS=${2-localhost}

#TODO: remove once LOC_REQUEST is implemented
export SERVER_PORT=$1
export SERVER_ADDRESS=${2-localhost}

./client
#valgrind --leak-check=yes ./server
