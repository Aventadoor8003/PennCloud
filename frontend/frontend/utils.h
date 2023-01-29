#ifndef FRONTEND_UTILS_H
#define FRONTEND_UTILS_H
#include <cstdio>
#include <stdlib.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <algorithm>
#include <vector>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <resolv.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

void error_handler(char const *msg);
bool DoRead(int conn_fd, char buffer[]);
bool DoReadWithCRLF(int conn_fd, char buffer[]);
std::string DoReadWithCRLF(int conn_fd);
std::string ReadBody(int conn_fd,int length);
bool DoWrite(int conn_fd, const char* buffer, int len);
long long int DoWrite(int conn_fd, std::string buffer, int len);
std::vector<std::string> Split(const std::string &s, char delim);
std::string GetMidStr(std::string src,std::string left,std::string right);
int TrimNumber(std::string num);
std::string TrimSpace(std::string str);
std::string GetQuery(std::string src, std::string key);
std::string StringToBit(std::string src);
std::string BitToString(std::string bitString);
std::string Bytes(std::string str);
std::string Stringify(std::string str);
std::string DecodeURL(std::string content);
std::string escapeHTML(const std::string &from);
std::string ReplaceAll(std::string& str, const std::string& from, const std::string& to);
std::vector<std::vector<std::string>> ParseDirContents(std::string dir_contents);
std::string ReadHTML(std::string file_path);
std::string CreateFileHash(std::string filename, std::string parent_id);
std::string htmlToString(const char* filename);
std::string VectorToString(std::vector<std::string> list);
std::string dns_query(std::string server_name);
#endif //FRONTEND_UTILS_H
