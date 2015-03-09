To compile the rpc library and the binder:
`make debug` (or just `make` to exclude debugging flags)

To compile the test code (test client and test server):
`make test`

Run the binder:
`./binder`

The binder will print out a port number, then you can run the server test code by running ./tester <port_num>
All the 'tester' file does is just set the necessary environment variables before running the server