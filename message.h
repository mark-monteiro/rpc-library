#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>
#include <string>
#include <vector>
#include <list>

enum Msg_Type {
    REGISTER,
    REGISTER_RESPONSE,
    LOC_REQUEST,
    LOC_SUCCESS,
    LOC_FAILURE,
    EXECUTE,
    EXECUTE_SUCCESS,
    EXECUTE_FAILURE,
    TERMINATE
};

struct Message {
    Msg_Type type;
    int len;
    std::vector<char> data;

    uint32_t length();
    void addData(std::vector<char>);
    void addData(char);
    bool send(int sock);
    static bool recv(int sock, Message *message);
    std::string typeToString();
    std::string dataToString(int startIndex = 0);
    void print();
};

#endif
