#pragma once
#include<string>
#include<vector>
#include "utils.h"
using namespace std;
class DirContent
{
private:
    /* data */
public:
    vector<string> fileId_;
    vector<string> dirIds_;
    DirContent();
    DirContent(string);
    void AddFile(string fileId);
    void RemoveFile(string fileId);
    void AddDir(string dirId);
    void RemoveDir(string dirId);
    string ToString();
};

class Dir
{
private:
    /* data */
public:
    string DirId_;
    DirContent DirContent_;
    string DirName_;
    string ParentId_;
    Dir();
    Dir(string DirName,string ParentId_);
    Dir(string DirId);
};

class File
{
public:
    string FileId_;
    string Name_;
    string Content_;
    string ParentId_;
    vector<string> chunkFileIds_;
    File(string Content, string FileName, string ParentId);
    File(string Content, string chunkId);
    File();
};