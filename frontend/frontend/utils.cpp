#include "utils.h"
#include <sstream>
#include <sstream>
#include <iostream>
#include <fstream>
#include <cstddef>
#include <cstring>
#include <bitset>
#include <algorithm>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <ctime>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <resolv.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fstream>
using namespace std;

extern int BUFF_SIZE;

void error_handler(char const *msg)
{
    fprintf(stderr, "%s\n", msg);
}

vector<string> Split(const string &s, char delim)
{
  vector<string> elems;
  stringstream ss;
  ss.str(s);
  string item;
  while (getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

std::string ReadBody(int conn_fd,int length) {
    std::string ret;
    char tmp;
    for(int i =0;i < length;i++) {
        int flag = read(conn_fd, &tmp, 1);
        if(flag < 0) {
            fprintf(stderr, "Read failed.\n");
            return "";
        }

        if(flag == 0) {
            printf("Read finished.\n");
            break;
        }
        ret += tmp;
    }
    return ret;
}

bool DoReadWithCRLF(int conn_fd, char buffer[]) {
    char tmp;
    int i = 0;
    bool got_cr = false;
    for(; i < BUFF_SIZE; i++) {
        int flag = read(conn_fd, &tmp, 1);
        if(flag < 0) {
            fprintf(stderr, "Read failed.\n");
            return false;
        }

        if(flag == 0) {
            printf("Read finished.\n");
            break;
        }
        if (tmp == '\n' && got_cr) {
            break;
        }else if (got_cr) {
            buffer[i-1] = '\r';
        }

        if (tmp == '\r') {
            got_cr = true;
            continue;
        }else {
            got_cr = false;
        }
        buffer[i] = tmp;
    }

    buffer[i - 1] = '\0';
    // cout << "buff is :" << buffer << endl;
    return true;
}

string DoReadWithCRLF(int conn_fd) {
    char tmp;
    int i = 0;
    bool got_cr = false;
    string ret;
    for(; true; i++) {
        int flag = read(conn_fd, &tmp, 1);
        if(flag < 0) {
            fprintf(stderr, "Read failed.\n");
            return ret;
        }

        if(flag == 0) {
            printf("Read finished.\n");
            break;
        }
        if (tmp == '\n' && got_cr) {
            break;
        }else if (got_cr) {
            ret[i-1] = '\r';
        }

        if (tmp == '\r') {
            got_cr = true;
            continue;
        }else {
            got_cr = false;
        }
        ret += tmp;
    }

    ret[i - 1] = '\0';
    // cout << "buff is :" << buffer << endl;
    return ret;
}

bool DoRead(int conn_fd, char buffer[]) {
    char tmp;
    int i = 0;
    bool got_cr = false;
    bool gotCRLF = false;
    for(; i < BUFF_SIZE; i++) {
        int flag = read(conn_fd, &tmp, 1);
        if(flag < 0) {
            fprintf(stderr, "Read failed.\n");
            return false;
        }
        
        if(flag == 0) {
            break;
        }
        if (!gotCRLF) {
            if (tmp == '\n' && got_cr) {
                gotCRLF = true;
            }
            if (tmp == '\r') {
                got_cr = true;
            }else {
                got_cr = false;
            }
            buffer[i] = tmp;
        }else {
            if (tmp == '\n' && got_cr) {
                break;
            }
            if(tmp != '\n' && got_cr) {
                buffer[i-1] = '\r';
                buffer[i] = tmp;
                gotCRLF = false;
            }
            if (tmp == '\r') {
                got_cr = true;
            }else {
                got_cr = false;
                gotCRLF = false;
                buffer[i] = tmp;
            }
        }
    }

    buffer[i + 1] = '\0';

    return true;
}

bool DoWrite(int conn_fd, const char* buffer, int len) {
    int sent = 0;
    while(sent < len) {
        int n = write(conn_fd, &buffer[sent], len - sent);
        if(n < 0) {
            fprintf(stderr, "Failed to send a message from server to client\n");
            return false;
        }
        sent += n;
    }
    return true;
}

long long int DoWrite(int conn_fd, string buffer, int len) {
    long long sent = 0;
    while(sent < len) {
        int n = write(conn_fd, &buffer[sent], len - sent);
        if(n < 0) {
            fprintf(stderr, "Failed to send a message from server to client\n");
            return false;
        }
        sent += n;
    }
    return sent;
}


string GetMidStr(string src,string left,string right) {
    auto pos1 = src.find(left);
    if (pos1 == string::npos) {
        return {};
    }
    string subSrc = src.substr(pos1);
    auto pos2 = subSrc.find(right);
    if (pos2 == string::npos) {
        return {};
    }
    auto length = pos2 - pos1 - left.size();
    return src.substr(pos1+left.size(),length);
}

int TrimNumber(string num) {

    while(num.find(" ") == 0) {
        num = num.substr(1);
    }
    reverse(num.begin(), num.end());
    while(num.find(" ") == 0) {
        num = num.substr(1);
    }
    reverse(num.begin(), num.end());
    return stoi(num);
}

std::string GetQuery(std::string src, std::string key) {
    auto res = Split(src,'&');
    for (auto val: res) {
        auto splits = Split(val,'=');
        if (splits.size() != 2) {
            continue;
        }
        auto k = splits[0];
        auto v = splits[1];
        if (k == key) {
            return v;
        }
    }
    return "";
}

std::string StringToBit(std::string s){
    std::byte bytes[s.length()];
    std::memcpy(bytes, s.data(), s.length());
    string temp;
    for (byte b: bytes) {
        temp+=std::bitset<8>(std::to_integer<int>(b)).to_string();
    
    }
    return temp;

}
std::string BitToString(std::string bitString){
    string res;
    for(int i=0;i<bitString.length();i+=8){
        std::bitset<8> b(bitString.substr(i,8));       
        unsigned char c = ( b.to_ulong() & 0xFF);
        res+=static_cast<char>(c); 
    }
    return res;
}
inline string change(char c)
{
    string data;
    for(int i=0;i<8;i++)
    {
        //  data+=c&(0x01<<i);
        if ( (( c >>(i-1) ) & 0x01) == 1 )
        {
            data+="1";
        }
        else
        {
            data+="0";
        }
    }
    for(int a=1;a<5;a++)
    {
        char x=data[a];
        data[a]=data[8-a];
        data[8-a]=x;
    }
    return data;
}

std::string Bytes(std::string str) {
    string ret;
    for (char c: str) {
        ret += change(c);
    }
    return ret;
}

inline string change1(string data)
{
    string result;
    char c='\0';
    for(int i=0;i<8;i++)
    {
        if(data[i]=='1') c=(c<<1)|1;
        else c=c<<1;
    }
    // cout<<c;
    result+=(unsigned char)c;
    return  result;
}

std::string Stringify(std::string str) {
    string ret;
    auto count = 0;
    string input;
    for (char v: str) {
        if (count < 8) {
            input += v;
            count ++;
        }
        if (count == 8) {
            count = 0;
            ret += change1(input);
            input.clear();
        }
    }
    return ret;
}

std::string TrimSpace(std::string str) {
    string ret;
    for (auto c: str) {
        if (c == ' '|| c == '\r' || c == '\n') {
            continue;
        }
        ret += c;
    }
    return ret;
}

std::string DecodeURL(std::string content){
	std::string res;
	char c;
	int i,ii;
	for(int i=0;i<content.length();i++){
		if(content[i]=='%'){
			sscanf(content.substr(i+1,2).c_str(),"%x",&ii);
			c=static_cast<char>(ii);
			res+=c;
			i+=2;
		}else if(content[i]=='+'){
			res+=" ";
		
		}else{
			res+=content[i];
		}
	}
	return res;
}

std::string escapeHTML(const string &from) {
  string ret = from;
  ReplaceAll(ret, "&", "&amp;");
  ReplaceAll(ret, "\"", "&quot;");
  ReplaceAll(ret, ">", "&gt;");
  ReplaceAll(ret, "<", "&lt;");
  ReplaceAll(ret, "\'", "&apos;");
  ReplaceAll(ret, "\r\n", "<br>");
  ReplaceAll(ret, "\n", "<br>");
  return ret;
}


std::string ReplaceAll(std::string& str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); 
    }
    return str;
}
std::string htmlToString(const char* filename){
    ifstream file(filename);
    std::string str((istreambuf_iterator<char>(file)), istreambuf_iterator<char>());
    return str;
}

