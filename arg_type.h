#ifndef ARG_TYPE_H
#define ARG_TYPE_H

struct ArgType {
    bool input;
    bool output;
    short type;
    short arrayLength;

    ArgType(int typeData);
    int toInt();
    bool operator<(const ArgType &other) const;
    void print();
};

#endif