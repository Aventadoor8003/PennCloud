#include "Request.h"
#include "unordered_map"
#include "iostream"
#include "utils.h"

using namespace std;

Request::Request(string msg,int comm_fd) {
    int initial_line_pos = msg.find("\r");
    string initial_line = msg.substr(0, initial_line_pos);
    vector<string> initial_line_tokens = Split(initial_line.c_str(), ' ');
    int content_len = -1;
    if (initial_line_tokens.size() != 3) {
        cout << "Initial line token numbers != 3\n" << endl;
        valid = false;
        return;
    }
    if (!GetMidStr(msg,"Content-Length: ","\r\n").empty()) {
        content_len = TrimNumber(GetMidStr(msg,"Content-Length: ","\r\n"));
    }
    if (msg.find("Cookie:") != string::npos)  {
        string Cookie = "Cookie:";
        auto pos1 = msg.find(Cookie);
        if (pos1 != string::npos) {
            string subSrc = msg.substr(pos1 + Cookie.size());
            auto pos2 = subSrc.find("\r\n");
            if (pos2 != string::npos) {
                auto cookie = subSrc.substr(0,pos2);
                auto vec = Split(cookie,';');
                for (auto item:vec) {
                    auto splits = Split(item,'=');
                    if (splits.size() != 2) {
                        continue;
                    }
                    auto k = TrimSpace(splits[0]);
                    auto v = TrimSpace(splits[1]);
                    cookies[k] = v;
                }
            }
        }
    }
    this->content_length = content_len;
    this->method_ = initial_line_tokens.at(0);
    auto url_ = initial_line_tokens.at(1);
    int pos = url_.find('?');
    if (pos == string::npos) {
        this->path_ = url_;
    }else {
        this->path_ = url_.substr(0,pos);
        this->params_ = url_.substr(pos+1);
    }
    this->version_ = initial_line_tokens.at(2);
    cout << "\n" << endl;
    cout << "---------parse request---------:" << endl;
    cout << "Method is: " << method_ << endl;
    cout << "Path is: " << path_ << endl;
    cout << "Version is: " << version_ << endl;
    cout << "Query string is: " << params_ << endl;
    cout << "Content length is: " << content_len << endl;

    if(content_len != -1){
        body_ = ReadBody(comm_fd,content_length);
    }
    if (body_.size() < 500) {
        cout << "Content body is: " << body_ << endl;
    }else {
        cout << "Content body is:[truncated] " << body_.substr(0,500) <<"\n,bytes:" << body_.size()<< endl;
    }
    cout << "------------end---------------:" << endl;
    cout << "\r" << endl;
}

string Request::GetPath() {
    return path_;
}

string Request::GetVersion() {
    return version_;
}

string Request::GetBody() {
    return body_;
}

string Request::GetMethod() {
    return method_;
}

const unordered_map<std::string, std::string> &Request::GetCookies() const {
    return cookies;
}

std::string Request::GetParams() {
    return params_;
}
