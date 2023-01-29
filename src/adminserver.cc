#include <experimental/filesystem>
#include <fstream>

#include "../include/adminserver.h"
#include "../include/cxxopts.hpp"
#include "../include/StringOperations.hh"
#include "../include/logger16.hpp"

using namespace std;
using namespace log16;
namespace fs = experimental::filesystem;
using namespace adminconsole;

AdminServer::AdminServer() {

}

AdminServer::~AdminServer() {

}

bool makeAddrStruct(string& addr, sockaddr_in& target) {
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = inet_addr(getIpAddr(addr).c_str());
    target.sin_port = htons(getPort(addr));
    return true;
}

int AdminServer::ParseConfig(int argc, char** argv) {
    cxxopts::Options node_opts("Master node", "Parse config info");

    node_opts.add_options()
        ("a,address", "address of node", cxxopts::value<string>())
        ("v,verbose", "Verbose mode", cxxopts::value<bool>())
        ("c,config", "Name of config file",cxxopts::value<string>())
        ("f,frontend_config", "frontend config file", cxxopts::value<string>())
    ;
    
    cxxopts::ParseResult result = node_opts.parse(argc, argv);

    if(result.count("verbose")) {
        vflag_ = 1;
        PrintLog(vflag_, "[S] Verbose mode on");
    }

    //TEST: Keep verbose mode on during development
    vflag_ = 1;

    //Add config file name into attribution, and then get backend addresses using config file
    if(!result.count("config")) {
        cout << "[ERROR] Missing backend config file" << endl;
        return -1;
    }
    string backend_config_filename = result["config"].as<string>();

    if(!result.count("frontend_config")) {
        cout << "[ERROR] Missing frontend config file" << endl;
        return -1;
    }
    string frontend_config_filename = result["frontend_config"].as<string>();

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
    PrintLog(vflag_, "[S] Address of server: " + addr_str);


    //TODO: Set up status map using the two config files
    SetUpBackendStatusMap(backend_config_filename);
    SetUpFrontendStatusMap(frontend_config_filename);
    return 0;
}

bool AdminServer::SetupListenAddr() {
    string ip_addr_str = getIpAddr(addr_str_);
    int port = getPort(addr_str_);
    server_inet_.sin_family = AF_INET;
    server_inet_.sin_port = htons(port);
    inet_pton(AF_INET, ip_addr_str.c_str(), &(server_inet_.sin_addr));

    if(bind(listen_fd_, (sockaddr*)&server_inet_, sizeof(server_inet_)) < 0) {
        cout << "[S] Failed to bind listen address";
        cout << " Error: " << strerror(errno) << endl;
        return false;
    }

    PrintLog(vflag_, "[S] Successfully listened on %s", addr_str_.c_str());
    return true;
}

void AdminServer::SetUpCommandIndexer() {
    command_indexer_["MNRT"] = kMasterNode;
    command_indexer_["LBRT"] = KLoadBalancer;
    command_indexer_["QUIT"] = kAnySrouce;
    //-----------------TEST
    command_indexer_["LIST"] = kAnySrouce;
    command_indexer_["PAUS"] = kAnySrouce;
    command_indexer_["CNTE"] = kAnySrouce;
    command_indexer_["VIEW"] = kAnySrouce;
}
/*
int AdminServer::Stop() {
    close(listen_fd_);
    for(int i = 0; i < fd_cnt_; i++) {
        close(active_fd_[i]);
    }
    run_flag_ = 0;
    return 0;
}*/

int AdminServer::Run() {

    if(!SetupListenFd()) {
        //Stop server
        cout << "Failed to open listen fd" << endl;
        return 1;
    }

    if(!SetupListenAddr()) {
        //Stop server
        cout << "Failed to set up listen address" << endl; 
        return 1;
    }
    
    if(listen(listen_fd_, 10) < 0) {
        cout << "Failed to listen" << endl;
        return 1;
    }

    SetUpCommandIndexer();
    thread frontend_monitor_thread([&] {CheckFrontendStatusWorker(nullptr); });
    frontend_monitor_thread.detach();

    while(run_flag_) {
        sockaddr client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int comm_fd = accept(listen_fd_, (sockaddr*) &client_addr, &client_addr_len);
        if(comm_fd == -1) {
            cout << "Failed to accept" << endl;
            cout << "Error: " << strerror(errno) << endl;
            return 1;
        }

        AddFd(comm_fd);

        thread new_thread([&] {Worker(&comm_fd); });
        new_thread.detach();
    }

    close(listen_fd_);

    return 0;
}

