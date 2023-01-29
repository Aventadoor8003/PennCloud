#pragma once

#include "baseserver.h"
#include "tablet.h"

void ParseArgs(int argc, char *argv[]);

class StorageServer : public BaseServer {
    public:

    StorageServer();
    ~StorageServer();

    std::string server_string_id;
    sockaddr_in server_sock_id;
    std::string server_addr;
    int server_port;
    Tablet tablet;
    void ReinitializeTablet();
    void Update(std::string server_string_id);
    std::string DoPut (std::string r, std::string c, std::string v, std::string message);
    std::string DoGet (std::string r, std::string c, std::string message);
    std::string DoCput (std::string r, std::string c, std::string v1, std::string v2, std::string message);
    std::string DoDelete (std::string r, std::string c, std::string message);
    void ResetIndex();
    std::string ForwardMessage(int dest_server_index, std::string message);
    bool SetupListenAddr() override;
    void Worker(void* param) override;
    int Run() override;
    int ParseConfig(int argc, char** argv) override;
    int ParseConfigWithReplica(int argc, char** argv);
    void ForwardWorker();
    void SendHeartBeats(void* param);
    bool CheckLogExist();
    void CreateLog();
    void CreateCheckPoint();
    void ReadLog();
    void WriteLog(bool write_log, std::string readin_string);
    void WriteCheckPoint();
    void ClearLog();
    void ParseArgs(int argc, char *argv[]);
    std::string get_ip_from_file(char *argv[], StorageServer &table);
    bool ProcessOperations(int comm_fd, std::string command, std::string message, std::string readin_string, int type);
    std::string GetPrimaryAndLogCount(int server_index);
    StorageServer (std::string server_string_id);
};

void signal_handler(int signum);
