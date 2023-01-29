#include "dirFileModel.h"
DirContent::DirContent(){

}

DirContent::DirContent(string str)
{
    if (str.empty() ||str == " ") {
        return;
    }
    auto pos = str.find("#");
    auto filesStr = str.substr(0,pos);
    for (auto &&i : Split(filesStr,','))
    {
        fileId_.push_back(i);
    }
    auto dirStr = str.substr(pos+string("#").size());
    for (auto &&i : Split(dirStr,','))
    {
        dirIds_.push_back(i);
    }
}

void DirContent::AddFile(string fileId){
    fileId_.push_back(fileId);
}

void DirContent::RemoveFile(string fileId){
    for (vector<string>::iterator i= fileId_.begin(); i!= fileId_.end(); i++)
    {
        if (*i == fileId) {
            fileId_.erase(i);
            break;
        }
    }
}

void DirContent::AddDir(string dirId){
    dirIds_.push_back(dirId);
}

void DirContent::RemoveDir(string dirId){
    for (vector<string>::iterator i= dirIds_.begin(); i!= dirIds_.end(); i++)
    {
        if (*i == dirId) {
            dirIds_.erase(i);
            break;
        }
    }
}

string DirContent::ToString(){
    string fileStr,dirStr;
    for (size_t i = 0; i < fileId_.size(); i++)
    {
        if (i != 0) {
            fileStr += ",";
        }
        fileStr += fileId_[i];
    }
    for (size_t i = 0; i < dirIds_.size(); i++)
    {
        if (i != 0) {
            dirStr += ",";
        }
        dirStr += dirIds_[i];
    }
     
    return fileStr + "#" + dirStr;
}


Dir::Dir(){};

Dir::Dir(string DirName,string ParentId){
    DirName_ = DirName;
    DirId_ = "D:" + CreateFileHash(DirName, ParentId);
    ParentId_ =  ParentId;
}

Dir::Dir(string DirId){
    DirId_ = DirId;
}

File::File(){};


// used for create file
File::File(string Content, string FileName, string ParentId){
    Content_ = Content;
    Name_ = FileName;
    ParentId_ = ParentId;
    FileId_ =  "F:" + CreateFileHash(FileName, ParentId);
}

// used for create chunck
File::File(string Content, string chunkId){
    Content_ = Content;
    FileId_ = chunkId;
}