string CutCmdOfLen4(string msg) {
    return msg.substr(0, 4);
}

void AdminServer::Worker(void* param) {
    int comm_fd = *(int*)param;

    while (true) {
        char buffer[BUFF_SIZE];
        memset(buffer, 0, sizeof(char) * BUFF_SIZE);
        if(!DoRead(comm_fd, buffer)) {
            PrintLog(1, "[S %d]Failed to read from socket. Closing connection", comm_fd);
            close(comm_fd);
            break;
        }
        //PrintLog(vflag_, "[S %d] Received [%s] from buffer", comm_fd, buffer);

        string readin_string(buffer);

        //Handle two types of commands:
        if(IsPrivateCommand(readin_string)) {
            int q_flag = HandlePrivateProtocol(comm_fd, readin_string);
            if(q_flag == 1) {
                break;
            }
        } else if(IsHttpRequest(readin_string)) {
            //TODO: Handle http command
        } else {
            PrintLog(1, "Got unknown command %s", readin_string.c_str());
            SendErrResponse(comm_fd, "Unkown command");
        }

        
    }

    close(comm_fd);
    RemoveFd(comm_fd);
}

int AdminServer::HandlePrivateProtocol(int comm_fd, std::string readin_string) {
    string command = readin_string.substr(0, 4);

    if(command == "MNRG") {
        master_alive_flag_ = true;
        PrintLog(vflag_, "Master node successfully connected to admin console");
    }

    if(command == "MNRT") {
        if(readin_string.size() <= 5 || readin_string[4] != ' ') {
            //SendErrResponse(comm_fd, "Illegal syntax. Correct one is [MNRT<SP><Node addr>,<Node status><CR><LF>]");
            PrintLog(vflag_, "[S %d], Received illegal request %s from frontend server", comm_fd, readin_string.c_str());
            return -1;
        }

        HandleMnrt(comm_fd, readin_string);
    }

    /*
    if(command == "LBRT") {
        if(readin_string.size() <= 5 || readin_string[4] != ' ') {
            //SendErrResponse(comm_fd, "Illegal syntax. Correct one is [MNRT<SP><Node addr>,<Node status><CR><LF>]");
            PrintLog(vflag_, "[S %d], Received illegal request %s from frontend server", comm_fd, readin_string.c_str());
            return -1;
        }

        HandleLbrt(comm_fd, readin_string);
    }*/

    if(command == "QUIT") {
        close(comm_fd);
        return 1;
    }

    //--------------------TEST
    if(command == "LIST") {
        SendToSocketAndAppendCrlf(comm_fd, "------Frontend--------");
        for(auto p : frontend_status_map_) {
            SendToSocketAndAppendCrlf(comm_fd, "Frontend " + p.first + " is " + to_string(p.second));
        }

        SendToSocketAndAppendCrlf(comm_fd, "------Backend--------");
        for(auto p : backend_status_map_) {
            SendToSocketAndAppendCrlf(comm_fd, "Backend " + p.first + " is " + to_string(p.second));
        }
    }

    if(command == "PAUS") {
        HandlePaus(comm_fd, readin_string);
    }

    if(command == "CNTE") {
        HandleCnte(comm_fd, readin_string);
    }

    if(command == "VIEW") {
        HandleView(comm_fd, readin_string);
    } 
    //-------------------TEST END

    return 0;
}


int AdminServer::HandleHttpRequest(int comm_fd, std::string request) {
    return 0;
}

bool AdminServer::IsPrivateCommand(std::string command) {
    if(command.size() < 4) {
        return false;
    }

    string cmd = command.substr(0, 4);
    return command_indexer_.count(cmd);
}

