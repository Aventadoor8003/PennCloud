#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <string>
#include <signal.h>

#include "../include/storageserver.h"

using namespace std;

int main(int argc, char *argv[]){
    StorageServer server;
    signal(SIGINT, signal_handler);
    /* Parse command line input opt*/
    server.ParseArgs(argc, argv);
    /* read configuration file and initialize server*/
    server.ParseConfigWithReplica(argc, argv);
    
    server.Run();

    return 0;
}


