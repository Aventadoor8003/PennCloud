#include "../include/storageserver.h"
#include "../include/StringOperations.hh"
#include "../include/logger16.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <mutex>
#include <queue>

using namespace std;
using namespace log16;

bool shutting_down = false; // signal handler ctrl c indicater
// bool write_log = true; // Indicate whether need to write to log (PUT, GET and DELETE need to write to log)
bool pausing = false;
bool heartbeat_status = true; //Indicate whether send heart beat
const int MAXIMUM_LOG_COUNT = 50; // When log_count reaches MAXIMUM_LOG_COUNT, save checkpoint
int primary_index = 1; // Define primary node's index
int log_count = 0;
int connection_num = 0;
int group_index;
int server_index;
vector<string>group;
thread heartbeat_thread;
// mutex write_log_lock;
// mutex forward_msg_lock;
mutex change_table_lock;
// mutex primary_index_lock;
mutex message_queue_lock;
queue<pair<string, int>> message_queue;
bool forward_worker_alive;


StorageServer::StorageServer () : BaseServer() {}

StorageServer::~StorageServer(){}

StorageServer::StorageServer (std::string server_string_id) {
    this->server_string_id = server_string_id;
    bzero(&this->server_sock_id, sizeof(this->server_sock_id));
    this->server_sock_id.sin_family = AF_INET;
    this->server_addr = getIpAddr(server_string_id);
    this->server_port = getPort(server_string_id);
    this->server_sock_id.sin_addr.s_addr = inet_addr(server_addr.c_str());
    this->server_sock_id.sin_port = htons(server_port);
    this->tablet = Tablet();
}

void StorageServer::Update(std::string server_string_id) {
    this->server_string_id = server_string_id;
    bzero(&this->server_sock_id, sizeof(this->server_sock_id));
    this->server_sock_id.sin_family = AF_INET;
    this->server_addr = getIpAddr(server_string_id);
    this->server_port = getPort(server_string_id);
    this->server_sock_id.sin_addr.s_addr = inet_addr(server_addr.c_str());
    this->server_sock_id.sin_port = htons(server_port);
    this->tablet = Tablet();
}

int StorageServer::ParseConfig(int argc, char** argv) {
    string config_doc_name = argv[optind];
    server_index = stoi(argv[optind + 1]);
    string line;
    ifstream myfile(config_doc_name);
    string server_string_id;
    int idx = 1;
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            if (idx == server_index) {
                server_string_id = line;
            }
            idx++;
        }
        myfile.close();
    }
    else {
        PrintLog(vflag_, "Unable to open file" + config_doc_name);
    }

    this->Update(server_string_id);

    return 0;
}

int StorageServer::ParseConfigWithReplica(int argc, char** argv) {
    string config_doc_name = argv[optind];
    group_index = stoi(argv[optind + 1]);
    server_index = stoi(argv[optind + 2]);
    string line;
    ifstream myfile(config_doc_name);
    string server_string_id;
    string group_string_ids;

    int idx = 1;
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            if (idx == group_index) {
                group_string_ids = line;
                stringstream ss(group_string_ids);
                while(ss.good()) {
                    string substr;
                    getline(ss, substr, ',');
                    group.push_back(substr);
                }
                server_string_id = group[server_index-1];
            }
            idx++;
        }
        myfile.close();
    }
    else {
        PrintLog(vflag_, "Unable to open file" + config_doc_name);
    }

    this->Update(server_string_id);

    return 0;
}

string StorageServer::DoPut(string r, string c, string v, string message) {
    string output;

    if (this->tablet.data.find(r) == this->tablet.data.end()) {
        /* new rowkey/user */
        map<string, vector<char>> new_row;
        vector<char> value_char(v.begin(),v.end());
        new_row[c] = value_char;
        this->tablet.data[r] = new_row;  
        output = "OKPT\r\n";
        PrintLog(vflag_,"+OK Put a new file for new user successfully\r\n");
    } else {
        /* existing rowkey/user */
        map<string, vector<char>> &row = this->tablet.data[r];
        if (row.find(c) == row.end()) {
            /* new file */
            vector<char> value_charvec(v.begin(),v.end());
            row[c] = value_charvec;
            output = "OKPT\r\n";
            PrintLog(vflag_,"+OK Put a new file for existing user successfully\r\n");

        } else {
            /* file already exists */
            output = "ERPT file alreay exists\r\n";
            PrintLog(vflag_,"-ERR file arealy exists for this user\r\n");
        }
    }
    
    return output;
}