int AdminServer::ConnectToServer(std::string target_server) {
    
    string server_ip = getIpAddr(target_server);
    int server_port = getPort(target_server);
    int sock = socket(PF_INET,SOCK_STREAM,0);
    struct sockaddr_in servaddr;
    bzero(&servaddr,sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(server_port);
    inet_pton(AF_INET,server_ip.c_str(),&(servaddr.sin_addr));

    int status = connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr));
    if(status < 0) {
        PrintLog(1, "[S] Cannot connect to server %s", target_server.c_str());
        return -1;
    }

    cout << "Successfully connected to " << target_server << endl;

    return sock;
}

bool AdminServer::IsHttpRequest(std::string request) {
    int space_idx = request.find(' ');
    if(space_idx < 0) {
        return false;
    }
    string first_token = request.substr(0, space_idx);
    return first_token == "GET" || first_token == "PUT";
}


int AdminServer::HandleMnrt(int comm_fd, std::string readin_string) {
    string param_str = readin_string.substr(5, readin_string.size() - 5);
    
    //cut MNRT requests
    int comma_idx = param_str.find(',');
    string node_addr = param_str.substr(0, comma_idx);
    int node_status = stoi(param_str.substr(comma_idx + 1, param_str.size() - comma_idx - 1));

    //TODO: Record this in the status map
    node_status_mutex_.lock();
    backend_status_map_[node_addr] = node_status;
    node_status_mutex_.unlock();

    /*
    for(auto p : backend_status_map_) {
        cout << p.first << "is in status " << p.second << endl;
    }
    */
   //Backend status check should be OK

    //PrintLog(vflag_, "Node %s is in status %d", node_addr.c_str(), node_status);
    return 0;
}

int AdminServer::HandlePaus(int comm_fd, std::string full_msg) {
    //1. Get the address of target address
    int space_idx = full_msg.find(' ');
    if(space_idx < 0) {
        cout << "Illegal PAUS command" << endl;
        return -1;
    }

    string target_addr = full_msg.substr(5, full_msg.size() - 5);
    if(!isAddrString(target_addr)) {
        cout << "Illegal Address for PAUS" << endl;
        return -1;
    }
    
    //2. connect to backend node and send pause command
    int storage_sock = ConnectToServer(target_addr);
    if(storage_sock == -1) {
        cout << "Cannot connect to server " << target_addr << endl;
        return -1;
    }

    //If dead, don't do anything
    if(backend_status_map_.count(target_addr) == 0 || backend_status_map_[target_addr] == 0) {
        PrintLog(1, "[S %d] Node %s doesn't exist or is already dead", comm_fd, target_addr.c_str());
        return -1;
    }

    SendToSocketAndAppendCrlf(storage_sock, "PAUS");
    //cout << "--------------------------Successfully executing paus for" + target_addr << endl;

    //----------TEST
    SendToSocketAndAppendCrlf(comm_fd, "+OK Successfully paused " + target_addr);
    //-----------TEST END
    //3. close connections
    close(storage_sock);
    return 0;
}

int AdminServer::HandleCnte(int comm_fd, std::string full_msg) {
    //1. Get the address of target address
    int space_idx = full_msg.find(' ');
    if(space_idx < 0) {
        cout << "Illegal CNTE command" << endl;
        return -1;
    }

    string target_addr = full_msg.substr(5, full_msg.size() - 5);
    if(!isAddrString(target_addr)) {
        cout << "Illegal Address for CNTE" << endl;
        return -1;
    }
    
    //2. connect to backend node and send pause command
    int storage_sock = ConnectToServer(target_addr);
    if(storage_sock == -1) {
        cout << "Cannot connect to server " << target_addr << endl;
        return -1;
    }

    //If dead, don't do anything
    if(backend_status_map_.count(target_addr) == 0 || backend_status_map_[target_addr] == 1) {
        PrintLog(1, "[S %d] Node %s doesn't exist or is already alive", comm_fd, target_addr.c_str());
        return -1;
    }

    SendToSocketAndAppendCrlf(storage_sock, "CNTE");
    //cout << "--------------------------Successfully executing paus for" + target_addr << endl;

    //----------TEST
    SendToSocketAndAppendCrlf(comm_fd, "+OK Successfully restarted " + target_addr);
    //-----------TEST END
    //3. close connections
    close(storage_sock);
    return 0;
}

