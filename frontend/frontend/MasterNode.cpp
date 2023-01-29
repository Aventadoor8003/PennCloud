#include "MasterNode.h"
#include "utils.h"
extern int BUFF_SIZE;

using namespace std;
MasterNode::MasterNode() {
    struct sockaddr_in dest;				    // define server address
    bzero(&dest, sizeof(dest));	                // clean
    dest.sin_family = AF_INET;				    // address family
    dest.sin_port = htons(MASTER_PORT);
    inet_pton(AF_INET, MASTER_IP.c_str(), &(dest.sin_addr));

    int master_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (master_fd < 0) {
        cout << "Cannot open socket with master!\n";
        exit(-1);
    }
    int res = connect(master_fd,(struct sockaddr *)&dest,sizeof(struct sockaddr));
    if(res == -1){
        perror("connect, error");
        exit(0);
    }
    masterFd_ = master_fd;
}

string MasterNode::GetAddress(string userName) {
    string to_master_msg = "ASTA " + userName + " \r\n";
    cout << "Send to MasterNode: " << to_master_msg << endl;
    write(masterFd_, to_master_msg.c_str(), to_master_msg.size());
    char ans[BUFF_SIZE];
    DoReadWithCRLF(masterFd_, ans);
    cout << "Receive from MasterNode: " << string(ans) << endl;
    string s = ans;
    cout << "Parse addr is :" << s.substr(5, s.size() - 5) << "\r\n" << endl;
    return s.substr(5, s.size() - 5);
}
