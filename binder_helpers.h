#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "server.h"


bool server_ptr_eq(const Server* first, const Server* second);

char* string_to_cstring(string s);

template<class TYPE>
TYPE* vector_to_array(vector<TYPE> x);