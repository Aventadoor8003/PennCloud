#ifndef FRONTEND_FILEBODY_H
#define FRONTEND_FILEBODY_H
#include "string"
#include "utils.h"
using namespace std;
class FileBody {
private:
    string startLine_;
public:
    string fileName_;
    string fileRaw_;
    FileBody(string body);
    
};


#endif //FRONTEND_FILEBODY_H
