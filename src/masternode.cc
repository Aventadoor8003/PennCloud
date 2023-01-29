///@author Haochen Gao
#include <cstring>
#include <cerrno>

#include <iostream>
#include <fstream>
#include <experimental/filesystem>

//TODO: Delete duplicated include
#include "../include/masternode.h"
#include "../include/cxxopts.hpp"
#include "../include/StringOperations.hh"
#include "../include/logger16.hpp"


#define COMMAND_LEN 4
#define FRONTEND_REDIRECT_CMD "ASTA"
#define STATUS_STATE_CMD "STST"
#define QUIT_CMD "QUIT"

#define ASSIGN_NEW_PRIMARY_REQUEST "ASPR"
#define REDIRECT_REQUEST "RDCT"
#define NODE_STATUS_REPORTER_REQUEST "MNRT"

using namespace std;
namespace fs = experimental::filesystem;
using namespace log16;
using namespace masternode;

MasterNode::MasterNode() : BaseServer() { 
    //Initialize Source pool
    SetUpCommandIndexer();
    console_addr_ = "";
    console_sock_ = -1;
}

MasterNode::~MasterNode() {
    for(auto p : group_mutex_map_) {
        delete(p.second);
    }
}

void MasterNode::SetUpCommandIndexer() {
    client_source_pool_["ASTA"] = kFrontendServer;
    client_source_pool_["STST"] = kKvStore;
    client_source_pool_["QUIT"] = kAnySource;
    //client_source_pool_["PAUS"] = kAdmainConsole;
    //client_source_pool_["CNTE"] = kAdmainConsole;
    //client_source_pool_["AMRG"] = kAdmainConsole;
    //client_source_pool_["AMQT"] = kAdmainConsole;
}

bool makeAddrStruct(string& addr, sockaddr_in& target) {
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = inet_addr(getIpAddr(addr).c_str());
    target.sin_port = htons(getPort(addr));
    return true;
}

int MasterNode::ParseConfig(int argc, char** argv) {
    cxxopts::Options node_opts("Master node", "Parse config info");

    node_opts.add_options()
        ("a,address", "address of node", cxxopts::value<string>())
        ("v,verbose", "Verbose mode", cxxopts::value<bool>())
        ("c,config", "Name of config file",cxxopts::value<string>())
    ;
    
    cxxopts::ParseResult result = node_opts.parse(argc, argv);

    if(result.count("verbose")) {
        vflag_ = 1;
        PrintLog(vflag_, "[Masternode] Verbose mode on");
    }

    //TEST: Keep verbose mode on during development
    vflag_ = 1;

    //Add config file name into attribution, and then get backend addresses using config file
    if(!result.count("config")) {
        cout << "[ERROR] Missing config file" << endl;
        return -1;
    }
    config_filename_ = result["config"].as<string>();

    if(!result.count("address")) {
        cout << "[ERROR] " << "Please specify address and port number" << endl;
        return -1;
    }
    string addr_str = result["address"].as<string>();
    addr_str_ = addr_str;

    if(!isAddrString(addr_str)) {
        cout <<  "[ERROR] " <<"String '" << addr_str << "' is not a valid addr string" << endl;
        return -1; 
    }

    string ip_str = getIpAddr(addr_str);
    int port = getPort(addr_str);

    makeAddrStruct(ip_str, server_inet_);
    PrintLog(vflag_, "[Masternode] Address of server: " + addr_str);

    return 0;
}

