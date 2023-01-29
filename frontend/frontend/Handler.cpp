#include "Handler.h"
#include "dirFileModel.h"
#include "Http.h"
#include "KVStorage.h"
#include "Message.h"
#include "iostream"
#include "utils.h"
#include<ctime>

using namespace std;

Handler::Handler() {

}

void Handler::SetHtmlFile(string str) {
    this->staticHtmlFile_ = str;
}

string Handler::GetHtmlFile(){
    return staticHtmlFile_;
}


string Handler::Process(Request req, Html *html) {
    Response resp(req.GetVersion(),html->GetRawData());
    return resp.ToString();
}

StaticPageHandler::StaticPageHandler() {
}

string StaticPageHandler::Process(Request req,Html* html) {
    return Handler::Process(req,html);
}


LoginHandler::LoginHandler() {
}

string LoginHandler::Process(Request req,Html* html) {
    string body = req.GetBody();
    string username = DecodeURL(GetQuery(body,"username"));
    string password = DecodeURL(GetQuery(body,"password"));
    cout << "Username is: " << username << endl << "PassWord is: " << password << endl;
    auto kvService = new KVStorageService(username);
    if (kvService->CheckExist() && kvService->CheckPassword(password)) {
        Response resp(req.GetVersion(), html->GetRawData());
        resp.SetCookie("username",username);
        return resp.ToString();
    }

    Html* fail_html = new Html("frontend/html/login_failure.html");
    Response resp(req.GetVersion(), fail_html->GetRawData());
    return resp.ToString();
}


RegisterHandler::RegisterHandler() {

}

string RegisterHandler::Process(Request req,Html* html) {
    string body = req.GetBody();
    string username = DecodeURL(GetQuery(body,"username"));
    string password = DecodeURL(GetQuery(body,"password"));
    cout << "Username is: " << username << endl << "PassWord is: " << password << endl;
    auto kvService = new KVStorageService(username);
    if (!kvService->CheckExist()) {
        bool success = kvService->CreateUser(password);
        if (success) {
            Response resp(req.GetVersion(),html->GetRawData());
            return resp.ToString();
        }
    }
    Html* fail_html = new Html("frontend/html/register_failure.html");
    Response resp(req.GetVersion(), fail_html->GetRawData());
    return resp.ToString();
}

ChangePswHandler::ChangePswHandler() {

}

std::string ChangePswHandler::Process(Request req, Html *html) {
    auto cookies = req.GetCookies();
    string userName = cookies["username"];
    string body = req.GetBody();
    string oldPsw =  DecodeURL(GetQuery(body, "oldpassword"));
    string newPsw = DecodeURL(GetQuery(body,"newpassword"));
    auto kvService = new KVStorageService(userName);
    if(kvService->ChangePassword(oldPsw, newPsw)){
        Response resp(req.GetVersion(),html->GetRawData());
        return resp.ToString();
    }
    Html* fail_html = new Html("frontend/html/changepsw_failure.html");
    Response resp(req.GetVersion(), fail_html->GetRawData());
    return resp.ToString();
}

ListFileHandler::ListFileHandler() {

}

