#ifndef FRONTEND_GLOBAL_VARS_H
#define FRONTEND_GLOBAL_VARS_H
#include "csignal"
#include "map"
#include "string"
using std::string;
bool sd_flag = false;
pthread_t main_tid;
sigset_t mask;
std::map<int, pthread_t> thread_pool; //<fd, tid>
int BUFF_SIZE = 10000;
string MASTER_IP = "127.0.0.1";
unsigned short MASTER_PORT = 2334;
string BalanceIP = "127.0.0.1";
unsigned short BalancePORT = 5000;

#endif //FRONTEND_GLOBAL_VARS_H