#include <stdlib.h>
#include <stdio.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<string.h>
#include<iostream>
#include <cstring>
#include<cerrno>
#include<signal.h>
#include<fcntl.h>
#include<filesystem>
#include<regex>
#include<unordered_map>
#include<fstream>
#include<ctime>
#include <sys/file.h>
#include "KVStorage.h"
#include "global_vars.h"
#include "Http.h"
#include "utils.h"
//define max concurrent connections to be 100
#define N 100

//int array to store thread index assigned
int thdIdx[N];
int length=0;
pthread_mutex_t mutex1;
pthread_cond_t cond;
// std::string rootdir;
//flag for debug output
int vFlag=0;
volatile int fd_array[N]{0};
volatile int shuttingdown=0;
int listen_fd;
// std::unordered_map<std::string,int> map={};
// pthread_mutex_t locks[100];
//args sent to each thread
struct clientInfo{
  /* data */
  int comm_fd;
  int threadId;

};

struct emailInfo{
  std::string sender;
  std::vector<std::string> receiver;
  std::string data;
  
};

/*
Signal Handler:
catch SIGINT signal
set shuttingdown flag to be 1
close all comm_fd that is in use
close listen_fd
clean up resources
exit process
*/
void signalHandler(int arg){
  if(arg==SIGINT){
    shuttingdown=1;
    for(int i=0;i<N;i++){
      int fd_to_close=fd_array[i];
      if(fd_to_close!=0){
        //set fd_to_close to be non-blocking
        int opts=fcntl(fd_to_close,F_GETFL);
        if(opts<0){
          fprintf(stderr,"F_GETFL failed (%s)\n",strerror(errno));
          abort();
        }
        opts=(opts|O_NONBLOCK);
        if(fcntl(fd_to_close,F_SETFL,opts)<0){
          fprintf(stderr,"F_SETFL failed (%s)\n",strerror(errno));
          abort();
        }
        //write goodby message
        std::string bye="421 <localhost> Service not available\r\n";
        write(fd_to_close,bye.c_str(),strlen(bye.c_str()));
        if(vFlag==1){
              fprintf(stderr,"[%d] S:%s\n",fd_to_close,bye.c_str());
        }
        //close connection
        close(fd_to_close);
        
      }
    }
    close(listen_fd);
    pthread_mutex_destroy(&mutex1);
    pthread_cond_destroy(&cond);
    // for(int i=0;i<map.size();i++){
    //   pthread_mutex_destroy(&locks[i]);
    // }
    exit(0);
  }
}

