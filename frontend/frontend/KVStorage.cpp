#include "KVStorage.h"
#include "dirFileModel.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include "utils.h"
#include "Message.h"
#include "MasterNode.h"
#include <iostream>
#include <cstring>
#include<vector>
extern int BUFF_SIZE;
using namespace std;
MasterNode* masterNode = new MasterNode();
// send one message to backend
string SendToBackend(string message, string username, string backend_addr){
    vector<string> tokens = Split(backend_addr, ':');
/*     cout << "ip : " << tokens.at(0) << endl;
    cout << "port : "<< tokens.at(1) << endl; */

    struct sockaddr_in backend;
    bzero(&backend, sizeof(backend));
    backend.sin_family = AF_INET;
    backend.sin_port = htons(atoi(tokens.at(1).c_str()));
    inet_pton(AF_INET, tokens.at(0).c_str(), &(backend.sin_addr));

    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (backend_fd < 0) {
        cout << "Cannot open socket with backend!\n";
        exit(-1);
    }

    int res = connect(backend_fd, (struct sockaddr *)&backend, sizeof(backend));
    if(res == -1){
        perror("connect,error");
        exit(0);
    }

    if (false){
        char m[message.length() + 1];
        strcpy(m, message.c_str());

        DoWrite(backend_fd, m, strlen(m));
        cout << "Send to Backend: " << message << endl;

        char resp[BUFF_SIZE];
        DoReadWithCRLF(backend_fd, resp);
        string ret = resp;
        cout << "Receive from Backend: " << resp << endl;

        close(backend_fd);  // close the connection after receive msgs from backend

        return resp;
    }else {
        cout << "send msg length:" << message.size() << endl;
        long long count = DoWrite(backend_fd, message, message.size());

        cout << "Send to Backend: [truncated]\n" << message.substr(0,100)<<endl;
        if(message.size() > 100) {
            cout << "....."<< endl << message.substr(message.size()-50,50) <<endl;
        }
        cout <<"send bytes:"<< count<< endl;
        auto resp = DoReadWithCRLF(backend_fd);
        cout << "Receive from Backend: [truncated] \n" << resp.substr(0,50)<<endl;
        if (resp.size() > 50) {
            cout <<"....."<< endl << resp.substr(resp.size()-50,50) <<endl;
        }
        cout << "receive bytes:"<< resp.size() << endl;

        close(backend_fd);  // close the connection after receive msgs from backend

        return resp;
    }

}

KVStorageService::KVStorageService(string username) {
    username_ = username;
    kvAddress_ = masterNode->GetAddress(username);
}

bool KVStorageService::CheckExist() {
    auto msg = Message("GETT",username_,"password"," "," ");
    auto ret = SendToBackend(msg.ParseMsg(),username_,kvAddress_);
    if (ret.substr(0,2) == "OK") {
        string result = ret.substr(5, ret.size() - 5);
        return true;
    }
    else {
        auto errMsg = ret.substr(5, ret.size() - 5);
        return false;
    }
}

bool KVStorageService::ChangePassword(string oldPassword, string newPassword) {
    auto msg = Message("CPUT",username_,"password", oldPassword, newPassword);
    auto ret = SendToBackend(msg.ParseMsg(),username_,kvAddress_);
    if (ret.find("OKCP") != string::npos) {
        return true;
    }
    auto errMsg = ret.substr(5, ret.size() - 5);
    cout << "error in change password for " << username_ << ",message is:" << errMsg << endl;
    return false;
}

bool KVStorageService::CreateUser(string password) {
    if (password.empty()) {
        cout << "password is null" << endl;
        return false;
    }
    auto msg = Message("PUTT",username_,"password",password," ");
    auto ret= SendToBackend(msg.ParseMsg(),username_,kvAddress_);
    cout << "ret" << ret << endl;
    if (ret.substr(0,2) == "OK") {
        return true;
    }
    auto errMsg = ret.substr(5, ret.size() - 5);
    cout << "error in creating account:" << username_ << ",message is:" << errMsg << endl;
    return false;
}

