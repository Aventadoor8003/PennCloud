#include "Message.h"

using namespace std;

Message::Message(std::string prex, std::string row, std::string col, std::string value1, std::string value2){
    this->prefix = prex;
    this->row = row;
    this->col = col;
    this->value1 = value1;
    this->value2 = value2;
}

Message::~Message(){}

string Message::ParseMsg(){
    string res;
    res = this->prefix + " " + this->row + ";" + this->col + ";" + this->value1 + ";" + this->value2 + "\r\n";
    return res;
}