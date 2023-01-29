#include <iostream>
#include <csignal>
#include <memory>

#include "../include/masternode.h"
#include "../include/logger16.hpp"

using namespace std;

MasterNode* master_ptr;

void SigintHandler(int dummy) {
    cout << "Masternode stopped by SIGINT" << endl;
    master_ptr->Stop();   
    //exit(1);
}

void SigpipeHandler(int dummy) {
    cout << "Masternode got SIGPIPE. Closing bad file descriptor" << endl;
    master_ptr->CloseBadFd();
}

int main(int argc, char** argv) {

    MasterNode master_node;
    master_ptr = &master_node;
    int status = master_node.ParseConfig(argc, argv);
    if(status) {
        cout << "Failed to start masternode" << endl;
        return 1;
    }
    signal(SIGINT, SigintHandler);
    signal(SIGPIPE, SigpipeHandler);
    master_node.Run();

    return 0;
}