bool KVStorageService::CheckPassword(string password) {
    auto msg = Message("GETT",username_,"password"," "," ");
    auto ret = SendToBackend(msg.ParseMsg(), username_, kvAddress_);
    if (ret.substr(0,2) == "OK") {
        string result = ret.substr(5, ret.size() - 5);
        if (result == password)
            return true;
    }
    else {
        auto errMsg = ret.substr(5, ret.size() - 5);
    }
    return false;
}

string KVStorageService::GetDirInfo(string dir_id, string info_type) {
    string symbol = "";
    string col = dir_id + ":" + info_type;
    //F:1234:parent   
    auto msg = Message("GETT",username_, col," "," ");
    auto ret = SendToBackend(msg.ParseMsg(), username_, kvAddress_);
    if (ret.substr(0,2) == "OK") {
        string result = ret.substr(5, ret.size() - 5);
        return result;
    }
    else {
        auto errMsg = ret.substr(5, ret.size() - 5);
        cout << "error:" <<errMsg <<endl;
    }
    return "";
}

bool KVStorageService::MoveDirOrFile(string fileOrDirId,string newDirId,bool isFile){
    string oldDirId;
    if (isFile) {
        oldDirId = GetFileByFileId(fileOrDirId).ParentId_;
    }else {
        oldDirId = GetDirByDirId(fileOrDirId).ParentId_;
    }
    auto TryCount = 0;
    bool success;
    while(TryCount < 50) {
        auto oldDir = GetDirByDirId(oldDirId);
        auto contentStr = oldDir.DirContent_.ToString();
        if (isFile) {
            oldDir.DirContent_.RemoveFile(fileOrDirId);
        }else {
            oldDir.DirContent_.RemoveDir(fileOrDirId);
        }
        if (contentStr.empty()){
            success = this->SetDirInfo(oldDirId,"content", oldDir.DirContent_.ToString());
        }else{
            success = this->CSetDirInfo(oldDirId,"content",contentStr,oldDir.DirContent_.ToString());
        }
        if (success) {
            break;
        }else{
            TryCount ++;
        }
    }
    if (TryCount == 50) {
        cout << "cput error" <<endl;
        return false;
    }
    TryCount = 0;
    while(TryCount < 50) {
        auto newDir = GetDirByDirId(newDirId);
        auto contentStr = newDir.DirContent_.ToString();
        if (isFile) {
            newDir.DirContent_.AddFile(fileOrDirId);
        }else {
            newDir.DirContent_.AddDir(fileOrDirId);
        }
        if (contentStr.empty()){
            success = this->SetDirInfo(newDirId,"content", newDir.DirContent_.ToString());
        }else{
            success = this->CSetDirInfo(newDirId,"content",contentStr,newDir.DirContent_.ToString());
        }
        if (success) {
            break;
        }else{
            TryCount ++;
        }
    }
    if (TryCount == 50) {
        cout << "cput error" <<endl;
        return false;
    }
    TryCount = 0;
    while (TryCount < 50)
    {
        success = CSetDirInfo(fileOrDirId,"parent",oldDirId,newDirId);
        if (success) {
            break;
        }else{
            TryCount ++;
        }
    }
    if (TryCount == 50) {
        cout << "cput error" <<endl;
        return false;
    }
    return true;
}

bool KVStorageService::CreateFolder(Dir dir){
    auto DirId =  dir.DirId_;
    SetDirInfo(DirId, "content",dir.DirContent_.ToString());
    SetDirInfo(DirId,"parent",dir.ParentId_);
    SetDirInfo(DirId, "name",dir.DirName_);
    auto TryCount = 0;
    while(TryCount < 50) {
        auto contentStr = GetDirInfo(dir.ParentId_, "content");
        auto content = DirContent(contentStr);
        content.AddDir(DirId);
        bool success;
        if (contentStr.empty()){
            success = this->SetDirInfo(dir.ParentId_,"content", content.ToString());
        }else{
            success = this->CSetDirInfo(dir.ParentId_,"content",contentStr,content.ToString());
        }
        if (success) {
            return true;
        }else{
            TryCount ++;
        }
    }
    return false;
}

