#ifndef FRONTEND_MESSAGE_H
#define FRONTEND_MESSAGE_H
#include <string>


class Message{
private:
    std::string prefix;  
    std::string row;
    std::string col;
    std::string value1;
    std::string value2;

public:
    Message(std::string prefix, std::string row, std::string col, std::string value1, std::string value2);
    ~Message();
    std::string ParseMsg();
};

#endif //FRONTEND_MESSAGE_H
