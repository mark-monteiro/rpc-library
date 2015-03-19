#!/bin/bash

# Pass binder port as first argument

export BINDER_PORT=$1
export BINDER_ADDRESS=${2-localhost}

# ./server
valgrind --leak-check=yes ./server
# >>>>>>> ace5dd934ade45182c1936b97e35515b5e5c5ea8:test_server.sh