std::string ListFileHandler::Process(Request req, Html *html) {
    auto cookies = req.GetCookies();
    string userName = cookies["username"];
    string cur_dir_id;
    if(DecodeURL(GetQuery(req.GetParams(),"id")) == ""){
        cur_dir_id = "/";  // root
    }else{
        cur_dir_id = DecodeURL(GetQuery(req.GetParams(),"id"));
    }
    auto kvService = new KVStorageService(userName);
    auto dir = kvService->GetDirByDirId(cur_dir_id);
    cout << "dir id" << cur_dir_id << endl;
    auto cur_dir_name = dir.DirName_;
    cout << "content is" << dir.DirContent_.ToString() << endl;
    vector<string> directories = dir.DirContent_.dirIds_;
    vector<string> files = dir.DirContent_.fileId_;
    //cout << "file name" << files[0] << endl;
    map<string, string> all_dirs = kvService->GetMoveOptions(cur_dir_id);
    string file_option_list = "<label for=\"dest\">Move to</label><select name=\"dest\" id=\"dest\">";

    for (map<string, string>::iterator it = all_dirs.begin(); it != all_dirs.end(); it++) {
        file_option_list.append("<option value=\"" + it->first + "\">" + it->second + "</option>");
    }
    file_option_list.append("</select>");
    auto create = "<form action=\"/createdir?id=" + cur_dir_id + "\" method=\"post\"> <div class=\"container\"> <br><label for=\"dirname\"><b>New folder name</b></label> <input type=\"text\"" +
     string("value= \"\"")+ 
     "name=\"dirname\" required><button type=\"submit\">Create</button><br></div></form>"; 
    string page = "";
    page.append(create);
    page.append("<h1>" + cur_dir_name + "</h1>");
    page.append("<h2> Folders: </h2><br>");

    for(string dir_id : directories){
        string dir_name = kvService->GetDirInfo(dir_id, "name");
        cout << "dir is :" << dir_id;
        page.append("<a href=\"/listfile?id=" + dir_id + "\">" + dir_name + "</a ><br>");
        page.append("<a href=\"/deletefile?id=" + dir_id + "\"> Delete " + dir_name + "</a ><br>");
        page.append("<form action=\"/renamefile?id=" + dir_id + "\"" + "method=\"post\"> <div class=\"container\"> <input type=\"text\" name=\"newname\" required> <button type=\"submit\">Rename</button> </div></form>");

        // folder move drop down
        page.append("<form action=\"/movefile?id=" + dir_id + "\" method=\"post\">");
        page.append(file_option_list);
        page.append("<input type=\"submit\" value=\"Move\"></form><hr>");
    }

    page.append("<br><h2>Files: </h2><br>");
    page.append("<form action=\"/uploadfile?id=" + cur_dir_id + "\" method=\"post\" enctype=\"multipart/form-data\"><label for=\"file\">Upload file: </label><input type=\"file\" id=\"file\" name=\"file\"><input type=\"submit\" value=\"Upload\"> <br></form>");
    page.append("<hr>");
    for (string file_id : files) {
        string filename = kvService->GetDirInfo(file_id, "name");
        page.append("<a href=\"/downloadfile?id=" + file_id + "\">" + filename + "</a ><br>");
        page.append("<a href=\"/deletefile?id=" + file_id + "\"> Delete " + filename + "</a ><br><br>");
        page.append("<form action=\"/renamefile?id=" + file_id + "\"");
        page.append("method=\"post\"><div class=\"container\"><input type=\"text\" name=\"newname\" required><button type=\"submit\">Rename</button> </div></form>");
        page.append("<form action=\"/movefile?id=" + file_id + "\" method=\"post\">");
        page.append(file_option_list);
        page.append("<input type=\"submit\" value=\"Move\"></form><hr>");
    }   
    // cout << "page is:" << page << endl;
    if(cur_dir_id != "/"){
        string parent_id = kvService->GetDirInfo(cur_dir_id, "parent");
        parent_id = parent_id.substr(parent_id.find(":") + 1);
        page.append("<a href=\"/listfile?id=" + parent_id + "\"> Go Back </a><br>");
    }
    html->Replace("$allFolders",page);
    Response resp(req.GetVersion(), html->GetRawData());
    return resp.ToString();
}

CreateHandler::CreateHandler()
{
}

std::string CreateHandler::Process(Request req, Html *html)
{
    auto cookies = req.GetCookies();
    string userName = cookies["username"];
    string body = req.GetBody();
    string dir_name = DecodeURL(GetQuery(body,"dirname"));
    string parent_id = DecodeURL(GetQuery(req.GetParams(),"id"));
    cout << "New folder name is :" << dir_name << endl;
    cout << "Parent id is : " << parent_id << endl;
    auto kvService = new KVStorageService(userName);
    auto newDir = Dir(dir_name,parent_id);
    auto success = kvService->CreateFolder(newDir);
    Response resp(req.GetVersion(), html->GetRawData());
    return resp.ToString();
}