int MasterNode::Run() {

    if(!SetupListenFd()) {
        //Stop server if failed to open listen socket
        cout << "Failed to open listen socket" << endl;
        return 1;
    }

    if(!SetupListenAddr()) {
        //Stop server if failed to set listen address
        cout << "Failed to set up listen address" << endl; 
        return 1;
    }
    
    if(listen(listen_fd_, 10) < 0) {
        //Stop if can't listen
        cout << "Failed to listen" << endl;
        close(listen_fd_);
        return 1;
    }

    //set up group_map_
    SetGroup();

    //Try to register to admin console
    thread try_connect_thread([&] { ConnectToAdminWorker(nullptr);});
    try_connect_thread.detach();

    //Create a thread to monitor STST requests
    thread monitor_thread([&] {NodeMonitorWorker(nullptr); });
    monitor_thread.detach();

    while(true) {
        sockaddr client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int comm_fd = accept(listen_fd_, (sockaddr*) &client_addr, &client_addr_len);
        if(comm_fd == -1) {
            cout << "Accept socket closed" << endl;
            return 1;
        }
        //cout << "New client connects on " << comm_fd << endl;
        AddFd(comm_fd);

        thread new_thread([&] {Worker(&comm_fd); });
        new_thread.detach();
    }

    //cout << "ending run" <<endl;

    DisconnectFromAdminConsole();
    close(listen_fd_);
    if(console_addr_ != "") {
        close(console_sock_);
    }

    return 0;

}

bool MasterNode::SetupListenAddr() {
    string ip_addr_str = getIpAddr(addr_str_);
    int port = getPort(addr_str_);
    server_inet_.sin_family = AF_INET;
    server_inet_.sin_port = htons(port);
    inet_pton(AF_INET, ip_addr_str.c_str(), &(server_inet_.sin_addr));

    if(bind(listen_fd_, (sockaddr*)&server_inet_, sizeof(server_inet_)) < 0) {
        cout << "[Masternode] Failed to bind listen address";
        cout << " Error: " << strerror(errno) << endl;
        return false;
    }

    PrintLog(vflag_, "[Masternode] Successfully listened on %s", addr_str_.c_str());
    return true;
}

string CutCmdOfLen4(string msg) {
    return msg.substr(0, 4);
}

void MasterNode::NodeMonitorWorker(void* param) {
    const int kTimeoutLimit = 4;
    while(true) {
        time_t now_ts = time(nullptr);
        for(auto it = node_ts_map_.begin(); it != node_ts_map_.end(); it++) {
            
            nodes_monitor_mutex_.lock();
            int pre_status = node_status_map_[it->first];
            if(now_ts - it->second > kTimeoutLimit) { 
                node_status_map_[it->first] = 0;
            } else {
                node_status_map_[it->first] = 1;
            }

            nodes_monitor_mutex_.unlock();
            //PrintLog(vflag_, "Node %s in status %d at time %ld", it->first.c_str(), node_status_map_[it->first], now_ts);

            //When found a primary node down, assign a new primary
            string node_addr = it->first;
            int group_id = group_indexer_[node_addr];
            int cur_node_status = node_status_map_[node_addr];

            if(primary_map_[group_id] == node_addr && cur_node_status == 0) {
                PrintLog(vflag_, "[S] Primary %s is down.", node_addr.c_str());
                string new_primary = GetBackendAddrWithLeastLoad(group_id);

                //If no node alive in a group, can't assign a new primary
                if(new_primary == "" || !isAddrString(new_primary)) {
                    PrintLog(1, "[S] Cannot assign new primary for group %d", group_id);
                    continue;
                }

                group_mutex_map_[group_id] -> lock();
                AssignNewPrimary(new_primary, group_id);
                PrintLog(vflag_, "[S] Assigned new primary %s for group %d", new_primary.c_str(), group_id);
                group_mutex_map_[group_id] -> unlock();
            }

            //TODO: Report node status to admin console
            if(console_alive_flag_) {
                SendToSocketAndAppendCrlf(console_sock_, "MNRT " + node_addr + "," + to_string(cur_node_status));
            }
    
        }

        sleep(kTimeoutLimit);
    }
}