//thread working function
/*
take connection from the main thread
read client message and reply
*/
void* thread_main(void* arg){
  clientInfo* client=(clientInfo*)arg;
  int comm_fd=client->comm_fd;
  int idx=client->threadId;
  emailInfo* email=new emailInfo;
  int boxId;
  //if shuttingdown flag is on, close the fd immediately
  //otherwise store fd in the fdarray
  if(shuttingdown==0){
    fd_array[idx]=comm_fd;
  }else{
    //shutting down
    //close comm_fd
    close(comm_fd);
    //delete client
    delete client;
    pthread_exit(nullptr);
    return nullptr;
  }
    char clientBuffer[1024];
    char command[1024];
    int tail=0;
    int flag=1;
    int data_mode=0;
    int helloed=0;
    //read client commands
    while(1){
      int n = read(comm_fd,&clientBuffer[tail],1023);
      if(n<=0) flag=0;
      char* ptr=&clientBuffer[tail];
      int idx=-1;
      int prev=0;
      //iterate each char in n chars read
      for(int i=0;i<n;i++){
        //check every \r\n terminate command
        if(*(ptr+i)=='\n'&&*(ptr+i-1)=='\r'){
            idx=i;
            //for later prev need to add tail but for first no need to add tail
            
            if(prev==0){
            std::memcpy(command,&clientBuffer[prev],tail+idx+1);
            
            prev=tail+idx+1;
            command[tail+idx+1]='\0';
            }else{
              std::memcpy(command,&clientBuffer[prev],idx-prev+1);
              command[idx-prev+1]='\0';
              prev=idx+1;
              
            }
            
            std::string str_command(command);
            std::cout<<str_command<<std::endl;
            if(vFlag==1){
                fprintf(stderr,"[%d] C:%s\n",comm_fd,str_command.c_str());
                
            }
            // std::cout<<"==|"<<std::endl;
            // std::cout<<str_command<<std::endl;
            // std::cout<<"|=="<<std::endl;
            std::string response;
            //in data mode, no need to write response,check for end dot
            if(data_mode==1){
              if(str_command==".\r\n"){
                //write email->data to file
                
                for(auto address:email->receiver){
                  // auto search=map.find(address.substr(0,address.find("@")));
                  // if(search==map.end()){
                  //   continue;
                  // }
                  
                   
                  std::string mb_name=address.substr(0,address.find("@"));
                  // std::string mailbox=rootdir+"/"+address.substr(0,address.find("@"))+".mbox";
                  std::time_t result = std::time(nullptr);
                  std::string format="From <"+email->sender+"> "+std::asctime(std::localtime(&result))+"\r\n"+email->data;
                  // mb_name is target username and format is the content to send
                  auto kvService = new KVStorageService(mb_name);
                  if(kvService->CheckExist()){
                    bool success = kvService->SendMail(mb_name,format);
                  }
                  delete kvService;
                  // int buffersize=format.length();
                  // char* writebuffer=new char[buffersize+1];
                  // std::strcpy(writebuffer,format.c_str());

                  // boxId=map[mb_name];
                  // pthread_mutex_lock(&locks[boxId]);
                  
                  
        
                  // int fd_box=open(mailbox.c_str(),O_RDWR|O_APPEND);
                  // flock(fd_box, LOCK_EX);
                  // int ret=write(fd_box,writebuffer,buffersize);
                  // flock(fd_box, LOCK_UN);
                  // close(fd_box);

                  // pthread_mutex_unlock(&locks[boxId]);
                  // delete[] writebuffer;
                }
                // std::string mailboxname=email->
                //end data_mode
                data_mode=0;
                //clean email
                bzero(email,sizeof(emailInfo));
                response="250 OK\r\n";
                write(comm_fd,response.c_str(),strlen(response.c_str()));
              }else{
                //write str_command to buffer
                email->data+=str_command;
                
              }
            }else{
              //command mode
        //MAIL FROM      
              if(std::regex_match (str_command, std::regex("(MAIL FROM:)(.*)(\r\n)", std::regex::ECMAScript | std::regex::icase))){
                  if(helloed==0){
                    response="503 Bad sequence of commands\r\n";
                  }else if (std::regex_match (str_command.substr(10), std::regex("<[a-zA-Z0-9\\.]+@[a-zA-Z0-9\\.]+>(\r\n)") )){
                    //if receiver or data is not empty, 503
                    response="250 OK\r\n";
                    std::string from_address(str_command.substr(11,str_command.find(">")-11));
                    email->sender=from_address;
                  }else{
                    response="501 Syntax error in parameters or arguments\r\n";
                  }

        //RCPT TO          
                }else if(std::regex_match (str_command, std::regex("(RCPT TO:)(.*)(\r\n)", std::regex::ECMAScript | std::regex::icase))){
                  if(helloed==0||email->sender.length()==0){
                    response="503 Bad sequence of commands\r\n";
                  }else if (std::regex_match (str_command.substr(8), std::regex("<[a-zA-Z0-9]+@t16>(\r\n)") )){
                    //to_address is the mail address that this email is sent to
                    std::string to_address(str_command.substr(9,str_command.find(">")-9));
                    //check user is in the system or not
                    
                    
                    response="250 OK\r\n";
                    email->receiver.push_back(to_address);
                    
                    
                  }else if(std::regex_match (str_command.substr(8), std::regex("[a-zA-Z0-9]+(@t16)(\r\n)"))){
                    response="501 Syntax error in parameters or arguments\r\n";
                  }else{
                    response="550 Requested action not taken: mailbox unavailable\r\n";
                  }

          //HELO        
                }else if(std::regex_match (str_command, std::regex("(HELO )(.*)(\r\n)", std::regex::ECMAScript | std::regex::icase))){
                  if (std::regex_match (str_command, std::regex("(HELO )(.+)(\r\n)",std::regex::ECMAScript | std::regex::icase) )){
                    //set initial state, check initial if not 503
                    response="250 localhost\r\n";
                    if(helloed==0){
                      bzero(email,sizeof(emailInfo));
                      helloed=1;
                    }
                  }else{
                    response="501 Syntax error in parameters or arguments\r\n";
                  }

         //DATA         
                }else if(std::regex_match (str_command, std::regex("DATA\r\n", std::regex::ECMAScript | std::regex::icase))){
                  if(helloed==0||email->sender.length()==0||email->receiver.size()==0){
                    response="503 Bad sequence of commands\r\n";
                  }else{
                    response="354 Start mail input; end with <CRLF>.<CRLF>\r\n";
                    data_mode=1;
                  }
        //QUIT
                }else if(std::regex_match (str_command, std::regex("(QUIT )(.*)(\r\n)", std::regex::ECMAScript | std::regex::icase))|std::regex_match(str_command, std::regex("(QUIT\r\n)",std::regex::ECMAScript | std::regex::icase))){
                    if(helloed==0){
                    response="503 Bad sequence of commands\r\n";
                  }else{
                    flag=0;
                  }
         //RSET           
                }else if(std::regex_match(str_command, std::regex("RSET( *)\r\n",std::regex::ECMAScript | std::regex::icase))){
                  //if before initial, 503
                  if(helloed==0){
                    response="503 Bad sequence of commands\r\n";
                  }else{
                    response="250 OK\r\n";
                    bzero(email,sizeof(emailInfo));
                  }
        //NOOP
                }else if(std::regex_match(str_command, std::regex("NOOP( *)\r\n",std::regex::ECMAScript | std::regex::icase))){
                  //if not initial, 503
                  if(helloed==0){
                    response="503 Bad sequence of commands\r\n";
                  }else{
                    response="250 OK\r\n";
                  }
                }else{
                  response="500 Syntax error, command unrecognized\r\n";
                }
              if(vFlag==1){
                fprintf(stderr,"[%d] S:%s\n",comm_fd,response.c_str());
              }
              write(comm_fd,response.c_str(),strlen(response.c_str()));
            }
        }
        //sign for quit
        if(flag==0){
          break;
        }
      }
      if(flag==0){
        std::string bye="221 localhost Service closing transmission channel\r\n";
        write(comm_fd,bye.c_str(),strlen(bye.c_str()));
        if(vFlag==1){
              fprintf(stderr,"[%d] S:%s\n",comm_fd,bye.c_str());
        }
        break;
      }
      //no \r\n detected,just add n chars to the buffer
      if(idx==-1){
        tail+=n;
      }else{
        //copy the rest in the buffer to the beginning
        std::memcpy(clientBuffer,&clientBuffer[tail+idx+1],n-idx-1);
        tail=n-idx-1;
      }
      
    }
    // printf("disconnecting... from %s\n",inet_ntoa(client.clientaddr.sin_addr));
    if(vFlag==1){
        fprintf(stderr,"[%d] Connection closed\n",comm_fd);
    }
    close(comm_fd);
    //produce free idx
    fd_array[idx]=0;
    pthread_mutex_lock(&mutex1);
    thdIdx[length++]=idx;
    pthread_cond_signal(&cond);
    pthread_mutex_unlock(&mutex1);


    delete client;
    delete email;
    pthread_exit(nullptr);
    return nullptr;
}

