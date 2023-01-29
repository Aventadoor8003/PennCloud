#ifndef FRONTEND_BALANCENODE_H
#define FRONTEND_BALANCENODE_H
#include "string"
#include "iostream"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "string.h"
#include <unistd.h>
#include "utils.h"
using namespace std;
extern string BalanceIP;
extern unsigned short BalancePORT;
class BalanceNode {
private:
    int Fd_;
public:
    BalanceNode();
    int Send();
};


#endif //FRONTEND_BALANCENODE_H