UploadHandler::UploadHandler() {

}

std::string UploadHandler::Process(Request req, Html *html) {
    string parent_id = DecodeURL(GetQuery(req.GetParams(),"id"));
    auto cookies = req.GetCookies();
    string userName = cookies["username"];
    string body = req.GetBody();
    FileBody fileBody(body);
    auto kvService = new KVStorageService(userName);
    auto file = File(fileBody.fileRaw_, fileBody.fileName_, parent_id);
    kvService->SaveFile(file);
    return Handler::Process(req, html);
}

DownloadHandler::DownloadHandler() {

}

std::string DownloadHandler::Process(Request req, Html *html) {
    string file_id = DecodeURL(GetQuery(req.GetParams(),"id"));
    auto cookies = req.GetCookies();
    string userName = cookies["username"];
    cout << "userName:" << userName << endl;
    cout << "file_id:" << file_id << endl;
    auto kvService = new KVStorageService(userName);
    File file = kvService->GetFile(file_id);
    string fileName = file.Name_;
    string fileRaw = file.Content_;
    Response resp(req.GetVersion(),html->GetRawData());
    resp.SetAttachment(fileName,fileRaw);
    return resp.ToString();
}

DeleteHandler::DeleteHandler() {

}

std::string DeleteHandler::Process(Request req, Html *html) {
    string file_id = DecodeURL(GetQuery(req.GetParams(),"id"));
    auto cookies = req.GetCookies();
    string userName = cookies["username"];
    auto kvService = new KVStorageService(userName);
    // cout << "delete file id :" << file_id << endl;
    if(file_id.substr(0, 1) == "D"){
        Dir dir = kvService->GetDirByDirId(file_id);
        auto success = kvService->DeleteFolder(dir);
    }else if(file_id.substr(0, 1) == "F"){
        File file = kvService->GetFileByFileId(file_id);
        auto success = kvService->DeleteFile(file);
    }
    Response resp(req.GetVersion(),html->GetRawData());
    return resp.ToString();
}

RenameHandler::RenameHandler() {

}

std::string RenameHandler::Process(Request req, Html *html) {
    string file_id = DecodeURL(GetQuery(req.GetParams(),"id"));
    string new_name = DecodeURL(GetQuery(req.GetBody(),"newname"));
    auto cookies = req.GetCookies();
    string userName = cookies["username"];
    auto kvService = new KVStorageService(userName);
    cout << "new name is" << new_name << endl;
    bool success;
    if(file_id.substr(0, 1) == "D"){
        Dir dir = kvService->GetDirByDirId(file_id);
        cout << "file id    " << file_id << endl;
        string old_name = dir.DirName_;
        success = kvService->CSetDirInfo(file_id, "name", old_name, new_name);
    }else{
        File file = kvService->GetFileByFileId(file_id);
        string old_name = file.Name_;
        success = kvService->CSetDirInfo(file_id, "name", old_name, new_name);
    }
    if (!success) {
        html = new Html("frontend/html/rename_fail.html");
    }
    Response resp(req.GetVersion(),html->GetRawData());
    return resp.ToString();
}

MoveHandler::MoveHandler() {

}

std::string MoveHandler::Process(Request req, Html *html) {
    string body = req.GetBody();
    auto cookies = req.GetCookies();
    string userName = cookies["username"];
    auto destDirId = DecodeURL(GetQuery(body,"dest"));
    auto fileOrDirId = DecodeURL(GetQuery(req.GetParams(),"id"));
    cout << "dest:" << destDirId << endl;
    cout << "origin:" << fileOrDirId << endl;
    if (fileOrDirId == destDirId) {
        html = new Html("frontend/html/move_fail.html");
        Response resp(req.GetVersion(),html->GetRawData());
        return resp.ToString();
    }
    bool isFile = fileOrDirId[0]== 'F';
    auto kvService = new KVStorageService(userName);
    cout << "isfile:" << isFile << endl;
    auto success = kvService->MoveDirOrFile(fileOrDirId,destDirId,isFile);
    if (!success) {
        html = new Html("frontend/html/move_fail.html");
    }
    Response resp(req.GetVersion(),html->GetRawData());
    return resp.ToString();
}

