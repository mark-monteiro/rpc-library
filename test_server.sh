#!/bin/bash

# Pass binder port as first argument
export BINDER_PORT=$1
export BINDER_ADDRESS=${2-localhost}

# ./server
valgrind --leak-check=yes ./server