void MasterNode::Worker(void* param) {
    int comm_fd = *(int*)param;
    //int source_flag = kUnknownSource;

    while (true) {
        char buffer[BUFF_SIZE];
        memset(buffer, 0, sizeof(char) * BUFF_SIZE);
        //read_mutex_.lock();
        if(!DoRead(comm_fd, buffer)) {
            PrintLog(1, "[S %d]Failed to read from socket. Closing connection", comm_fd);
            close(comm_fd);
            //read_mutex_.unlock();
            break;
        }
        //read_mutex_.unlock();
        //PrintLog(vflag_, "[S %d] Received [%s] from buffer", comm_fd, buffer);

        string readin_string(buffer);

        //Format checking
        if(readin_string.size() < 4) { //Messages too short
            //SendErrResponse(comm_fd, "Invalid message " + readin_string);
            PrintLog(1, "[WARNING %d] Masternode got illegal [%s] from fd %d", comm_fd, buffer, comm_fd);
            continue;
        }

        string command = CutCmdOfLen4(readin_string);
        if(!client_source_pool_.count(command)) { //Illegal command
            //SendErrResponse(comm_fd, "Unknown command " + command);
            PrintLog(1, "[WARNING %d] Masternode got unknown command %s from fd %d", comm_fd, command.c_str(), comm_fd);
            continue;
        }

        //Handle commands
        if(command == "ASTA") {
            //Validate request syntax
            if(readin_string.size() <= 5 || readin_string[4] != ' ') {
                //SendErrResponse(comm_fd, "Illegal syntax. Correct one is [ASTA<SP><Username><CR><LF>]");
                PrintLog(vflag_, "[WARNNING %d], Received illegal request %s from frontend server", comm_fd, readin_string.c_str());
                continue;
            }

            HandleAsta(comm_fd, readin_string);

        } 
        
        if(command == "STST") {
            if(readin_string.size() <= 5 || readin_string[4] != ' ') {
                //SendErrResponse(comm_fd, "Illegal syntax. Correct one is [STST<SP><Node addr>,<Number of connections><CR><LF>]");
                PrintLog(vflag_, "[WARNNING %d], Received illegal request %s from frontend server", comm_fd, readin_string.c_str());
                continue;
            }

            string param_str = readin_string.substr(5, readin_string.size() - 5);
            HandleStst(comm_fd, param_str);
            
        } 

        if(command == "QUIT") {
            PrintLog(vflag_, "[Masternode %d] Got QUIT", comm_fd);
            SendOkResponse(comm_fd, "Goodbye");
            break;
        }

        
    }


    close(comm_fd);
    RemoveFd(comm_fd);
}

int StrHash(string str) {
    int hashcode = 0;
    int hash = 0;
    if (hash == 0) {
        int off = 0;
        string val = str;
        int len = str.length();
        for (int i = 0; i < len; i++) {
            hash = 31 * hash + val[off++];
        }
        hashcode = hash;
    }
    return hashcode;
}

int MasterNode::HandleAsta(int comm_fd, string request) {
    //Get the group id using hash
    int group_cnt = group_map_.size();
    cout << "--Group check: there are " << group_cnt << " groups" << endl;
    
    string username = request.substr(5, request.size() - 5);
    int group_id = abs(StrHash(username) % group_cnt);

    cout << "username is " << username << " is expected to be in group "<< group_id << endl;
    
    //If no nodes avaliable for the group id, send err response to frontend node
    if(!load_map_.count(group_id) || load_map_[group_id].size() == 0) {
        PrintLog(1, "[S %d] User %s should be in group %d but no active node avaliable for that group", comm_fd, username.c_str(), group_id);
        SendErrResponse(comm_fd, "User " + username + " should be in group (" + to_string(group_id) + ") but no active node for that group avaliable");
        return 1;
    }
    
    //Get the address using group id
    string target_addr = GetBackendAddrWithLeastLoad(group_id);
    if(target_addr == "") {
        PrintLog(1, "[S %d] Cannot find a storage node for group %d in ASTA", comm_fd, group_id);
        return 1;
    }

    int t = SendToSocketAndAppendCrlf(comm_fd, string(REDIRECT_REQUEST) + " " + target_addr);
    if(!t) {
        bad_fd_ = console_sock_;
        console_alive_flag_ = false; 
    }

    //cout << "Expected to find a node in group " << group_id << endl;
    //cout << "Actually  found  a node in group " << group_indexer_[target_addr] << endl;
    // Group test OK

    return 0;
}

