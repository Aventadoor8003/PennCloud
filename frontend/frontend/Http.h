#ifndef FRONTEND_HTTP_H
#define FRONTEND_HTTP_H
#include <string>
#include "Handler.h"
#include <unordered_map>

class Http {
private:
    int listen_fd;

public:
    int server_port;
    Http(int);
    ~Http();
    void Register(std::string path,Handler*,std::string htmlFile);
    int Run() const;
    void ParseConfig(int argc, char** argv);
};


#endif //FRONTEND_HTTP_H