int main(int argc, char *argv[])
{
  /* Your code here */
  std::cout<<"start of the program"<<endl;
  //PF_INET for internet
  //SOCK_STREAM for stream socket
  //0for default protocol
  pthread_mutex_init(&mutex1,0);
  pthread_cond_init(&cond,0);

  signal(SIGINT,signalHandler);
  int port_num=2500;
  // int n;
  // char* port=nullptr;
  
  
  // int pFlag=0;
  // int aFlag=0;
  // while((n=getopt(argc,argv,"vap:"))!=-1){
  //   switch(n){
  //     case 'v':
  //       vFlag=1;
  //       break;
  //     case 'a':
  //       aFlag=1;
  //       break;
  //     case 'p':
  //       pFlag=1;
  //       port=optarg;
  //       break;
  //     case '?':
  //     if(optopt=='p'){
  //       fprintf(stderr,"option -%c requires an argument.\n",optopt);
  //       return 1;
  //     }else if(isprint(optopt)){
  //       fprintf(stderr,"Unknown option `-%c'.\n",optopt);
  //       return 1;
  //     }else{
  //       fprintf(stderr,"Unknown option character `\\x%x'.\n",optopt);
  //       return 1;
  //     }
  //     default:
  //       abort();
  //   }
  // }
  // if(aFlag==1){
  //   fprintf(stderr,"Yimin Sheng: yimsheng\n");
  //   return 0;
  // }
  // if(pFlag==0){
  //   port_num=2500;
  // }else{
  //   port_num=atoi(port);
  // }
  
  // std::cout << "Current path is " << std::filesystem::current_path() << '\n';
  //   std::cout << "Absolute path for " << path << " is " 
  //             << std::filesystem::absolute(path) << '\n';
  // int user_count=0;
  
  // if(std::filesystem::is_directory(rootdir)){
  //   for(const auto& entry: std::filesystem::directory_iterator(rootdir)){
  //     std::string filename(entry.path().stem());
  //     map[filename]=user_count++;
  //   }
  // }else{
  //   fprintf(stderr,"Invalid directory path");
  //   return 1;
  // }

  // for(int i=0;i<user_count;i++){
  //   pthread_mutex_init(&locks[i],0);
  // }

  listen_fd=socket(PF_INET,SOCK_STREAM,0);
  if(listen_fd<0){
    fprintf(stderr,"Fail to listen: %s\n",strerror(errno));
    exit(1);
  }
  int opt=1;
  int retListen=setsockopt(listen_fd,SOL_SOCKET,SO_REUSEADDR|SO_REUSEPORT,&opt,sizeof(opt));
  if(retListen<0){
    fprintf(stderr,"Fail to reuse port: %s\n",strerror(errno));
    exit(1);
  }

  //prepare available thread index array
  for(int i=0;i<N;i++){
    thdIdx[length++]=i;
  }

  struct sockaddr_in servaddr;
  //erase the given memory area with \0
  bzero(&servaddr,sizeof(servaddr));
  servaddr.sin_family=AF_INET;
  //program doesn't care with adres is used
  servaddr.sin_addr.s_addr=htons(INADDR_ANY);
  servaddr.sin_port=htons(port_num);
  int ret=bind(listen_fd,(struct sockaddr*)&servaddr,sizeof(servaddr));
  if(ret<0){
    fprintf(stderr,"Fail to bind: %s\n",strerror(errno));
    exit(1);
  }
  listen(listen_fd,100);

  pthread_t thds[N];
  int threadIdx=0;
  while(true){
    //get available threadIdx
    pthread_mutex_lock(&mutex1);
    //no available threadIdx,wait until signal
    while(length==0){
      pthread_cond_wait(&cond,&mutex1);
    }
    threadIdx=thdIdx[--length];
    pthread_mutex_unlock(&mutex1);

    struct  sockaddr_in clientaddr;
    socklen_t clientaddrlen=sizeof(clientaddr);
    int comm_fd=accept(listen_fd,(struct sockaddr*)&clientaddr,&clientaddrlen);

    //send greetings
    std::string greeting="220 localhost server Ready\r\n";
    write(comm_fd,greeting.c_str(),strlen(greeting.c_str()));
    // printf("connection from %s\n",inet_ntoa(clientaddr.sin_addr));
    if(vFlag==1){
      fprintf(stderr,"[%d] New connection\n",comm_fd);
    }
    //pack parameters for each thread
    clientInfo* c=new clientInfo;
    bzero(c,sizeof(clientInfo));
    c->comm_fd=comm_fd;
    c->threadId=threadIdx;
    
    //create a new thread to handle
    pthread_create(&thds[threadIdx],nullptr,&thread_main,(void*)c);
    pthread_detach(thds[threadIdx]);
    
  }
  close(listen_fd);
  pthread_mutex_destroy(&mutex1);
  pthread_cond_destroy(&cond);
  // for(int i=0;i<user_count;i++){
  //   pthread_mutex_destroy(&locks[i]);
  // }
  return 0;
}