vector<vector<string> > ParseDirContents(string dir_contents){
    vector<vector<string>> contents;
    if (dir_contents == " ") {
        contents.push_back(vector<string>{});
        contents.push_back(vector<string>{});
        return contents;
    }
    vector<string> directories;
    vector<string> files;
    size_t next_delim = dir_contents.find(","), prev_delim = -1;

    do {
        size_t length = next_delim - prev_delim - 1;
        string item = dir_contents.substr(prev_delim + 1, length);
        size_t type_delim = item.find(":");
        string item_type = item.substr(0, type_delim);
        if (item_type == "D") {
            directories.push_back(item.substr(type_delim + 1));
        } else {
            files.push_back(item.substr(type_delim + 1));
        }
        prev_delim = next_delim;
        next_delim = dir_contents.find(",", prev_delim + 1);
    } while (prev_delim != string::npos);

    contents.push_back(directories);
    contents.push_back(files);
    return contents;
}

std::string ReadHTML(std::string file_path){
    int html_fd = open(file_path.c_str(), O_RDONLY);
	string http_body = "";
	char buffer[4096];
	int bytes_read = read(html_fd, buffer, 4095);
	while (bytes_read != 0) {
		buffer[bytes_read] = '\0';
		http_body.append(buffer);
		bytes_read = read(html_fd, buffer, 4095);
	}
	return http_body;
}