string StorageServer::DoGet(string r, string c, string message) {
    string output;
    if (this->tablet.data.find(r) != this->tablet.data.end()) {
        /* user exists */
        map<string, vector<char>> &row = this->tablet.data[r];
        if (row.find(c) != row.end()) {
            /* both user and file exist */
            vector<char> value_charvec = this->tablet.data[r][c];
            string value_str(value_charvec.begin(), value_charvec.end());
            output = "OKGT " + value_str + "\r\n";
            PrintLog(vflag_, "+OK get successfully\r\n");
        } else {
            /* user exists but file does not exist */
            output = "ERGT user exists but file does not exist\r\n";
            PrintLog(vflag_, "-ERR user exists but file does not exist\r\n");
        }
    } else {
        /* rowkey/user does not exist in tablet */
        output = "ERGT user does not exist\r\n";
        PrintLog(vflag_, "-ERR user does not exist\r\n");
    }
    
    return output;
}

string StorageServer::DoCput(string r, string c, string v1, string v2, string message) {
    string output;
    if (this->tablet.data.find(r) != this->tablet.data.end()) {
        /* user exists */
        map<string, vector<char>> &row = this->tablet.data[r];
        if (row.find(c) != row.end()) {
            // vector<char> value1_char(v1.begin(),v1.end());
            string current_value(row[c].begin(), row[c].end());
            if (current_value == v1) {
                vector<char> value2_char(v2.begin(),v2.end());
                row[c] = value2_char;
                output = "OKCP\r\n";
                PrintLog(vflag_, "+OK Cput successfully\r\n");
            } else {
                /* file exists but current value does not match */
                output = "ERCP file exists but current value does not match\r\n";
                PrintLog(vflag_, "-ERR file exists but current value does not match\r\n");
            }
        } else {
            /* user exists but file does not exist */
            output = "ERCP user exists but file does not exist\r\n";
            PrintLog(vflag_, "-ERR user exists but file does not exist\r\n");
        }
    } else {
        /* user does not exist */
        output = "ERCP user does not exist\r\n";
        PrintLog(vflag_, "-ERR user does not exist\r\n");
    }
    return output;
}

string StorageServer::DoDelete(string r, string c, string message) {
    string output;
    if (this->tablet.data.find(r) != this->tablet.data.end()) {
        /* user exists */
        map<string, vector<char>> &row = this->tablet.data[r];
        if (row.find(c) != row.end()) {
            /* both user and file exist */
            row.erase(c);
            output = "OKDL\r\n";
            PrintLog(vflag_, "+OK delete successfully\r\n");
        } else {
            /* user exists but file does not exist */
            output = "ERDL user exists but file does not exist\r\n";
            PrintLog(vflag_, "-ERR user exists but file does not exist\r\n");
        }
    } else {
        /* user does not exist */
        output = "ERDL user does not exist\r\n";
        PrintLog(vflag_, "-ERR user does not exist\r\n");
    }
    return output;
}

bool StorageServer::SetupListenAddr() {
    server_inet_ = this->server_sock_id;

    if(bind(listen_fd_, (sockaddr*)&server_inet_, sizeof(server_inet_)) < 0) {
        return false;
    }

    return true;
}

