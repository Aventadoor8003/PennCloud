#include "FileBody.h"
FileBody::FileBody(string body) {
    bool start;
    string firstLine;
    for (int i = 0; i < body.size()-1; ++i) {
        auto c = body.at(i);
        if (c == ' ') {
            continue;
        }
        if (c == '-') {
            start = true;
        }
        if (start) {
            if (c == '\r' && body.at(i + 1) == '\n') {
                break;
            }
            firstLine += c;
        }
    }
    startLine_ = firstLine;

    auto fileNamePos = body.find("filename");
    auto sub = body.substr(fileNamePos);
    auto nextPos = sub.find("\r\n");
    auto fileNameRaw = sub.substr(0,nextPos);
    start = false;
    for (auto c: fileNameRaw) {
        if (c == '"' && !start) {
            start = true;
            continue;
        }
        if (c == '"' && start) {
            break;
        }
        if (!start) {
            continue;
        }
        fileName_ += c;
    }

    auto fileRawPos = body.find("\r\n\r\n");
    sub = body.substr(fileRawPos + string("\r\n\r\n").size());
    auto endPos = sub.find(firstLine);
    fileRaw_ = sub.substr(0,endPos-2);
}
