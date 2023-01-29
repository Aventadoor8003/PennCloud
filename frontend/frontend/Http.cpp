#include "Http.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string>
#include <fstream>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <vector>
#include <cstring>
#include <iostream>
#include <unordered_set>
#include <map>
#include "utils.h"
#include "Request.h"
#include "BalanceNode.h"

using namespace std;

extern int BUFF_SIZE;
extern sigset_t mask;
extern pthread_t main_tid;
extern bool sd_flag;
extern std::map<int, pthread_t> thread_pool;
unordered_map<string,Handler*> registeredHandler;
unordered_map<string,string> registeredHTML;

void sig_handler(int signo);
void *shutdown_fc(void *arg);
void *worker(void *arg);
void *workerForBalance(void *arg);

Handler* GetHandler(string);
Html* GetHTML(string path);
int initSocket(unsigned int ip, in_port_t port);

string GetHttpRaw(int comm_fd);

Http::Http(int server_port) {
    main_tid = pthread_self();
    listen_fd = initSocket(htonl(INADDR_ANY), htons(server_port));

    // block SIGINT
    sigemptyset(&mask);
    sigaddset(&mask, SIGINT);
    pthread_sigmask(SIG_BLOCK, &mask, NULL);

    pthread_t shutdown_thread;
    pthread_create(&shutdown_thread, NULL, shutdown_fc, NULL);

    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = sig_handler;
    sigaction(SIGUSR1, &sa, NULL);
}

void Http::Register(string path, Handler* handler,string htmlFile) {
    registeredHandler.emplace(path, handler);
    registeredHTML.emplace(path, htmlFile);
    // cout << "Register:" << path << "->[" << htmlFile << "]" << endl;
}

void Http::ParseConfig(int argc, char** argv){
    string config_doc_name = argv[optind];
    int server_index = stoi(argv[optind + 1]);
    string line;
    ifstream myfile(config_doc_name);
    string server_string_id;
    int idx = 1;
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            if (idx == server_index) {
                server_string_id = Split(line, ':')[1];
            }
            idx++;
        }
        myfile.close();
    }
    else {
        fprintf(stdout, "Unable to open file %s", config_doc_name.c_str());
    }
    this->server_port = stoi(server_string_id);
    cout << "bind port:" << this->server_port << endl;
}

int Http::Run() const {
    int loopNumber = 0;
    if (loopNumber == 0) {
        pthread_t thread;
        pthread_create(&thread, NULL, workerForBalance, nullptr);
    }

    while (!sd_flag)
    {
        struct sockaddr_in clt_addr;			   // define client address
        socklen_t clt_addr_len = sizeof(clt_addr); // get the size of client address
        // user int * for pthread_create() function
        int comm_fd = accept(listen_fd, (struct sockaddr *)&clt_addr, &clt_addr_len);

        if (comm_fd == -1)
        {
            if (sd_flag)
            {
                break;
            }
            else
            {
                error_handler("accept() error");
            }
        }

        pthread_t thread;
        int ret;
        int *thread_ret = NULL;
        int* number = new int(comm_fd);
        ret = pthread_create(&thread, NULL, worker, number);
        thread_pool.insert(pair<int, pthread_t>(comm_fd, thread));

        if (ret != 0)
        {
            error_handler("pthread create error");
        }
        loopNumber++;
    }
    map<int, pthread_t>::iterator iter;
    for (iter = thread_pool.begin(); iter != thread_pool.end(); iter++)
    {
        pthread_kill(iter->second, SIGUSR1);
        close(iter->first);
        pthread_join(iter->second, NULL);
    }
    //
    close(listen_fd);
    return 0;
}

Http::~Http() {

}


void sig_handler(int signo)
{
}

void *shutdown_fc(void *arg)
{
    // sigset_t *mask = (sigset_t *)arg; // mask global varible
    int s, sig;
    while (true)
    {
        s = sigwait(&mask, &sig);
        if (s == 0)
        { // sig == SIGINT
            sd_flag = true;
            pthread_kill(main_tid, SIGUSR1);
            return NULL;
        }
    }
}

void *worker(void *arg)
{
    int comm_fd = *(int *)arg;
    free(arg);
    int ContentLength = -1;
    string httpRaw = GetHttpRaw(comm_fd);
    if (httpRaw.empty()) {
        // todo: fix connection params
        close(comm_fd);// close connection
        // cout << "close" << endl;
        thread_pool.erase(comm_fd);// delete element from thread_pool
        // cout << "erase" << endl;
        pthread_exit(NULL);
    }
    cout << "\nrequest raw:\n" << httpRaw << "----------------------"<< "\n" << endl;

    Request request(httpRaw, comm_fd);

    Handler* handler = GetHandler(request.GetPath());
    Html* html = GetHTML(request.GetPath());
    
    string response;
    if (handler == nullptr || html == nullptr) {
        response = "not found";
    }else {
        response = handler->Process(request,html);
    }
    if (response.size() < 1000) {
        cout << "resp is:\n" <<response <<endl;
    }else {
        cout << "resp is[truncated]:" << endl << response.substr(0,500) << endl;
        cout << "....." << endl;
        cout << response.substr(response.size()-500,500) << endl;
    }
    int ret = write(comm_fd,response.c_str(),response.size());
    // cout << "write down:" <<ret <<". end close cfd:" <<comm_fd <<endl;
    close(comm_fd);// close connection
    // cout << "close" << endl;
    thread_pool.erase(comm_fd);// delete element from thread_pool
    // cout << "erase" << endl;
    pthread_exit(NULL);
    return nullptr;
}

void *workerForBalance(void *arg) {
    auto BalanceServer = new BalanceNode();
    while(!sd_flag) {
        sleep(5);
        //BalanceServer->Send();
    }
    return nullptr;
}

string GetHttpRaw(int comm_fd) {
    string httpRaw;
    char buffer[BUFF_SIZE];
    memset(buffer, 0, sizeof(char) * BUFF_SIZE);
    DoRead(comm_fd, buffer);
    string readin_string(buffer);
    return readin_string;
}

Handler* GetHandler(string path) {
    
    return registeredHandler[path];
}

Html* GetHTML(string path){
    auto html = new Html(registeredHTML[path]);
    if (html->GetRawData().empty()) {
        return nullptr;
    }
    return html;
}

int initSocket(unsigned int ip, in_port_t port) {
    int listen_fd;
    struct sockaddr_in serv_addr;				   // define server address
    memset(&serv_addr, 0, sizeof(serv_addr));	   // clean
    serv_addr.sin_family = AF_INET;				   // address family
    serv_addr.sin_addr.s_addr = ip; 
    serv_addr.sin_port = port;	   

    // define listen socket
    listen_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (listen_fd == -1)
    {
        error_handler("listen socket() error");
    }

    int opt = 1;
    int ret = setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));
    if (ret < 0)
    {
        error_handler("setsockopt() error");
    }

    if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    { // bind listen_fd with server
        error_handler("bind() error");
    }

    if (listen(listen_fd, 200) == -1)
    { // set listen_fd to listen, and set listen upper limit as 200
        error_handler("listen() error");
    }
    return listen_fd;
}