bool KVStorageService::DeleteFolder(Dir dir){
    auto DirId =  dir.DirId_;
    auto contentStr = GetDirInfo(dir.ParentId_, "content");
    auto content = DirContent(contentStr);
    content.RemoveDir(DirId);
    CSetDirInfo(dir.ParentId_,"content", contentStr, content.ToString());
    CleanDirInfo(DirId, "content");
    CleanDirInfo(DirId,"parent");
    CleanDirInfo(DirId, "name");
}

bool KVStorageService::DeleteFile(File file){
    auto FileId =  file.FileId_;
    auto contentStr = GetDirInfo(file.ParentId_, "content");
    auto content = DirContent(contentStr);
    content.RemoveFile(FileId);
    CSetDirInfo(file.ParentId_,"content", contentStr, content.ToString());
    auto maxChunkNumberStr = GetDirInfo(FileId,"content");
    if (!maxChunkNumberStr.empty()) {
        auto maxChunkNumber = stoi(maxChunkNumberStr);
        for (size_t i = 1; i <= maxChunkNumber; i++)
        {
            auto chunk_id = FileId + "$" + to_string(i);
            auto msg = Message("DELE",username_,chunk_id," "," ");
            SendToBackend(msg.ParseMsg(), username_, kvAddress_);
        }
    }
    CleanDirInfo(FileId, "content");
    CleanDirInfo(FileId,"parent");
    CleanDirInfo(FileId, "name");
    return true;
}

bool KVStorageService::CleanDirInfo(string dir_id, string info_type){
    string col = dir_id + ":" + info_type;  
    //F:1234:parent
    auto msg = Message("DELE",username_, col, " ", " ");
    auto ret = SendToBackend(msg.ParseMsg(), username_, kvAddress_);
    if (ret.substr(0,2) == "OK") {
        return true;
    }
    else {
        auto errMsg = ret.substr(5, ret.size() - 5);
        cout << "error:" <<errMsg <<endl;
    }
    return false;
}

bool KVStorageService::SetDirInfo(string dir_id, string info_type, string val){
    string col = dir_id + ":" + info_type;  
    //F:1234:parent
    auto msg = Message("PUTT",username_, col, val, " ");
    auto ret = SendToBackend(msg.ParseMsg(), username_, kvAddress_);
    if (ret.substr(0,2) == "OK") {
        return true;
    }
    else {
        auto errMsg = ret.substr(5, ret.size() - 5);
        cout << "error:" <<errMsg <<endl;
    }
    return false;

}

Dir KVStorageService::GetDirByDirId(string DirId)
{
    auto dir = Dir();
    auto content = this->GetDirInfo(DirId,"content");
    auto parentId = this->GetDirInfo(DirId, "parent");
    auto dirName = this->GetDirInfo(DirId,"name");
    dir.DirId_ = DirId;
    dir.DirContent_= DirContent(content);
    dir.DirName_ = dirName;
    dir.ParentId_= parentId;
    return dir;
}

File KVStorageService::GetFileByFileId(string FileId){
    auto file = File();
    auto content = this->GetDirInfo(FileId,"content");
    auto parentId = this->GetDirInfo(FileId,"parent");
    auto FileName = this->GetDirInfo(FileId,"name");
    file.FileId_ = FileId;
    file.Content_= content;
    file.Name_ = FileName;
    file.ParentId_= parentId;
    return file;
}

bool KVStorageService::CSetDirInfo(string dir_id, string info_type, string old,string newVal){
    string col = dir_id + ":" + info_type;  
    //F:1234:parent
    auto msg = Message("CPUT",username_, col, old, newVal);
    auto ret = SendToBackend(msg.ParseMsg(), username_, kvAddress_);
    if (ret.substr(0,2) == "OK") {
        return true;
    }
    else {
        auto errMsg = ret.substr(5, ret.size() - 5);
        cout << "error:" <<errMsg <<endl;
    }
    return false;

}