SendMailHandler::SendMailHandler() {
}

string SendMailHandler::Process(Request req,Html* html) {
    string body = req.GetBody();
    string rcpt = DecodeURL(GetQuery(body,"rcpt"));
    string server_name=rcpt.substr(rcpt.find("@")+1);
    string data = DecodeURL(GetQuery(body,"data"));
    auto cookies = req.GetCookies();
    string sender=cookies["username"];
    if(server_name=="t16"){
        string target=rcpt.substr(0,rcpt.find("@"));//encoded@
        cout<<"This mail is sent to >>>>>>>>>>>>>"<<endl;
        cout<<target<<endl;
        
        std::time_t result = std::time(nullptr);
        std::string format="From <"+sender+"@t16> "+std::asctime(std::localtime(&result))+data;
        auto kvService = new KVStorageService(target);
        bool success = kvService->SendMail(target,format);
            if (success) {
                Response resp(req.GetVersion(),html->GetRawData());
                return resp.ToString();
            }
    }else{
        string server_ip=dns_query(server_name);
        cout<<"The ip address for "+server_name+" is "+server_ip<<endl;
        int sockfd=socket(PF_INET,SOCK_STREAM,0);
        if(sockfd<0){
            fprintf(stderr,"Cannot open socket (%s)\n",strerror(errno));
            Response resp(req.GetVersion(), "Cannot open socket");
            return resp.ToString();
        }
        struct sockaddr_in servaddr;
        bzero(&servaddr,sizeof(servaddr));
        servaddr.sin_family=AF_INET;
        servaddr.sin_port=htons(25);
        inet_pton(AF_INET,server_ip.c_str(),&(servaddr.sin_addr));
        cout<<"Now connecting to server..."<<endl;
        if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr))<0){
            fprintf(stderr,"Cannot send connect to server (%s)\n",strerror(errno));
            Response resp(req.GetVersion(), "Cannot send connect to server");
            return resp.ToString();
        }
        cout<<"connected to "+server_ip+":25"<<endl;
        //send helo t16
        char response_buffer[1024];
      
        DoReadWithCRLF(sockfd,response_buffer);
        
        string response(response_buffer);
        if(response.find("220")!=string::npos){
            cout<<"connected to "+server_ip+":25"<<endl;
        }else{
            cout<<"Cannot connect: "+response<<endl;
        }

        string msg_to_send="helo seas.upenn.edu\r\n";
        if(DoWrite(sockfd,msg_to_send,msg_to_send.size())<0){
            fprintf(stderr,"Cannot send helo to server (%s)\n",strerror(errno));
            close(sockfd);
            Response resp(req.GetVersion(), "Cannot send helo to server");
            return resp.ToString();
        }
        
        memset(response_buffer,0,sizeof(response_buffer));
        DoReadWithCRLF(sockfd,response_buffer);
        response=response_buffer;
        
        if(response.find("250")!=string::npos){
            cout<<"Sent helo to target server: "+response<<endl;
        }else{
            cout<<"Receive error message: "+response<<endl;
        }

        msg_to_send="MAIL FROM:<"+sender+"@seas.upenn.edu>\r\n";
        if(DoWrite(sockfd,msg_to_send,msg_to_send.size())<0){
            fprintf(stderr,"Cannot send mail from to server (%s)\n",strerror(errno));
            close(sockfd);
            Response resp(req.GetVersion(), "Cannot send mail from to server");
            return resp.ToString();
        }
        memset(response_buffer,0,sizeof(response_buffer));
        DoReadWithCRLF(sockfd,response_buffer);
        response=response_buffer;
        if(response.find("250")!=string::npos){
            cout<<"Sent mail from to target server: "+response<<endl;
        }else{
            cout<<"MAIL FROM:Receive error message: "+response<<endl;
        }
        
        msg_to_send="RCPT TO:<"+rcpt+">\r\n";
        if(DoWrite(sockfd,msg_to_send,msg_to_send.size())<0){
            fprintf(stderr,"Cannot send rcpt to server (%s)\n",strerror(errno));
            close(sockfd);
            Response resp(req.GetVersion(), "Cannot send rcpt to server");
            return resp.ToString();
        }
        memset(response_buffer,0,sizeof(response_buffer));
        DoReadWithCRLF(sockfd,response_buffer);
        response=response_buffer;
        if(response.find("250")!=string::npos){
            cout<<"Sent rcpt to target server: "+response<<endl;
        }else{
            cout<<"RCPT: Receive error message: "+response<<endl;
        }

        msg_to_send="DATA\r\n";
        if(DoWrite(sockfd,msg_to_send,msg_to_send.size())<0){
            fprintf(stderr,"Cannot send DATA to server (%s)\n",strerror(errno));
            close(sockfd);
            Response resp(req.GetVersion(), "Cannot send DATA to server");
            return resp.ToString();
        }
        memset(response_buffer,0,sizeof(response_buffer));
        DoReadWithCRLF(sockfd,response_buffer);
        response=response_buffer;
        if(response.find("354")!=string::npos){
            cout<<"Sent DATA to target server: "+response<<endl;
        }else{
            cout<<"DATA: Receive error message: "+response<<endl;
        }

        msg_to_send=data;
        if(DoWrite(sockfd,msg_to_send,msg_to_send.size())<0){
            fprintf(stderr,"Cannot send email to server (%s)\n",strerror(errno));
            close(sockfd);
            Response resp(req.GetVersion(), "Cannot send email to server");
            return resp.ToString();
        }
        cout<<"Sent email data"<<endl;

        msg_to_send="\r\n.\r\n";
        if(DoWrite(sockfd,msg_to_send,msg_to_send.size())<0){
            fprintf(stderr,"Cannot send email to server (%s)\n",strerror(errno));
            close(sockfd);
            Response resp(req.GetVersion(), "Cannot send email to server");
            return resp.ToString();
        }
        memset(response_buffer,0,sizeof(response_buffer));
        DoReadWithCRLF(sockfd,response_buffer);
        response=response_buffer;
        if(response.find("250")!=string::npos){
            cout<<"Sent email to target server: "+response<<endl;
        }else{
            cout<<"FINISH DATA with \".\":Receive error message: "+response<<endl;
        }
        
        msg_to_send="QUIT\r\n";
        if(DoWrite(sockfd,msg_to_send,msg_to_send.size())<0){
            fprintf(stderr,"Cannot send QUIT to server (%s)\n",strerror(errno));
            close(sockfd);
            Response resp(req.GetVersion(), "Cannot send QUIT to server");
            return resp.ToString();
        }
        close(sockfd);


    }
    Response resp(req.GetVersion(), html->GetRawData());
    return resp.ToString();
    

}