void StorageServer::Worker(void* param) {
    connection_num += 1;
    int comm_fd = *(int*)param;
    // PrintLog(vflag_, "+OK Server ready (Author: Yunshu Huang /yunshuh)\r\n");
    bool write_log;

    while(!shutting_down) {
        write_log = false;
        // char buffer[BUFF_SIZE];
        // memset(buffer, 0, sizeof(char) * BUFF_SIZE);
        string buffer;
        if(!DoRead(comm_fd, buffer))
            break;

        string readin_string(buffer);
        PrintLog(vflag_, "[%d] Read in message: %s",comm_fd, readin_string.c_str());
        
        string command = readin_string.size() >= 4 ? readin_string.substr(0, 4) : "UNKNOWN";
        string message = readin_string.size() >= 6 ? readin_string.substr(5, readin_string.size() - 5) : "";
        
        if (command == "FORW") {
            int pos = message.find(";");
            string forward_readin_string = message.substr(pos + 1, message.size()- pos - 1);
            string forward_command = forward_readin_string.size() >= 4 ? forward_readin_string.substr(0, 4) : "UNKNOWN";
            string forward_message = forward_readin_string.size() >= 6 ? forward_readin_string.substr(5, forward_readin_string.size() - 5) : "";
        
            write_log = ProcessOperations(comm_fd, forward_command, forward_message, readin_string, 2);
        } else if (command == "PUTT" || command == "CPUT" || command == "DELE") {
            if (server_index != primary_index) {
                string response = ForwardMessage(primary_index, readin_string);
                DoWrite(comm_fd, response.c_str(), response.length());
            } else {
                message_queue_lock.lock();
                message_queue.push({readin_string, comm_fd});
                message_queue_lock.unlock();
                cout << "pushed to message queue" << endl;
            }
        } else {
            ProcessOperations(comm_fd, command, message, readin_string, 1);
        }

        if (server_index != primary_index) {
            cout << "server " << server_index << " writing log: " << readin_string << endl;
            /* Write command + message to log file */
            WriteLog(write_log, readin_string);
            if (log_count == MAXIMUM_LOG_COUNT) {
                /* Write tablet to checkpoint file periodically */
                WriteCheckPoint();
                CreateLog(); // clear log file
                log_count = 0;
            }
        }
    }
    // RemoveFd(comm_fd);
    close(comm_fd);
    connection_num -= 1;
}

bool StorageServer::ProcessOperations(int comm_fd, string command, string message, string readin_string, int type) {
    string output;
    string rowKey;
    string colKey;
    string value1;
    string value2;
    bool write_log = true;
    cout << "processing opt: "<< command << " message: " << message << endl;

    if (command == "PUTT") {
        rowKey = getRowKey(message);
        colKey = getColKey(message);
        value1 = getFirstValue(message);

        // change_table_lock.lock();
        output = DoPut(rowKey, colKey, value1, message);
        // change_table_lock.unlock();
        if (type != 0)
            DoWrite(comm_fd, output.c_str(), output.length());
        cout << "PUTT completed: " << output << endl;
    } else if (command == "GETT") {
        write_log = false;
        rowKey = getRowKey(message);
        colKey = getColKey(message);
        output = DoGet(rowKey, colKey, message);
        if (type == 1)
            DoWrite(comm_fd, output.c_str(), output.length());
    } else if (command == "CPUT") {
        rowKey = getRowKey(message);
        colKey = getColKey(message);
        value1 = getFirstValue(message);
        value2 = getSecondValue(message);

        output = DoCput(rowKey, colKey, value1, value2, message);
        if (type != 0)
            DoWrite(comm_fd, output.c_str(), output.length());
    } else if (command == "DELE") {
        rowKey = getRowKey(message);
        colKey = getColKey(message);
        
        output = DoDelete(rowKey, colKey, message);
        if (type != 0)
            DoWrite(comm_fd, output.c_str(), output.length());
    } else if (command == "ASPR") {
        write_log = false;
        if (type == 1) {
            ResetIndex();
            // start forward worker as primary
            if (primary_index == server_index && !forward_worker_alive) {
                thread forward_worker_thread([&] {ForwardWorker(); });
                forward_worker_thread.detach();
            }
        }
        else if (type == 2) {
            primary_index = stoi(message.substr(0,message.size()-2));
        }
        output = "+OK new primary set\r\n";
        if (type != 0)
            DoWrite(comm_fd, output.c_str(), output.length());
    } else if (command == "WSPR") {
        write_log = false;
        output = to_string(primary_index) + ";" +to_string(log_count) + "\r\n";
        DoWrite(comm_fd, output.c_str(), output.length());
    } else if (command == "PAUS") {
        // pausing = true;
        write_log = false;
        heartbeat_status = false;
    } else if (command == "CNTE") {
        // pausing = false;
        ReadLog();
        write_log = false;
        heartbeat_status = true;
    } else if (command == "CONT") {
        write_log = false;
        output = "OKDT " + tabletToString(this->tablet)+ "\r\n";
        if (type != 0)
            DoWrite(comm_fd, output.c_str(), output.length());
    } else {
        write_log = false;
        string unknown_str = "-ERR Unknown command\r\n";
        if (type != 0)
            DoWrite(comm_fd, unknown_str.c_str(), unknown_str.size());
    }
    return write_log;
}