map<string, string> KVStorageService::GetMoveOptions(string dir_id){
    map<string, string> all_dirs;
    auto dir = GetDirByDirId(dir_id);
    if (dir_id != "/") {
        all_dirs.insert(pair<string,string>(dir.ParentId_,".."));
    }
    for (auto &&i : dir.DirContent_.dirIds_)
    {
        auto subDir = GetDirByDirId(i);
        all_dirs.insert(pair<string,string>(subDir.DirId_,subDir.DirName_));
    }
    return all_dirs;
}

map<string, string> KVStorageService::GetAllFiles(string dir_id, string parent){

}

void KVStorageService::SaveFile(File file) {
    cout << "file_name:" << file.Name_ <<endl;
    if (file.Content_.size() <= 200) {
        cout << "file_data:---------" <<endl << file.Content_ << "-----------"<<endl;
    }else {
        cout << "file_data:---------" << endl << "bytes:" << file.Content_.size() << endl <<
        "last letters:\n" << file.Content_.substr(file.Content_.size() - 20) << endl
        << "-----------"<<endl;
    }
    cout << "file_parent id:" << file.ParentId_ <<endl;
    string content = file.Content_;
    vector<string> chunk_id_list;
    const int chunk_size = 1000; 
    int chunk_num = 0;
    while (content.size() > chunk_size){
        chunk_num += 1;
        string chunk_data = content.substr(0, chunk_size);
        string chunk_id = file.FileId_ + "$" + to_string(chunk_num);
        chunk_id_list.push_back(chunk_id);
        auto msg = Message("PUTT", username_, chunk_id, StringToBit(chunk_data), " ");
        auto ret= SendToBackend(msg.ParseMsg(),username_,kvAddress_);
        cout << "chunk no : " << chunk_num << endl;
        content = content.substr(chunk_size);
    }
    if(content.size() <= chunk_size && !content.empty()){
        chunk_num += 1;
        string chunk_id = file.FileId_ + "$" + to_string(chunk_num);
        chunk_id_list.push_back(chunk_id);
        auto msg = Message("PUTT", username_, chunk_id, StringToBit(content), " ");
        auto ret= SendToBackend(msg.ParseMsg(),username_,kvAddress_);
        cout << "chunk no : " << chunk_num << endl;
    }
    file.chunkFileIds_ = chunk_id_list;
    cout << "upload id:" << chunk_id_list.size() << endl;
    cout << "all bytes:" << file.Content_.size() << endl;
    this->CreateFile(file);
}

bool KVStorageService::CreateFile(File file){
    auto FileId = file.FileId_;
    SetDirInfo(FileId, "content",to_string(file.chunkFileIds_.size()));
    SetDirInfo(FileId, "parent",file.ParentId_);
    SetDirInfo(FileId, "name",  file.Name_);
    auto TryCount = 0;
    while(TryCount < 50) {
        auto contentStr = GetDirInfo(file.ParentId_, "content");
        auto content = DirContent(contentStr);
        content.AddFile(FileId);
        bool success;
        if (contentStr.empty()){
            success = this->SetDirInfo(file.ParentId_,"content", content.ToString());
        }else{
            success = this->CSetDirInfo(file.ParentId_,"content",contentStr,content.ToString());
        }
        if (success) {
            return true;
        }else{
            TryCount ++;
        }
    }
    return false;
}


