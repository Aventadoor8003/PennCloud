#include "frontend/global_vars.h"
#include "frontend/Http.h"
#include "frontend/utils.h"
#include<fstream>
#include<iostream>
using namespace std;

int ParseConfig(int argc, char** argv){
    string config_doc_name = argv[optind];
    int server_index = stoi(argv[optind + 1]);
    string line;
    ifstream myfile(config_doc_name);
    string server_string_id;
    int idx = 1;
    if (myfile.is_open()) {
        while (getline(myfile, line)) {
            if (idx == server_index) {
                server_string_id = Split(line, ':')[1];
            }
            idx++;
        }
        myfile.close();
    }
    else {
        fprintf(stdout, "Unable to open file %s", config_doc_name.c_str());
    }
    auto server_port = stoi(server_string_id);
    cout << "bind port:" << server_port << endl;
    return server_port;
}

int main(int argc, char *argv[])
{
    auto port = ParseConfig(argc, argv);
    Http http = Http(port);
    http.Register("/register",new StaticPageHandler(),"frontend/html/register.html");
    http.Register("/checkregister",new RegisterHandler(),"frontend/html/register_success.html");
    http.Register("/login",new StaticPageHandler(),"frontend/html/login.html");
    http.Register("/checklogin",new LoginHandler(),"frontend/html/login_success.html");
    http.Register("/changepsw",new StaticPageHandler(),"frontend/html/changepsw.html");
    http.Register("/checkchangepsw",new ChangePswHandler(),"frontend/html/changepsw_success.html");
    http.Register("/homepage",new StaticPageHandler(),"frontend/html/login_success.html");
    http.Register("/listfile",new ListFileHandler(),"frontend/html/storage.html");
    http.Register("/createdir",new CreateHandler(),"frontend/html/create_success.html");
    http.Register("/uploadfile",new UploadHandler(),"frontend/html/upload_success.html");
    http.Register("/downloadfile",new DownloadHandler(),"frontend/html/download_success.html");
    http.Register("/renamefile",new RenameHandler(),"frontend/html/rename_success.html");
    http.Register("/deletefile",new DeleteHandler(),"frontend/html/delete_success.html");
    http.Register("/movefile",new MoveHandler(),"frontend/html/move_success.html");
    
    http.Register("/mailcenter",new StaticPageHandler(),"frontend/html/mailservice.html");
    http.Register("/newmail",new StaticPageHandler(),"frontend/html/newmail.html");
    http.Register("/writemail",new SendMailHandler(),"frontend/html/sendmailsuccess.html");
    http.Register("/allmails",new AllMailHandler(),"frontend/html/mailservice.html");
    http.Register("/viewmail",new ViewMailHandler(),"frontend/html/mailservice.html");
    http.Register("/deletemail",new DeleteMailHandler(),"frontend/html/mailservice.html");
    http.Register("/replymail",new ReplyMailHandler(),"frontend/html/mailservice.html");
    http.Register("/forwardmail",new ForwardMailHandler(),"frontend/html/mailservice.html");
    
    return http.Run();
}

