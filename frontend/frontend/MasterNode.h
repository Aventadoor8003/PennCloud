#ifndef FRONTEND_MASTERNODE_H
#define FRONTEND_MASTERNODE_H
#include "string"
#include "iostream"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "string.h"
#include <unistd.h>
#include "utils.h"
using namespace std;
extern string MASTER_IP;
extern unsigned short MASTER_PORT;

class MasterNode {
private:
    int masterFd_;
public:
    MasterNode();
    string GetAddress(string userName);
};


#endif //FRONTEND_MASTERNODE_H
