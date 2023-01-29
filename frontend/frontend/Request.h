#ifndef FRONTEND_REQUEST_H
#define FRONTEND_REQUEST_H
#include <string>
#include <unordered_map>


class Request {
private:
    std::string path_;
    std::string method_; // GET POST
    std::string body_;
    std::string params_;
    std::string version_;
    int content_length;
    bool valid;
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> cookies;

public:
    Request(std::string msg,int comm_fd);
    std::string GetPath();
    std::string GetVersion();
    std::string GetBody();
    std::string GetMethod();
    std::string GetParams();
    const std::unordered_map<std::string, std::string> &GetCookies() const;
};


#endif //FRONTEND_REQUEST_H
