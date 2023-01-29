#pragma once

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <ctime>

#include "baseserver.h"

using CommandPool = std::unordered_map<std::string, int>;
using NodeIdIndexer = std::unordered_map<std::string, int>;
using LoadIndexer = std::unordered_map<std::string, int>;
using LoadMap = std::unordered_map<int, std::map<int, std::unordered_set<std::string>>>;

/// @brief A masternode server, used to redirect requests from frontend server
/// and moniter KV stores
/// Parameters when invoking: -v (optinal) verbose mode
///                           -a address and port number
///                           -c name of config file

/// TODO:              
///                           -m (optional) address of admin console

namespace masternode {
enum ClientTYpe {
    kUnknownSource,
    kFrontendServer,
    kKvStore,
    kAdmainConsole,
    kAnySource
};
}

class MasterNode : public BaseServer {
public:

    MasterNode();

    ~MasterNode();

    virtual int ParseConfig(int argc, char** argv) override;

    virtual int Run() override;

    int CloseBadFd() {
        close(bad_fd_);
        return 0;
    }

private:

    /*++++++++++++++++++++++++ Attribution data ++++++++++++++++++++*/
    
    std::string config_filename_;

    std::mutex nodes_monitor_mutex_;

    /// @brief 0: not connected to console, 1: connected to console
    int console_alive_flag_ = 0;

    std::string console_addr_ = "127.0.0.1:23333";

    int console_sock_;


    /// @brief K: group id, V: the corresponding mutex
    std::unordered_map<int, std::mutex*> group_mutex_map_;
     
    /// @brief Identify a source of a command. 
    /// Example: STST: KV store, ASTA: Frontend server
    CommandPool client_source_pool_;

    /// @brief K: group id, V: set of all addresses in that group
    std::unordered_map<int, std::unordered_set<std::string>> group_map_;

    /// @brief Get a group id using an address string
    NodeIdIndexer group_indexer_;

    /// @brief Get a connection number by an address string
    LoadIndexer load_indexer_;

    /// @brief Last report timestamp of one node
    std::unordered_map<std::string, std::time_t> node_ts_map_;

    /// @brief K: node id, V: health level: 1: connected, 0: dead
    /// Shouldn't be written concurrently
    std::unordered_map<std::string, int> node_status_map_;

    /// @brief K: Group number, V: a map of <connection_number, set_of_backend_nodes> 
    /// strings with that number of connections
    LoadMap load_map_;

    /*+++++++++++++++++ Command Handlers ++++++++++++++++++++*/
    /// @return success: 0. Otherwise status code

    /// @brief Handle asta and redirect
    /// @param request: Full request
    int HandleAsta(int comm_fd, std::string request);

    /// @brief Handle STST, Record heartbeat
    /// This method will also update node status, load and assign primary
    /// @param param_str string only contains STST parameters
    int HandleStst(int comm_fd, std::string param_str);
    
    /*++++++++++++++++++++++++++++ Helper methods ++++++++++++++++++++++++++*/
    /// @brief Thread function
    /// @param param communication file descriptor
    virtual void Worker(void* param) override;

    /// @brief Monitor node status. No socket needed
    /// @param param dummy param. Won't be used
    void NodeMonitorWorker(void* param);

    //--------DEPRECATED
    /// @brief Report node status to admin console
    /// @param param dummy param. Won't be used
    //void ReportWorker(void* param);

    /// @brief Set up listen address
    /// @return If success, return 0. Otherwise status code
    virtual bool SetupListenAddr() override;

    /// @brief Create an indexer from command to client type
    void SetUpCommandIndexer();

    /// @brief  Set up group map
    /// @return success: 0. Otherwise status code 
    int SetGroup();

    /// @brief Get a backend address with least
    /// @return The address of the required backend node
    std::string GetBackendAddrWithLeastLoad(int group_id);


    /// @brief Try to connect to console
    /// @param param nothing
    /// @return success: 0. Otherwise status code
    int ConnectToAdminWorker(void* param);

    /// @brief Disconnect from console
    /// @param param nothing
    /// @return success: 0. Otherwise status code
    int DisconnectFromAdminConsole();

    //-------------------------------IN_DEV
    ///@brief K: group id, V: address of primary node
    std::unordered_map<int, std::string> primary_map_;
    
    /// @brief Assign a new primary storage node
    /// @return success: 0. Otherwise status code
    int AssignNewPrimary(int comm_fd, std::string new_primary_addr, int group_id);

    /// @brief Assign a new primary storage node
    /// @return success: 0. Otherwise status code
    int AssignNewPrimary(std::string new_primary_addr, int group_id);

    /// @brief Remove a dead node from load map
    /// @return success: 0. Otherwise status code
    int RemoveNodeFromNodeMap(std::string node_addr);

    //------------------------------IN_DEV
    int bad_fd_;

    std::mutex read_mutex_;

    
};

/**
 * --: undone, ++: done
 * 
 * TODO:
 * -Fix crash in big file transmission
 
 * 
 * TEST: (With all components on)
 * -- Test if load map can direct to a correct address
 * -- Test if ASTA can always redirect to a correct node
 * -- If connection to admin console is correct
 * ++ If masternode can find a new primary
 * ++ If masternode don't find new primary when no alive node in a group
 * 
 * TARGETS:
 *  + Add admain console support
 * 
 * DONE:
 * + Add command MNRG for trying to connect to admin console
 *  ++ Add a register worker to try and register
 *  ++ Quit at the end of run
 * + Fix segmentation fault when the target group didn't start
 *
 * + STST Should have two modes: admin console off, and admin console on
 * + Add report methods in stst to report node status to admin console
 *  ++ Bind and connect to admin console
 *  ++ Report nodes status in STST method
 * 
 * + Add ASPR(Assign primary node)
 *  ++ Initialize primary_map_ in config file reading
 *  ++ Update primary_map_ in STST
 * 
 * +Fix case 1: start 8000 and 8001, then kill 8000(expect 8001 primary) (1219-1710)
 *              Then stop 8001 (expecting no primary for group 0)
 * 
 * 
 * TESTED:
 * 
 */