AllMailHandler::AllMailHandler() {

}

std::string AllMailHandler::Process(Request req, Html *html) {
    vector<string> allmails;
    auto cookies = req.GetCookies();
    string sender=cookies["username"];
    // cout<<"We are now checking.....["<<sender<<"]'s mail box"<<endl;
    auto kvService = new KVStorageService(sender);
     bool success = kvService->GetAllMail(allmails);
     string page="<html><head><title>Your Inbox</title></head>\n<body>\n<h1>"+sender+"'s Inbox</h1>\n";
        if (success) {
            string htmlpage=htmlToString("frontend/html/template.html");
            
            string append="<h1>"+sender+"'s Inbox</h1>\n";
            append+="<ol class=\"list-group list-group-numbered\">";
            for(auto mail:allmails){
                string mailID=mail.substr(0,mail.find('#'));
                string mailsender=mail.substr(mail.find('<')+1,mail.find('>')-mail.find('<')-1);
                string rest=mail.substr(mail.find(">")+2);
                string mailcontent=rest.substr(0,rest.find("\r\n"))+"  ...";
              
                append+="<li class=\"list-group-item d-flex justify-content-between align-items-start\">";
                append+="<div class=\"ms-2 me-auto\">";
                append+="<div class=\"fw-bold\">"+mailsender+"</div>";
                append+=escapeHTML(mailcontent)+"<br><a class=\"btn btn-link btn-sm\" href=\"/viewmail?id=m"+mailID+"\" role=\"button\">View full email</a>";
                append+="</div></li>";
                
            }
            append+="</ol>";
            
            ReplaceAll(htmlpage,"$body",append);
            string navbar="<nav style=\"--bs-breadcrumb-divider: '>';\" aria-label=\"breadcrumb\"><ol class=\"breadcrumb\">";
            navbar+="<li class=\"breadcrumb-item\"><a href=\"/homepage\">Home</a></li>";
            navbar+="<li class=\"breadcrumb-item\"><a href=\"/mailcenter\">MailCenter</a></li>";
            navbar+="<li class=\"breadcrumb-item active\" aria-current=\"page\">Inbox</li>";
            navbar+="</ol></nav>";
            ReplaceAll(htmlpage,"$nav",navbar);
            Response resp(req.GetVersion(),htmlpage);
            return resp.ToString();
        }else{
            string page="<html><head><title>Inbox</title></head>\n<body>\n<h1>Your Inbox is empty</h1>\n";
            page+="<a class=\"btn btn-info\" href=\"/mailcenter\" role=\"button\">Return to mail center</a>";
            page+="</body>\n</html>\n";
            Response resp(req.GetVersion(),page);
            return resp.ToString();
        }
    return Handler::Process(req, html);
}

