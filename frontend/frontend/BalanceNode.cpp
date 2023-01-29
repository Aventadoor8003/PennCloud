#include "BalanceNode.h"
using namespace std;
extern string MASTER_IP;
extern unsigned short MASTER_PORT;
BalanceNode::BalanceNode() {
    struct sockaddr_in dest;				    // define server address
    bzero(&dest, sizeof(dest));	                // clean
    dest.sin_family = AF_INET;				    // address family
    dest.sin_port = htons(BalancePORT);
    inet_pton(AF_INET, BalanceIP.c_str(), &(dest.sin_addr));

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
    Fd_ = master_fd;
}

int BalanceNode::Send() {
    string to_master_msg = "ASTA\r\n";
    cout << "start write" << endl;
    int ret = write(Fd_, to_master_msg.c_str(), to_master_msg.size());
    cout << "Send to Balance: " << to_master_msg <<"///" << "count:" << ret <<endl;
    char ans[10000];
    DoRead(Fd_, ans);
    cout << "Receive from balance: " << string(ans) << endl;
    return ret;
}
