#ifndef FRONTEND_KVSTORAGE_H
#define FRONTEND_KVSTORAGE_H
#include <string>
#include <vector>
#include <map>
#include "dirFileModel.h"
using namespace std;
using std::string;
class KVStorageService {
private:
    string username_;
    string kvAddress_;
public:
    KVStorageService(string username);
    bool CheckExist();
    bool CreateUser(string password);
    bool CheckPassword(string password);
    bool ChangePassword(string oldPassword, string newPassword);
    bool SendMail(string target, string data);
    bool GetMaxID(string cmd, string target, string& result);
    bool GetAllMail(std::vector<string>& result);
    bool GetMail(string id,string& result);
    bool DeleteMail(string id);
    void SaveFile(File file);
    File GetFile(string file_id);
    string GetDirInfo(string dir_id, string info_type);
    bool CreateFolder(Dir dir);
    bool DeleteFolder(Dir dir);
    bool CreateFile(File file);
    bool DeleteFile(File file);
    bool CleanDirInfo(string dir_id, string info_type);
    bool SetDirInfo(string dir_id, string info_type, string val);
    bool CSetDirInfo(string dir_id, string info_type, string ori,string newVal);
    Dir GetDirByDirId(string DirId);
    File GetFileByFileId(string FileId);
    bool MoveDirOrFile(string fileOrDirId,string newDirId,bool isFile);
    map<string, string> GetMoveOptions(string dir_id);
    map<string, string> GetAllFiles(string dir_id, string parent);
};

string SendToBackend(string message, string username, string backend_addr);


#endif //FRONTEND_KVSTORAGE_H