int AdminServer::HandleView(int comm_fd, std::string full_msg) {
    //1. Get the address of target address
    int space_idx = full_msg.find(' ');
    if(space_idx < 0) {
        cout << "Illegal VIEW command" << endl;
        return -1;
    }

    string target_addr = full_msg.substr(5, full_msg.size() - 5);
    if(!isAddrString(target_addr)) {
        cout << "Illegal Address for VIEW" << endl;
        return -1;
    }
    
    //2. connect to backend node and send pause command
    int storage_sock = ConnectToServer(target_addr);
    if(storage_sock == -1) {
        cout << "Cannot connect to server " << target_addr << endl;
        return -1;
    }

    //If dead, don't do anything
    if(backend_status_map_.count(target_addr) == 0 || backend_status_map_[target_addr] == 0) {
        PrintLog(1, "[S %d] Node %s doesn't exist or is already dead", comm_fd, target_addr.c_str());
        return -1;
    }

    SendToSocketAndAppendCrlf(storage_sock, "CONT");
    //cout << "--------------------------Successfully executing paus for" + target_addr << endl;

    //TODO: Readin reply
    string content;
    DoRead(storage_sock, content);
    content = content.substr(5, content.size() - 5);
    SendToSocketAndAppendCrlf(comm_fd, "-------Raw content in storage node "  + target_addr);
    SendToSocketAndAppendCrlf(comm_fd, content);
    
    
    //3. close connections
    close(storage_sock);
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

int AdminServer::SetUpBackendStatusMap(std::string config_filename) {
    int is_abosolute_path = 0;

    fs::path path = config_filename;

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

    PrintLog(vflag_, "[S] The backend config file is good");

    //Read file and set up map
    string line;
    while(getline(config_stream, line)) {
        unordered_set<string> addr_set = GenerateAddrSet(line);
        //Add data to indexer
        for(string node_addr : addr_set) {
            backend_status_map_[node_addr] = 0;
        }
    }

    /*
    for(auto p : backend_status_map_) {
        cout << p.first << "is in status " << p.second << endl;
    }*/ 
    // Status initialization should work correctly
    
    return 0;
}

int AdminServer::SetUpFrontendStatusMap(std::string config_filename) {
    int is_abosolute_path = 0;

    fs::path path = config_filename;

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

    PrintLog(vflag_, "[S] The frontend config file is good");

    //Read file and set up map
    string line;
    while(getline(config_stream, line)) {
        unordered_set<string> addr_set = GenerateAddrSet(line);
        //Add data to indexer
        for(string node_addr : addr_set) {
            frontend_status_map_[node_addr] = 0;
        }
    }

    /*
    cout << "Frontend check" << endl;
    for(auto p : frontend_status_map_) {
        cout << p.first << "is in status " << p.second << endl;
    } */
    // Status initialization should work correctly
    
    return 0;
}

void AdminServer::CheckFrontendStatusWorker(void* param) {
    //check every 2s
    while(true){

        for(auto p : frontend_status_map_){
            string node_addr = p.first;

            string server_ip=getIpAddr(node_addr);
            int server_port=getPort(node_addr);
            int frontfd=socket(PF_INET,SOCK_STREAM,0);
            struct sockaddr_in servaddr;
            bzero(&servaddr,sizeof(servaddr));
            servaddr.sin_family=AF_INET;
            servaddr.sin_port=htons(server_port);
            inet_pton(AF_INET,server_ip.c_str(),&(servaddr.sin_addr));
            //successfully connect to frontend NodeAddrIdentifier
            if (connect(frontfd,(struct sockaddr*)&servaddr,sizeof(servaddr))==0){
                frontend_status_map_[node_addr] = 1;
            } else {
                frontend_status_map_[node_addr] = 0;
            }

            close(frontfd);
        }
        sleep(2);
    }
}