void StorageServer::ForwardWorker() {
    vector<int> fds = vector<int>(group.size());
    for (int i = 1; i <= group.size(); i++) {
        if (i == primary_index) continue;
        int sock = socket(PF_INET, SOCK_STREAM, 0);
        StorageServer dest_server(group[i - 1]);
        if (connect(sock, (struct sockaddr*)&dest_server.server_sock_id, sizeof(dest_server.server_sock_id)) == 0) {
            PrintLog(vflag_, "Primary established connection to " + dest_server.server_string_id);
            fds[i - 1] = sock;
        } else {
            fds[i - 1] = -1;
            close(sock);
        }
    }
    while(!shutting_down && (message_queue.size() > 0 || server_index == primary_index)) {
        forward_worker_alive = true;
        if (message_queue.size() > 0) {
            message_queue_lock.lock();
            string readin_string = message_queue.front().first;
            int comm_fd = message_queue.front().second;
            message_queue.pop();
            message_queue_lock.unlock();
            cout << "process message from queue: " << readin_string << " fd: " << comm_fd << endl;

            string command = readin_string.size() >= 4 ? readin_string.substr(0, 4) : "UNKNOWN";
            string message = readin_string.size() >= 6 ? readin_string.substr(5, readin_string.size() - 5) : "";
            bool write_log = ProcessOperations(comm_fd, command, message, readin_string, 1);

            /* Write command + message to log file */
            WriteLog(write_log, readin_string);
            if (log_count == MAXIMUM_LOG_COUNT) {
                /* Write tablet to checkpoint file periodically */
                WriteCheckPoint();
                CreateLog(); // clear log file
                log_count = 0;
            }
            string forward_message = "FORW;"+to_string(server_index)+';'+readin_string+"\r\n";

            int count_conn = 0;
            for (int i = 1; i <= group.size(); i++) {
                if (i == primary_index) continue;
                if (fds[i - 1] > 0) count_conn++;
            }
            cout << "count_conn: " << count_conn << endl;

            for (int i = 1; i <= group.size(); i++) {
                if (i == primary_index) continue;
                if (fds[i - 1] > 0) {
                    if(!DoWrite(fds[i - 1], forward_message.c_str(), forward_message.length())) {
                        cout << "failed to forward to rep" << endl;
                        fds[i - 1] = -1;
                    } else {
                        cout << "forwarded: " << forward_message << endl;
                    }
                }
            }
        }
        
        for (int i = 1; i <= group.size(); i++) {
            if (i == primary_index) continue;
            if (fds[i - 1] < 0) {
                int sock = socket(PF_INET, SOCK_STREAM, 0);
                StorageServer dest_server(group[i - 1]);
                if (connect(sock, (struct sockaddr*)&dest_server.server_sock_id, sizeof(dest_server.server_sock_id)) == 0) {
                    PrintLog(vflag_, "Primary established connection to " + dest_server.server_string_id);
                    fds[i - 1] = sock;
                } else {
                    close(sock);
                }
            }
        }
        // cout << "active connection to rep: " << count_conn << endl;
    }
    forward_worker_alive = false;
}

