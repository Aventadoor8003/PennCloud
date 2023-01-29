#ifndef FRONTEND_HTML_H
#define FRONTEND_HTML_H
#include "string"
#include "../utils.h"
using std::string;

class Html {
private:
    string filePath_;
    string rawData_;
public:
    Html(string filePath);
    string GetRawData();
    void Replace(string signal,string source);
};


#endif //FRONTEND_HTML_H
