#ifndef ARG_TYPE_H
#define ARG_TYPE_H

struct ArgType {
    bool input;
    bool output;
    short type;
    short arrayLength;

    ArgType(int typeData);
    int toInt();
    short memoryLength();
    bool isScalar() const;
    bool isArray() const;
    bool operator<(const ArgType &other) const;
    bool operator==(const ArgType &other) const;
    bool operator!=(const ArgType &other) const;
    void print() const;
};

// struct ArgTypeList : vector<ArgType> {
//     ArgTypeList(int *args);
// }

#endif