File KVStorageService::GetFile(string file_id) {
    auto maxChunkNumberStr = GetDirInfo(file_id,"content");
    if (maxChunkNumberStr.empty()) {
        cout << "can not find file, file_id:" << file_id << endl;
        return File();
    }
    auto maxChunkNumber = stoi(maxChunkNumberStr);
    auto parentId = GetDirInfo(file_id,"parent");
    auto FileName = GetDirInfo(file_id,"name");
    string content;
    for (size_t i = 1; i <= maxChunkNumber; i++)
    {
        auto chunk_id = file_id + "$" + to_string(i);
        auto msg = Message("GETT",username_,chunk_id," "," ");
        auto res = SendToBackend(msg.ParseMsg(),username_,kvAddress_);
        if (res.size() > 5 && res.substr(0,4) == "OKGT") {
            content += BitToString(res.substr(5));
        }else {
            cout << "error in read:" << res << endl;
            content = "";
            break;
        }
    }
    return File(content,FileName,parentId);
}

bool KVStorageService::SendMail(string target, string data){
    if(target.empty()){
        cout<<"receiver empty"<<endl;
        return false;
    }
    //string
    string maxsofar="0";
    GetMaxID("mail",target,maxsofar);
    string updatedIndex=std::to_string(stoi(maxsofar)+1);
    cout<<"allocated mail id is:"<<endl;
    cout<<updatedIndex<<endl;
    auto msg = Message("CPUT",target,"MaxMailIndex",StringToBit(maxsofar),StringToBit(updatedIndex));
    auto ret = SendToBackend(msg.ParseMsg(), target, kvAddress_);
    cout<<"update max mail index to kv table"<<endl;
    cout<<ret<<endl;
    string col="m"+updatedIndex;
    msg=Message("PUTT",target,col,StringToBit(data)," ");
    ret= SendToBackend(msg.ParseMsg(),target,kvAddress_);
    cout<<"put mail to kv table"<<endl;
    cout<<ret<<endl;
    auto errMsg = GetMidStr(ret,"ERPT ","\r\n");
    if (!errMsg.empty()) {
        cout << "error in sending email:" << username_ << ",message is:" << errMsg << endl;
        return false;
    }
    return true;
}

bool KVStorageService::GetAllMail(std::vector<string>& result){
    string maxsofar="0";
    GetMaxID("mail",username_,maxsofar);
    int maxid=stoi(maxsofar);
    if(maxid==0){
        return false;
    }else{
        for(int i=1;i<=maxid;i++){
            string res="";
            int k=GetMail("m"+std::to_string(i),res);
            
            if(k){
                result.push_back(std::to_string(i)+"#"+res);
                cout<<"mail content is.."<<endl;
                cout<<res<<endl;
            }
        }
    }
    return true;
}

bool KVStorageService::GetMail(string id,string& result){
        auto msg = Message("GETT",username_,id," "," ");
        auto ret = SendToBackend(msg.ParseMsg(), username_, kvAddress_);
        cout<<"get mail from kv table"<<endl;
        cout<<ret<<endl;
        if (ret.substr(0,2) == "OK") {
            result=BitToString(ret.substr(5));
            
            return true;
        }else{
            return false;
        }
}
bool KVStorageService::DeleteMail(string id){
    auto msg = Message("DELE",username_,id," "," ");
    auto ret = SendToBackend(msg.ParseMsg(), username_, kvAddress_);
    cout<<"delete mail from kv table"<<endl;
    cout<<ret<<endl;
    if (ret.substr(0,2) == "OK") {
            return true;
        }else{
            return false;
        }
}

bool KVStorageService::GetMaxID(string cmd,string target, string& result){
    if(cmd=="mail"){
        auto msg = Message("GETT",target,"MaxMailIndex"," "," ");
        auto ret = SendToBackend(msg.ParseMsg(), target, kvAddress_);
        cout<<"get max mail id from kv table"<<endl;
        cout<<ret<<endl;
        if (ret.substr(0,2) == "OK") {
            result=BitToString(ret.substr(5));
            return true;
        }else{
            msg = Message("PUTT",target,"MaxMailIndex",StringToBit("0")," ");
            ret= SendToBackend(msg.ParseMsg(),target,kvAddress_);
            result="0";
        }
    }
        return false;
}


