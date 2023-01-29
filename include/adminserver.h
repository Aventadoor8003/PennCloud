#pragma once

#include <unordered_map>

#include "baseserver.h"

#define BACKEND_REPORTER_CMD "MNTR"
#define FRONTEND_REPORTER_CMD "FRTR"
#define MASERNODE_REGISTER_CMD "MNRG"
#define LOADBALANCER_REGISTER_CMD "LBRGS"

namespace adminconsole {

enum CommandType {
    kHttpCommand,
    kBackendCommand,
    kUnownCommand
};

enum ClientType {
    kUnknownSource,
    kAdminPage,
    kMasterNode,
    KLoadBalancer,
    kAnySrouce
};
}

using CommandSourceIndexer = std::unordered_map<std::string, int>;

/// @brief An admin console server, used to redirect requests from frontend server
/// and moniter KV stores
/// Parameters when invoking: -v (optional) verbose mode
///                           -a address and port number
///                           -c name of backend config file
///                           -f name of frontend config file


class AdminServer : public BaseServer {
public:

    AdminServer();

    ~AdminServer();

    virtual int ParseConfig(int argc, char** argv) override;

    virtual int Run() override;

    //virtual int Stop() override;

private:

/**+++++++++++++++++++++++++++++++++ Server status ++++++++++++++++++++++++++++++++++*/
    int run_flag_ = 1;

    std::string master_addr_ = "127.0.0.1:2334";

    CommandSourceIndexer command_indexer_;

    std::mutex node_status_mutex_;

    int master_alive_flag_;

    std::unordered_map<std::string, int> frontend_status_map_;

    std::unordered_map<std::string, int> backend_status_map_; 

/**+++++++++++++++++++++++++++++++++ Helper methods ++++++++++++++++++++++++++++++++++*/
    virtual void Worker(void* param);

    virtual bool SetupListenAddr() override;

    void SetUpCommandIndexer();

    bool IsPrivateCommand(std::string command);

    int SetUpFrontendStatusMap(std::string config_file);   

    int SetUpBackendStatusMap(std::string config_file);   

    void CheckFrontendStatusWorker(void* param);

    
    /// @brief Handle private protocol
    int HandlePrivateProtocol(int comm_fd, std::string full_message);   

    //+++++++++++++++++++ Command Handlers ++++++++++++++++
    /// @brief 
    /// @return 
    int HandleMnrt(int comm_fd, std::string full_msg);


    //--------IN DEV
    
    /// @brief conne
    /// @param target_server 
    /// @return Success: the file descriptor to the socket, fail: -1
    int ConnectToServer(std::string target_server);

    int HandlePaus(int comm_fd, std::string full_msg);

    int HandleCnte(int comm_fd, std::string full_msg);

    int HandleView(int comm_fd, std::string full_msg);

    //------FUTURE

    int HandleHttpRequest(int comm_fd, std::string request);

    bool IsHttpRequest(std::string request);
};

/**
 * TODO: 
 * - Add a connect to server method
 * - PAUS: pause a backend node Example: PAUS 127.0.0.1:8001
 * - CNTE: continue a backend node Example: CNTE 127.0.0.1:8001
 * - VIEW: view raw content in backend node Example: VIEW 127.0.0.1:8001
 *  --Send CONT to that server
 * 
 * - Find a way to seperate http command and private commands
 * - Replace -c option with -b(backend config file) and -f(frontend config file)
 * - HTTP commands:
 *  -- GET: 1. get a web page showing all status 2. get raw content
 *  -- POST: 1.  Post pause and continue commands to backend
 * TEST:
 * - Test PAUS, CNTE and VIEW with all components on
 * 
 * TARGET: 
 *         Monitor backend server through master node
 *         Send Disable and start commands to backend servers
 *         Monitor frontend server through load balancer
 * BEHAVIOR:
 *  Requests from browser: kill backend node, start backend node, Check content in backend node,
 *  Requests send to backend node(): Pause, continue
 *  Message from master node(MNRT): Status of backend node. 
 *      If no MNRT, mark it as dead
 *  Message from load balancer(LBRT): Status of frontend node
 * 
 * DONE:
 * 
 * + Initialize status map in ParseConfig 1218-1819
 *  ++ Initialize backend config file
 *  ++ Initialize frontend config file
 * 
 * + Add MNRT support
 *  ++ Update status map
 */