// create uuid for folder or file
string CreateFileHash(std::string filename, std::string parent_id){
    time_t now = time(0);
    string key = filename + parent_id + to_string(now);
    hash<string> hash_table;
    size_t hash_value = hash_table(key);
    string hash_str = to_string(hash_value);
    return filename + "~" + hash_str;
}

/* string GetId(std::string params){
    auto res = Split(params,'&');
    for (auto &&i : res)
    {
        auto Sp = Split(i,'=');
        if (Sp[0] == "id") {
            if(Sp.size() > 1) {
                return Sp[1];
            }else {
                return "";
            }
        }
    }
    return "";
} */

string VectorToString(vector<string> list){
    string res;
    for (auto item : list)
    {
        res += item;
        res += ",";
    }
    return res;
}

std::string dns_query(std::string server_name){
    unsigned char hostBuffer[10240];
    int r=res_query(server_name.c_str(),C_IN,T_MX,hostBuffer,10240);
    HEADER* hdr=reinterpret_cast<HEADER*>(hostBuffer);
    int question = ntohs(hdr->qdcount);
    int answers = ntohs(hdr->ancount);
    int nameservers = ntohs(hdr->nscount);
    int addrrecords = ntohs(hdr->arcount);
    char toprint[4096];
    ns_msg m;
    int k=ns_initparse(hostBuffer,r,&m);
    int min_priority = INT_MAX;
    string domain;
    for(int i=0;i<question;++i){
        ns_rr rr;
        ns_parserr(&m, ns_s_an, i, &rr);
        ns_sprintrr(&m,&rr,NULL,NULL,toprint,sizeof(toprint));
        struct in_addr in;
        
        cout<<"MX records are>>>>>>>>>>>>"<<endl;
        cout<<toprint<<endl;

        const u_char* rdata = ns_rr_rdata(rr);
        const uint16_t priority = ns_get16(rdata);
        char exchange[NS_MAXDNAME];
        int len = dn_expand(hostBuffer, hostBuffer + r, rdata + 2, exchange, sizeof(exchange));
        if (len < 0) {
            perror("dn expand failed");
            continue;
        }
        cout << "S: SMTP client got mx record " << exchange << " with priority " << priority << endl;
        if (priority < min_priority) {
            min_priority = priority;
            domain = exchange;
        }
    }
    struct hostent* host=gethostbyname(domain.c_str());
    char ip[16];
    struct sockaddr_in sock_addr;
    sock_addr.sin_addr = *((struct in_addr*) host->h_addr_list[0]);
    inet_ntop(AF_INET, &sock_addr.sin_addr, ip, sizeof(ip));
    string ret(ip);
    return ret;
}