#include <iostream>
#include <string.h>
#include "../include/loadBalancer.h"

using namespace std;

int main(int argc, char** argv) {
    LoadBalancer loadbalancer;
    int status = loadbalancer.ParseConfig(argc, argv);
    if(status) {
        cout << "Server configuration failed" << endl;
        return 1;
    }
    //start listening thread to check frontend server 
    loadbalancer.updateAvailableServer();
    loadbalancer.Run();

    return 0;
}