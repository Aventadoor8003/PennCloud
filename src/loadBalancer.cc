#include <iostream>
#include "../include/loadBalancer.h"
#include "../include/cxxopts.hpp"
#include "../include/StringOperations.hh"
#include "../include/logger16.hpp"
#include <fstream>
#include <map>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

using namespace std;
using namespace log16;

LoadBalancer::LoadBalancer():BaseServer(){
  pthread_mutex_init(&mutex_,0);
}
LoadBalancer::~LoadBalancer(){
  pthread_mutex_destroy(&mutex_);
}

bool makeAddrStruct(string& addr, sockaddr_in& target) {
    target.sin_family = AF_INET;
    target.sin_addr.s_addr = inet_addr(getIpAddr(addr).c_str());
    target.sin_port = htons(getPort(addr));
    return true;
}

bool LoadBalancer::SetupListenAddr(){
    

    if(bind(listen_fd_, (sockaddr*)&server_inet_, sizeof(server_inet_)) < 0) {
        //cout << "Failed to bind addr";
        return false;
    }

    //cout << "Successfully bind on 0.0.0.0:4711" << endl;
    return true;
}

int LoadBalancer::ParseConfig(int argc,char** argv){
    int n;
    int vFlag;
    int pFlag;
    char* port=nullptr;
    
    string ip_addr="127.0.0.1:";
    while((n=getopt(argc,argv,"vp:"))!=-1){
    switch(n){
      case 'v':
        vFlag=1;
        break;

      case 'p':
        pFlag=1;
        port=optarg;
        break;
      case '?':
      if(optopt=='p'){
        fprintf(stderr,"option -%c requires an argument.\n",optopt);
        return 1;
      }else if(isprint(optopt)){
        fprintf(stderr,"Unknown option `-%c'.\n",optopt);
        return 1;
      }else{
        fprintf(stderr,"Unknown option character `\\x%x'.\n",optopt);
        return 1;
      }
      default:
        abort();
    }
    }
    if(pFlag==1){
        string portnumber(port);
        ip_addr+=portnumber;
        cout<<"Current ip addr is "+ip_addr<<endl;
				addr_str_ = ip_addr;
        makeAddrStruct(ip_addr,server_inet_);
    }

    int index=optind;
    char* filename=argv[index];
    cout<<filename<<endl;
    numserver_=loadServerConfig(filename);
    printf("Currently there are %d servers in config file",numserver_);
    return 0;

}

int LoadBalancer::loadServerConfig(const char* filename){
    //load config.txt and initialize server map
    std::ifstream configFile(filename);
    int count=0;
    for(string line; getline(configFile,line);){
        ServerInfo server;
        server.ip_addr=line;
        server.status=0;
        servers_[count++]=server;
        cout<<line<<endl;
    }
    return count;
}

int LoadBalancer::DisconnectFromAdmin() {
	if(!console_alive_flag_) {
		return 0;
	}

  SendToSocketAndAppendCrlf(console_sock_, "QUIT");
  close(console_sock_);
	return 0;
}

int LoadBalancer::updateAvailableServer(){

	thread new_thread([&] {checkHeartbeat(nullptr); });
	new_thread.detach();
	return 0;
}

int LoadBalancer::Run() {

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

	//TODO: Add try connect worker
	thread try_connect_thread([&] {TryConnectAdminWorker(nullptr); });

	while(true) {
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

	//Disconnect from admin when stops
	DisconnectFromAdmin();

	return 0;
}

int LoadBalancer::TryConnectAdminWorker(void* param) {
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
		PrintLog(1, "Cannot connect to admin console. System will run without it");
		return -1;
	}

	console_alive_flag_ = 1;
	PrintLog(vflag_, "Successfully connected to admin console");
	SendToSocketAndAppendCrlf(console_sock_, "LBRG " + addr_str_);

	return 0;
}



void LoadBalancer::checkHeartbeat(void* param){
    //check every 2s
    while(true){

        for(int i=0;i<numserver_;i++){
          ServerInfo server=servers_[i];

					//TEST LINE

          string server_ip=getIpAddr(server.ip_addr);
          int server_port=getPort(server.ip_addr);
          int frontfd=socket(PF_INET,SOCK_STREAM,0);
          struct sockaddr_in servaddr;
          bzero(&servaddr,sizeof(servaddr));
          servaddr.sin_family=AF_INET;
          servaddr.sin_port=htons(server_port);
          inet_pton(AF_INET,server_ip.c_str(),&(servaddr.sin_addr));
          //successfully connect to frontend server
          if(connect(frontfd,(struct sockaddr*)&servaddr,sizeof(servaddr))==0){
            pthread_mutex_lock(&mutex_);
            servers_[i].status=1;
            pthread_mutex_unlock(&mutex_);
            cout<<"Server "+servers_[i].ip_addr+" is available to use"<<endl;
          }else{
            pthread_mutex_lock(&mutex_);
            servers_[i].status=0;
            pthread_mutex_unlock(&mutex_);
            cout<<"Server "+servers_[i].ip_addr+" is down..."<<endl;
          }

					//TODO: Send message to admin console using FRST
					//SendToSocketAndAppendCrlf(console_sock_, "FRST " + server.ip_addr + "," + to_string(servers_[i].status));

          close(frontfd);
        }
        sleep(2);
    }
}


void LoadBalancer::Worker(void* param){
    int comm_fd = *(int*)param;
    cout << "LoadBalancer starts" << endl;
    //find an available server to use
    pthread_mutex_lock(&mutex_);
    int chosen=-1;
    int cnt=0;
    while(cnt<numserver_){
      cout<<cnt<<endl;
      cout<<currHandler_<<endl;
      cout<<servers_[currHandler_].status<<endl;
      if(servers_[currHandler_].status==1){
        chosen=currHandler_;
        currHandler_++;
        break;
      }else{
        if(++currHandler_==numserver_) currHandler_=0;
        cnt++;
      }
    }
    pthread_mutex_unlock(&mutex_);
    string header = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: ";
    string page;
    cout<<numserver_<<endl;
    cout<<cnt<<endl;
    cout<<chosen<<endl;
    
    //if there's no available server
    if(cnt>=numserver_){
      page="<html><head><title>hello world</title></head>\n<body>\n<h1>Sorry</h1>\n<p>There is no available server now</p>\n</body>\n</html>\n";
      
    }else{
      //else: return the available server to client
      ServerInfo chosenServer=servers_[chosen];
      string front_addr=chosenServer.ip_addr;
      page="<html><head><title>hello world</title></head>\n<body>\n<h1>Redirect</h1>\n<p>Redirect to frontend server <a href=\"http://"+front_addr+"/register\">"+front_addr+"/register"+"</a></p>\n</body>\n</html>\n";
    }
    string response=header+to_string(page.length())+"\r\n\r\n"+page;
    DoWrite(comm_fd,response.c_str(),response.size());
    RemoveFd(comm_fd);
    close(comm_fd);
}