int MasterNode::HandleStst(int comm_fd, string param_str) {
    //cut STST requests
    int comma_idx = param_str.find(',');
    string node_addr = param_str.substr(0, comma_idx);
    int conn_cnt = stoi(param_str.substr(comma_idx + 1, param_str.size() - comma_idx - 1));

    //PrintLog(vflag_, "[S %d] Received STST message from %s, connection number is %d", comm_fd, node_addr.c_str(), conn_cnt);
    //1. if node id not in Monitor Map, add it to ts map and status map
    if(!node_status_map_.count(node_addr)     /* New node */ 
        || node_status_map_[node_addr] == 0   /* Dead node recovers*/) {
        
        nodes_monitor_mutex_.lock();
        node_status_map_[node_addr] = 1;
        node_ts_map_[node_addr] = -1;
        nodes_monitor_mutex_.unlock();
    }

    //cout << "The node id of the address is " << group_indexer_[node_addr] << endl;
    //----Address string mapping should work correctly

    //2. update the timestamp
    node_ts_map_[node_addr] = time(nullptr);

    //3. Update load map
    int group_id = group_indexer_[node_addr];
    group_mutex_map_[group_id]->lock();

    //Access the map by group id
    //When a load map doesn't contain a group, add it
    if(load_map_.count(group_id) == 0) {
        load_map_[group_id][conn_cnt].emplace(node_addr);
    }

    map<int, unordered_set<string>>& cur_load_map = load_map_[group_id];
    int prev_load = (load_indexer_.count(node_addr) ? load_indexer_[node_addr] : -1);
    //cout << "Prev load of " << node_addr << " is " << prev_load << ". Now it's " << conn_cnt << endl;
    //----Load modifier should work correctly

    //if conn_cnt didn't change, go ahead
    if(prev_load != conn_cnt) {
        //Update load map and load indexer
        cur_load_map[prev_load].erase(node_addr);

        if(cur_load_map[prev_load].size() == 0) {
            cur_load_map.erase(prev_load);
        }

        cur_load_map[conn_cnt].emplace(node_addr);
        load_indexer_[node_addr] = conn_cnt;
    }

    group_mutex_map_[group_id]->unlock();

    //4. Report nodes status to admin console
    if(console_alive_flag_) {
        //SendToSocketAndAppendCrlf(console_sock_, "MXXT " + node_addr + "," + to_string(node_status_map_[node_addr]));
        
    }

    //5. Check primary change
    //if a group doesn't have a primary, add it 
    if(primary_map_[group_id] == "") {
        primary_map_[group_id] = node_addr;
        AssignNewPrimary(comm_fd, node_addr, group_id);
        PrintLog(vflag_, "[S %d] Storage node %s is the first storage node for group %d. It's assigned as the primary", comm_fd, node_addr.c_str(), group_id);
        return 0;
    }

    return 0;
}

unordered_set<string> GenerateAddrSet(string config_line) {
    unordered_set<string> result;
    int comma_idx = 0;
    int start_idx = 0;
    while(comma_idx < config_line.size()) {
        while(comma_idx < config_line.size() && config_line[comma_idx] != ',') {
            comma_idx++;
        }

        string cur_addr_str = config_line.substr(start_idx, comma_idx - start_idx);
        result.emplace(cur_addr_str);
        start_idx = comma_idx + 1;
        comma_idx = start_idx; 
    }

    return result;
}

int MasterNode::SetGroup() {
    int is_abosolute_path = 0;

    fs::path path = config_filename_;

    if (path.is_absolute()) {
        is_abosolute_path = 1;
    }

    if (!is_abosolute_path){
        path = fs::absolute(path);
    }

    ifstream config_stream(path);
    if(!config_stream.good()) {
        PrintLog(1, "File %s is not a valid file", path.c_str());
        return -1;
    }

    PrintLog(vflag_, "[S] The config file is good");

    //Read file and set up map
    int group_id = 0;
    string line;
    while(getline(config_stream, line)) {
        unordered_set<string> addr_set = GenerateAddrSet(line);
        group_map_[group_id] = addr_set;

        //Add data to indexer
        for(string node_addr : addr_set) {
            group_indexer_[node_addr] = group_id;
        }

        //Create group mutex
        mutex* m = new mutex();
        group_mutex_map_[group_id] = m;

        group_id++;
    }

    //Initialize primary map
    int group_cnt = group_id;
    for(int i = 0; i < group_cnt; i++) {
        primary_map_[i] = "";
    }
    
    return 0;
}