void StorageServer::SendHeartBeats(void* param) {
    int period = *(int*)param;

    int sock = socket(PF_INET, SOCK_STREAM, 0);
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family=AF_INET;
    servaddr.sin_port=htons(2334);
    inet_pton(AF_INET, "127.0.0.1", &(servaddr.sin_addr));
    if (connect(sock, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
        PrintLog(vflag_, "Cannot connect to master node (%s)", strerror(errno));
    while (!shutting_down) {
        string heart_beat = "STST " + this->server_string_id + "," + to_string(connection_num) + "\r\n";
        if (heartbeat_status)
            DoWrite(sock, heart_beat.c_str(), heart_beat.length());
        sleep(period);
    }
    string shutdown_msg = "QUIT\r\n";
    DoWrite(sock, shutdown_msg.c_str(), shutdown_msg.length());
    close(sock);
    exit(0);
}

void StorageServer::CreateLog() {
    string path_log = "./log/" + this->server_string_id + "_log.txt";
    ofstream file_log(path_log);
}
void StorageServer::CreateCheckPoint() {
    string path_checkpoint = "./log/" + this->server_string_id + "_checkpoint.txt";
    ofstream file_checkpoint(path_checkpoint);
}

void StorageServer::WriteLog(bool write_log, string readin_string) {
    if (write_log) {
        // write_log_lock.lock();
        if (readin_string.substr(0,4) == "FORW") {
            int pos1 = readin_string.find(";");
            string readin_string1 = readin_string.substr(pos1 + 1, readin_string.size() - pos1 + 1);
            int pos2 = readin_string1.find(";");
            string readin_string2 = readin_string1.substr(pos2+ 1, readin_string1.size() - pos2 + 1);
            readin_string = readin_string2;
        }
        log_count += 1;
        string path = "./log/" + this->server_string_id + "_log.txt";
        fstream myfile;
        myfile.open(path, fstream::app);
        myfile << readin_string << endl;
        myfile.close();
        // write_log_lock.unlock();
    }
    
}

void StorageServer::WriteCheckPoint() {
    // write_log_lock.lock();
    string path = "./log/" + this->server_string_id + "_checkpoint.txt";
    ofstream myfile;
    myfile.open(path, ofstream::out | ofstream::trunc);
    string current_data = tabletToString(this->tablet);
    myfile << current_data << endl;
    myfile.close();
    // write_log_lock.unlock();
}

void StorageServer::ClearLog() {
    string path = "../log/" + this->server_string_id + "_log.txt";

}

void StorageServer::ParseArgs(int argc, char *argv[]) {
    int c;

    while((c = getopt(argc, argv, "v")) != -1) {
		switch (c) {
			case 'v':
                vflag_ = 1;
                break;
            case '?':
                exit(2);
			default:
                exit(3);
				abort();
		}
	}
}


string StorageServer::get_ip_from_file(char *argv[], StorageServer &table) {
    string config_doc_name = argv[optind];
    int server_index = stoi(argv[optind + 1]);
    string line;
    ifstream myfile(config_doc_name);
    string forward_addr;
    string bind_addr;
    int idx = 1;
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            if (idx == server_index) {
                table.server_string_id = line;
            }
            idx++;
        }
        myfile.close();
    }
    else {
        PrintLog(vflag_, "Unable to open file" + config_doc_name);
    }
    return table.server_string_id;
}

int StorageServer::Run() {

    if(!SetupListenFd()) {
        //TODO: Stop server
        cout << "Failed to open listen fd" << endl;
        return 1;
    }

    if(!SetupListenAddr()) {
        //TODO: Stop server
        cout << "Failed to set up listen address" << endl; 
        return 1;
    }
    
    if(listen(listen_fd_, 10) < 0) {
        cout << "Failed to listen" << endl;
        return 1;
    }

    /* Periodically send master node heartbeats */
    int period = 1;
    thread heartbeat_thread([&] {SendHeartBeats(&period); });
    heartbeat_thread.detach();

    /* Read Log File if exists */
    if (CheckLogExist()) {
        ReadLog();
    } else {
        CreateLog();
        CreateCheckPoint();
    }

    if (primary_index == server_index && !forward_worker_alive) {
        thread forward_worker_thread([&] {ForwardWorker(); });
        forward_worker_thread.detach();
    }

    while(!shutting_down) {
        sockaddr client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int comm_fd = accept(listen_fd_, (sockaddr*) &client_addr, &client_addr_len);
        if(comm_fd == -1) {
            PrintLog(vflag_, "Failed to accept (%d)", strerror(errno));
            return 1;
        }
        // AddFd(comm_fd);
        thread new_thread([&] {Worker(&comm_fd); });
        new_thread.detach();
    }
    return 0;
}

string StorageServer::ForwardMessage(int dest_server_index, std::string message) {
    // forward_msg_lock.lock();
    string response;
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    StorageServer dest_server(group[dest_server_index - 1]);
    PrintLog(vflag_, "Forward message to server " + dest_server.server_string_id);
    if (message[message.length() - 1] != '\n')
        message.push_back('\n');
    if (connect(sock, (struct sockaddr*)&dest_server.server_sock_id, sizeof(dest_server.server_sock_id)) == 0) {
        int iswrite = DoWrite(sock, message.c_str(), message.length());
        cout << "forwarding: " << message.substr(0, message.length()-2) << " | " << server_index << " | " << dest_server_index << " | " << primary_index << " | " << sock << " | " << iswrite << endl;
        DoRead(sock, response);
    } else {
        PrintLog(vflag_, "Cannot connect to node for forward");
        response = "ERROR\r\n";
    }
    close(sock);
    return response;
    // forward_msg_lock.unlock();
}