ViewMailHandler::ViewMailHandler() {

}

std::string ViewMailHandler::Process(Request req, Html *html) {
    cout<<"req param is>>>>>>>"+req.GetParams()<<endl;
    string mailID = DecodeURL(GetQuery(req.GetParams(),"id"));
    cout<<"mail ID is>>>>>>>>>>"+mailID<<endl;
    auto cookies = req.GetCookies();
    string sender=cookies["username"];
    // cout<<"We are now checking.....["<<sender<<"]'s mail box"<<endl;
    auto kvService = new KVStorageService(sender);
    string mailcontent;
    bool success = kvService->GetMail(mailID,mailcontent);
    string page="<html><head><title>Inbox</title></head>\n<body>\n<h1>Your Mail</h1>\n";;
    if(success){
        page+="<p>"+escapeHTML(mailcontent)+"<br><a class=\"btn btn-info\" href=\"/deletemail?id="+mailID+"\" role=\"button\">Delete</a>"+"</p>"; 
        page+="<br><a class=\"btn btn-info\" href=\"/replymail?id="+mailID+"\" role=\"button\">Reply</a><br>";
        page+="<br><a class=\"btn btn-info\" href=\"/forwardmail?id="+mailID+"\" role=\"button\">Forward</a><br>";
        
    }else{
        page+="<p>This mail does not exist!</p>";
    }
    page+="<br><a class=\"btn btn-info\" href=\"/allmails\" role=\"button\">Return to inbox</a>";
    page+="</body>\n</html>\n";
    Response resp(req.GetVersion(),page);
    return resp.ToString();
    // return Handler::Process(req, html);
}

ReplyMailHandler::ReplyMailHandler() {

}

