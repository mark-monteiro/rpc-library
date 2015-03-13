#ifndef SERIALIZE_H
#define SERIALIZE_H

#include <string>
#include <vector>

std::vector<char> serializeString(char *data);
std::string deserializeString(std::vector<char>::iterator &buffer);

std::vector<char> serializeChar(char data);
char deserializeChar(std::vector<char>::iterator &buffer);

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
std::vector<int> deserializeArgTypes(std::vector<char>::iterator &buffer);

std::vector<char> serializeArgs(int *argTypes, bool inputs, bool outputs, void **data);
void deserializeArgs(int *argTypes, bool inputs, bool outputs, void **args, std::vector<char>::iterator &buffer); 

#endif