string StorageServer::GetPrimaryAndLogCount(int dest_server_index) {
    string message = "WSPR\r\n";
    int sock = socket(PF_INET, SOCK_STREAM, 0);
    StorageServer dest_server(group[dest_server_index - 1]);
    PrintLog(vflag_, "Ask other servers who primary is, asking server " + dest_server.server_string_id);
    if (connect(sock, (struct sockaddr*)&dest_server.server_sock_id, sizeof(dest_server.server_sock_id)) == 0) {
        DoWrite(sock, message.c_str(), message.length());
    } else {
        PrintLog(vflag_, "Cannot connect to node for asking primary");
    }
    string response;
    if(!DoRead(sock, response))
        PrintLog(vflag_, "Cannot get response from node for asking primary");
    close(sock);

    return response;

}

void signal_handler(int signum) {
    shutting_down = true;
    cout << "shutting down set to true" << endl;
    
    // exit(signum);
}

void StorageServer::ResetIndex() {
    /* set the current server that received "ASPR" to be the new primary */
    // primary_index_lock.lock();
    primary_index = server_index;
    // primary_index_lock.unlock(); 
    /* Foward who the new primary is to all the other servers */
    for (int i = 1; i < group.size()+1; i++) {
        if (i != server_index) {
            ForwardMessage(i, "FORW;"+to_string(server_index)+";ASPR "+to_string(primary_index)+"\r\n");
        }
    }
    
}

bool StorageServer::CheckLogExist() {
    string path_log = "./log/" + this->server_string_id + "_log.txt";
    string path_checkpoint = "./log/" + this->server_string_id + "_checkpoint.txt";
    ifstream file_log(path_log);
    ifstream file_checkpoint(path_checkpoint);
    return file_log.good() && file_checkpoint.good();

}

void StorageServer::ReadLog() {
    /* ask other nodes who primary is */
    for (int i = 1; i < group.size()+1; i++) {
        if (i != server_index) {
            string response = GetPrimaryAndLogCount(i);
            int pos = response.find(";");
            cout<<"log_count: "<<response.substr(pos + 1, response.size() - pos - 3)<<endl;
            // primary_index_lock.lock();
            primary_index = stoi(response.substr(0, pos));
            // primary_index_lock.unlock();
            log_count = stoi(response.substr(pos + 1, response.size() - 1));
            break;
        }
    }
 

    string primary_node_id_str = group[primary_index-1];
    cout<<"primary_node_id_str: "<< primary_node_id_str <<endl;

    string primary_path_log = "./log/" + primary_node_id_str + "_log.txt";
    string primary_path_checkpoint = "./log/" + primary_node_id_str + "_checkpoint.txt";

    string my_path_log = "./log/" + this->server_string_id + "_log.txt";
    string my_path_checkpoint = "./log/" + this->server_string_id + "_checkpoint.txt";
    
    // write_log_lock.lock();
    /* recover checkpoint from primary node*/
    ifstream primary_checkpoint(primary_path_checkpoint);
    ofstream my_checkpoint;
    my_checkpoint.open(my_path_checkpoint, ofstream::out | ofstream::trunc);
    string checkpoint_line;
    if (primary_checkpoint.is_open()) {
        while (getline(primary_checkpoint, checkpoint_line)) {
            this->tablet = stringToTablet(checkpoint_line);
            /* copy primary's checkpoint to the current node's */
            my_checkpoint << checkpoint_line << endl;
        }
        primary_checkpoint.close();
        my_checkpoint.close();
    }

    /* recover logs from primary node*/    
    ifstream primary_log(primary_path_log);
    ofstream my_log;
    my_log.open(my_path_log, ofstream::out | ofstream::trunc);
    string log_line;
    if (primary_log.is_open()) {
        while(getline(primary_log, log_line)) {
            string command = log_line.size() >= 4 ? log_line.substr(0, 4) : "UNKNOWN";
            string message = log_line.size() >= 6 ? log_line.substr(5, log_line.size() - 5) : "";
            ProcessOperations(0, command, message, log_line, 0);
            /* copy primary's logs to the current node's */
            my_log << log_line << endl;
        }
        primary_log.close();
        my_log.close();
    }
    // write_log_lock.unlock();
}
