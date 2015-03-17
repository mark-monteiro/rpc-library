#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <string>
#include <vector>
#include "arg_type.h"

std::vector<char> serializeString(char *data);
std::string deserializeString(std::vector<char>::iterator &buffer);
// can use std::string(&buffer[0]) if string is null-terminated

std::vector<char> serializeShort(short data);
short deserializeShort(std::vector<char>::iterator &buffer);

std::vector<char> serializeInt(int data);
int deserializeInt(std::vector<char>::iterator &buffer);

std::vector<char> serializeLong(long data);
long deserializeLong(std::vector<char>::iterator &buffer);

std::vector<char> serializeDouble(double data);
double deserializeDouble(std::vector<char>::iterator &buffer);

std::vector<char> serializeFloat(float data);
float deserializeFloat(std::vector<char>::iterator &buffer);

std::vector<char> serializeArgTypes(int *data);
std::vector<ArgType> deserializeArgTypes(std::vector<char>::iterator &buffer);

std::vector<char> serializeArgs(int **data);
int** deserializeArgs(std::vector<char>::iterator &buffer); 

#endif