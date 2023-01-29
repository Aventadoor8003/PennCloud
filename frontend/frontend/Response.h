#ifndef FRONTEND_RESPONSE_H
#define FRONTEND_RESPONSE_H

#include <string>
#include "vector"

class Response {
private:
    std::string version_;
    int code_;
    std::string contentType_;
    std::string htmlBody_;
    std::vector<std::string> cookiesKey_;
    std::vector<std::string> cookiesVal_;
    std::string filename_;
    std::string fileRaw_;
public:
    Response(std::string version,int code,std::string contentType,std::string htmlBody);
    Response(std::string version,std::string htmlBody);
    std::string ToString();
    void SetCookie(std::string k, std::string v);
    void SetAttachment(std::string filename,std::string fileRaw);
};


#endif //FRONTEND_RESPONSE_H