string MasterNode::GetBackendAddrWithLeastLoad(int group_id) {
    /*if(load_map_.count(group_id) == 0     //Invalid group id
       || load_map_[group_id].size() == 0 //No avaliable node
       || load_map_[group_id].begin()->second.size() == 0) {
        }*/
    //string result = *(load_map_[group_id].begin()->second.begin());

    const int kMaxTry = 10;
    int cnt = 0;

    //If node with least load down within this small period, pick a node within a group randomly
    int min_load = MAX_CLIENTS;
    string target = "";
    for(auto p : node_status_map_) {
        if(p.second && group_indexer_[p.first] == group_id && load_indexer_[p.first] < min_load) {
            target = p.first;
            min_load = load_indexer_[p.first];
        }
    }
    return target;
}

int MasterNode::ConnectToAdminWorker(void* param) {
    //0. If connected, return 0
    if(console_alive_flag_) {
        return 0;
    }

    //1. Try to connect to admin console
    int tmp_sock = socket(PF_INET, SOCK_STREAM, 0);
    if(tmp_sock == -1) {
        PrintLog(1, "[S] Failed to open a socket for admin console");
        return -1;
    }
    console_sock_ = tmp_sock;

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(23333);
    inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));

    const int kMaxTry = 60;
    int try_cnt = 0;

    //Try to connect to admin console for several times
    while (connect(tmp_sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0 && try_cnt < kMaxTry) {
        fprintf(stderr, "[S] Trying to connect to admin console, but have error (%s)\n", strerror(errno));
        try_cnt++;
        sleep(2);
    }

    //Exceeded max try count
    if(try_cnt == kMaxTry) {
        PrintLog(1, "[S] Cannot connect to admin console. System will run without it");
        return -1;
    }

    console_alive_flag_ = 1;
    PrintLog(vflag_, "[S] Successfully connected to admin console");
    //SendToSocketAndAppendCrlf(console_sock_, "MNRG " + addr_str_);

    return 0;
}

    
int MasterNode::DisconnectFromAdminConsole() {
    if(!console_alive_flag_) {
        return 0;
    }

    SendToSocketAndAppendCrlf(console_sock_, "QUIT");
    close(console_sock_);
    return 0;
}

int MasterNode::AssignNewPrimary(int comm_fd, std::string new_primary_addr, int group_id) {
    //TODO: 1. send a ASPR command to new_primary_addr(directly using comm_fd)
    SendToSocketAndAppendCrlf(comm_fd, ASSIGN_NEW_PRIMARY_REQUEST);
    primary_map_[group_id] = new_primary_addr;
    return 0;
}

int MasterNode::AssignNewPrimary(std::string new_primary_addr, int group_id) {
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(getPort(new_primary_addr));
    inet_pton(AF_INET, getIpAddr(new_primary_addr).c_str(), &(servaddr.sin_addr));

    if (connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0) {
        PrintLog(1, "[S] Cannot connect new primary %s", new_primary_addr.c_str());
        fprintf(stderr, "Error is (%s)", strerror(errno));
        return -1;
    }

    AssignNewPrimary(sock, new_primary_addr, group_id);
    close(sock);
    return 0;
}

int MasterNode::RemoveNodeFromNodeMap(string node_addr) {
    int group_id = group_indexer_[node_addr];
    group_mutex_map_[group_id]->lock();

    //Access the map by group id
    map<int, unordered_set<string>>& cur_load_map = load_map_[group_id];
    int prev_load = load_indexer_[node_addr];
    //cout << "Prev load of " << node_addr << " is " << prev_load << ". Now it's " << conn_cnt << endl;
    //----Load modifier should work correctly
    
    //Update load map and load indexer
    cur_load_map[prev_load].erase(node_addr);

    if(cur_load_map[prev_load].size() == 0) {
        cur_load_map.erase(prev_load);
    }

    //cur_load_map[conn_cnt].emplace(node_addr);
    //load_indexer_[node_addr] = conn_cnt;
    

    group_mutex_map_[group_id]->unlock();
    
    return 0;
}