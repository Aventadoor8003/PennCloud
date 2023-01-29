#pragma once

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <mutex>
#include <thread>

#define MAX_CLIENTS 101
#define BUFF_SIZE 1024

/**
 * A base server to be inherited
 * Provided basic multithreaded framework and simple thread locks
 */

/// @brief A base multi-threaded server framework
class BaseServer {
public:

    /// @brief No argument constructor
    BaseServer();

    /// @brief No argument destructor
    ~BaseServer();


    /// @brief Parse config and set up the server. Both argc and argv are from main
    /// @param argc Number of arguments
    /// @param argv Actual arguments
    /// @return return status
    virtual int ParseConfig(int argc, char** argv)=0;

    /// @brief Start the server
    /// @return return status
    virtual int Run();

    /// @brief  Stop the server
    /// @return return status
    /// Can be used in signal handler
    virtual int Stop();


protected:
    //----------------------- Server configuration ---------------------
    // Verbose mode
    int vflag_;
    // The file descriptor which the server listens to
    int listen_fd_;


    //Number of active file descriptors
    int fd_cnt_;

    //An array recording all active file descriptors
    int active_fd_[MAX_CLIENTS];

    /// @brief Server address
    std::string addr_str_;

    sockaddr_in server_inet_;
    std::mutex fd_list_mutex_;

    //------------------------- Useful methods ------------------------
    /// @brief Worker method
    /// @param param A pointer all params needed to pass in
    /// Please implement your own type cast in your worker method
    virtual void Worker(void* param)=0;

    /// @brief open a file descriptor for listening purpose
    /// @return true: successfully set up listen fd. False: failed to set up listen fd
    bool SetupListenFd();

    /// @brief Default addr: localhost 4711. Please override or overload this function in your class
    /// @return true: successfully set up listen addr. False: failed to set up
    virtual bool SetupListenAddr();

    /// @brief Add a file descriptor to this server
    /// @param fd the new fd to be added
    /// @return if succesfully added
    bool AddFd(int fd);

    /// @brief Remoce a file descriptor to this server
    /// @param fd the fd to be deleted
    /// @return if succesfully deleted
    bool RemoveFd(int fd);

    /// @brief Read a whole sentence from socket. 
    // Attention: !!!!!!This function will trim <CR> and <LF> at the end!!!!!!
    /// @param conn_fd file descriptor to connection
    /// @param buffer buffer to save message
    /// @return if successfully read content, return true. If failed return false
    bool DoRead(int conn_fd, char buffer[]);

    // @brief Read a whole sentence from socket. 
    // Attention: !!!!!!This function will trim <CR> and <LF> at the end!!!!!!
    /// @param conn_fd file descriptor to connection
    /// @param buffer_string buffer string to save message
    /// @return if successfully read content, return true. If failed return false
    bool DoRead(int conn_fd, std::string& buffer_string);
    
    /// @brief Send message to client
    /// @param conn_fd fd to the connection socket
    /// @param buffer buffer to save bytes while writing
    /// @param len the length of message to be sent
    bool DoWrite(int conn_fd, const char* buffer, int len);

    /// @brief Send a string to socket and append crlf at the end
    /// @param fd the socket to send
    /// @param text content
    /// @return if successfully sent
    bool SendToSocketAndAppendCrlf(int fd, std::string text);

    /// @brief Send a string to socket
    /// @param fd communication fd
    /// @param text text content
    /// @return if successfully sent
    bool SendToSocket(int fd, std::string text);

    /// @brief Send an OK response and append CRLF at the end
    /// @param text text content
    /// @return if successfully sent
    bool SendOkResponse(int fd, std::string text);

    /// @brief Send an ERR response and append CRLF at the end
    /// @param text text content
    /// @return if succesfully sent
    bool SendErrResponse(int fd, std::string text);

    
};

///IMPROVE:
/**
 * 1. Add sigpipe handler
 * 2. Improve naming on AddFd, RemoveFd
 */