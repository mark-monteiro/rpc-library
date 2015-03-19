#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#include "server.h"
#include "binder_helpers.h"


bool server_ptr_eq(const Server* first, const Server* second) {
    return *first == *second;
}

char* string_to_cstring(string s) {
    return &s[0];
}

template<class TYPE>
TYPE* vector_to_array(vector<TYPE> x) {
    return &x[0];
}