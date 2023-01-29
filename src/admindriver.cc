#include "../include/adminserver.h"
#include <iostream>
#include <csignal>

using namespace std;

AdminServer* admin_ptr;

void SigintHandler(int dummy) {
    cout << "Admin server stopped by SIGINT" << endl;
    admin_ptr->Stop();   
    //exit(1);
}


int main(int argc, char** argv) {
    AdminServer server;
    int status = server.ParseConfig(argc, argv);
    if(status) {
        cout << "Failed to start masternode" << endl;
        return 1;
    }

    admin_ptr = &server;
    signal(SIGINT, SigintHandler);
    server.Run();
    return 0;
}