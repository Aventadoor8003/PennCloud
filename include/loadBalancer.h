#pragma once

#include "baseserver.h"
#include <map>
#include <string.h>
#include <set>

//using namespace std;
/// @brief LoadBalancer
/// Parameters when invoking: -v (optinal) verbose mode
///                           -a address and port number
///                           -m (optional) address of admin console

    
class LoadBalancer : public BaseServer {
public:

    LoadBalancer();

    ~LoadBalancer();

    virtual int ParseConfig(int argc, char** argv) override;

    virtual int Run() override;

    int updateAvailableServer();
    

private:

    

    struct ServerInfo{
            std::string ip_addr;
            int status;
        };
    
    virtual void Worker(void* param) override;

    virtual bool SetupListenAddr() override;

    void checkHeartbeat(void* arg);

    int loadServerConfig(const char* filename);

    int numserver_;

    int currHandler_=0;
        
    pthread_mutex_t mutex_;

    std::map<int,ServerInfo> servers_;
    
    //IN DEV
    std::string console_addr_ = "127.0.0.1:23333";

    int console_sock_;

    int console_alive_flag_ = 0;

    int TryConnectAdminWorker(void* param);

    int DisconnectFromAdmin();
};

/**TODO:
 * +Fix LBRG message 
 * +Add reporter in worker
 * +Add connect worker and dieconnect method (public)
 * 
 * TEST：
 * -Test redirection when several frontend server running
 */