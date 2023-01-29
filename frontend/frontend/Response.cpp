#include "Response.h"
#include "iostream"
using namespace std;
Response::Response(string version, int code, string contentType, string htmlBody) {
    version_ = version;
    code_ = code;
    contentType_ = contentType;
    htmlBody_ = htmlBody;
}

Response::Response(string version, string htmlBody) {
    version_ = version;
    code_ = 200;
    contentType_ = "text/html";
    htmlBody_ = htmlBody;
}

string Response::ToString() {
    string ret;
    ret += version_ + " ";
    ret += std::to_string(code_) + " " + "OK" + "\r\n";
    for(int i = 0; i< cookiesKey_.size();i++) {
        auto k = cookiesKey_[i];
        auto v = cookiesVal_[i];
        auto newCookie = "Set-Cookie: " + k + "=" + v + "\r\n";
        ret += newCookie;
    }
    ret += "Content-type: " + contentType_ + "\r\n";
    string data = htmlBody_;
    if (!filename_.empty()) {
        data = fileRaw_;
        ret += "Content-Disposition: attachment; filename=" +filename_ + "\r\n";
    }
    ret += "Content-length: " + std::to_string(data.size()) + "\r\n";
    ret += "\r\n";
    ret += data;
    return ret;
}

void Response::SetCookie(string k, string v) {
    cookiesKey_.push_back(k);
    cookiesVal_.push_back(v);
}

void Response::SetAttachment(std::string filename, std::string fileRaw) {
    filename_ = filename;
    fileRaw_ = fileRaw;
}