std::string ReplyMailHandler::Process(Request req, Html *html) {
    cout<<"req param is>>>>>>>"+req.GetParams()<<endl;
    string mailID = DecodeURL(GetQuery(req.GetParams(),"id"));
    cout<<"mail ID is>>>>>>>>>>"+mailID<<endl;
    auto cookies = req.GetCookies();
    string sender=cookies["username"];
    // cout<<"We are now checking.....["<<sender<<"]'s mail box"<<endl;
    auto kvService = new KVStorageService(sender);
    string mailcontent;
    bool success = kvService->GetMail(mailID,mailcontent);
    string page="<html><head><title>Inbox</title></head>\n<body>\n<h1>Write Reply</h1>\n";;
    if(success){
        //parse mail content to sender's address and mail content
        string replyto=mailcontent.substr(mailcontent.find('<')+1,mailcontent.find('>')-mailcontent.find('<')-1);
        string mail="\r\n\r\n--------Reply to original-----------\r\n"+mailcontent;
        page+="<form action=\"/writemail\" method=\"post\">";
        page+="<label for=\"rcpt\">To: </label> <input type=\"text\" id=\"rcpt\" name=\"rcpt\" value=\""+replyto+"\"><br>";
        page+="<br><textarea id=\"data\" name=\"data\" rows=\"10\" cols=\"100\">"+mail+"</textarea><br><br><input type=\"submit\" value=\"Send\"></form>";
       
        
    }else{
        page+="<p>This mail does not exist!</p>";
    }
    page+="<a class=\"btn btn-info\" href=\"/allmails\" role=\"button\">Return to inbox</a>";
    page+="</body>\n</html>\n";
    Response resp(req.GetVersion(),page);
    return resp.ToString();
    // return Handler::Process(req, html);
}

ForwardMailHandler::ForwardMailHandler() {

}

std::string ForwardMailHandler::Process(Request req, Html *html) {
    cout<<"req param is>>>>>>>"+req.GetParams()<<endl;
    string mailID = DecodeURL(GetQuery(req.GetParams(),"id"));
    cout<<"mail ID is>>>>>>>>>>"+mailID<<endl;
    auto cookies = req.GetCookies();
    string sender=cookies["username"];
    // cout<<"We are now checking.....["<<sender<<"]'s mail box"<<endl;
    auto kvService = new KVStorageService(sender);
    string mailcontent;
    bool success = kvService->GetMail(mailID,mailcontent);
    string page="<html><head><title>Inbox</title></head>\n<body>\n<h1>Email Forward</h1>\n";;
    if(success){
        //parse mail content to sender's address and mail content
        string replyto=mailcontent.substr(mailcontent.find('<')+1,mailcontent.find('>')-mailcontent.find('<')-1);
        string mail="\r\n\r\n--------Forwarded message-----------\r\n"+mailcontent;
        page+="<form action=\"/writemail\" method=\"post\">";
        page+="<label for=\"rcpt\">To: </label> <input type=\"text\" id=\"rcpt\" name=\"rcpt\"><br>";
        page+="<br><textarea id=\"data\" name=\"data\" rows=\"10\" cols=\"100\">"+mail+"</textarea><br><br><input type=\"submit\" value=\"Send\"></form>";
       
        
    }else{
        page+="<p>This mail does not exist!</p>";
    }
    page+="<a class=\"btn btn-info\" href=\"/allmails\" role=\"button\">Return to inbox</a>";
    page+="</body>\n</html>\n";
    Response resp(req.GetVersion(),page);
    return resp.ToString();
    // return Handler::Process(req, html);
}

DeleteMailHandler::DeleteMailHandler() {

}

std::string DeleteMailHandler::Process(Request req, Html *html) {
    cout<<"req param is>>>>>>>"+req.GetParams()<<endl;
    string mailID = DecodeURL(GetQuery(req.GetParams(),"id"));
    cout<<"mail ID is>>>>>>>>>>"+mailID<<endl;
    auto cookies = req.GetCookies();
    string sender=cookies["username"];
    // cout<<"We are now checking.....["<<sender<<"]'s mail box"<<endl;
    auto kvService = new KVStorageService(sender);
    string mailcontent;
    bool success = kvService->DeleteMail(mailID);
    string page="<html><head><title>Delete</title></head>\n<body>\n<h1>Delete email</h1>\n";;
    if(success){
        page+="<p>mail deleted</p>"; 
        
    }else{
        page+="<p>This mail does not exist!</p>";
    }
    page+="<a class=\"btn btn-info\" href=\"/allmails\" role=\"button\">Return to inbox</a>";
    page+="</body>\n</html>\n";
    Response resp(req.GetVersion(),page);
    return resp.ToString();
   
}
