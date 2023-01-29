#include <iostream>

#include "cxxopts.hpp"
#include "echoserver.h"

using namespace std;

EchoServer::EchoServer() : BaseServer() {}

EchoServer::~EchoServer() { }

int EchoServer::ParseConfig(int argc, char** argv) {
    return 0;
}

//Default address: 0.0.0.0:4711
void EchoServer::Worker(void* param) {
    int comm_fd = *(int*)param;
    cout << "Echo worker starts" << endl;
    RemoveFd(comm_fd);
    close(comm_fd);
}