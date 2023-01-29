#ifndef FRONTEND_HANDLER_H
#define FRONTEND_HANDLER_H
#include <string>
#include "Request.h"
#include "Response.h"
#include "html/Html.h"
#include "KVStorage.h"
#include "FileBody.h"
#include "dirFileModel.h"
class Handler {
private:
    std::string staticHtmlFile_;

public:
    Handler();
    virtual std::string Process(Request req,Html* html);
    void SetHtmlFile(std::string);
    std::string GetHtmlFile();
};

class StaticPageHandler:public Handler {
public:
    StaticPageHandler();
    std::string Process(Request req,Html* html) override;
};

//----------------------User---------------------------
class RegisterHandler:public Handler {
public:
    RegisterHandler();
    std::string Process(Request req,Html* html) override;
};

class LoginHandler:public Handler {
public:
    LoginHandler();
    std::string Process(Request req,Html* html) override;
};

class ChangePswHandler:public Handler {
public:
    ChangePswHandler();
    std::string Process(Request req,Html* html) override;
};

//----------------------File---------------------------
class ListFileHandler:public Handler {
public:
    ListFileHandler();
    std::string Process(Request req,Html* html) override;
};

class MoveFileHandler:public Handler {
public:
    MoveFileHandler();
    std::string Process(Request req,Html* html) override;
};

class UploadHandler:public Handler {
public:
    UploadHandler();
    std::string Process(Request req,Html* html) override;
};

class DownloadHandler:public Handler {
public:
    DownloadHandler();
    std::string Process(Request req,Html* html) override;
};

class CreateHandler:public Handler {
public:
    CreateHandler();
    std::string Process(Request req,Html* html) override;
};

class DeleteHandler:public Handler {
public:
    DeleteHandler();
    std::string Process(Request req,Html* html) override;
};

class RenameHandler:public Handler {
public:
    RenameHandler();
    std::string Process(Request req,Html* html) override;
};

class MoveHandler:public Handler {
public:
    MoveHandler();
    std::string Process(Request req,Html* html) override;
};

//----------------------Mail---------------------------
class SendMailHandler:public Handler {
public:
    SendMailHandler();
    std::string Process(Request req,Html* html) override;
};

class AllMailHandler:public Handler {
public:
    AllMailHandler();
    std::string Process(Request req,Html* html) override;
};

class ViewMailHandler:public Handler {
public:
    ViewMailHandler();
    std::string Process(Request req,Html* html) override;
};

class DeleteMailHandler:public Handler {
public:
    DeleteMailHandler();
    std::string Process(Request req,Html* html) override;
};

class ReplyMailHandler:public Handler {
public:
    ReplyMailHandler();
    std::string Process(Request req,Html* html) override;
};

class ForwardMailHandler:public Handler {
public:
    ForwardMailHandler();
    std::string Process(Request req,Html* html) override;
};

#endif //FRONTEND_HANDLER_H
