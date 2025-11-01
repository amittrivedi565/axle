#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <string>
#include <netinet/in.h>

class HttpServer
{
    public:
        HttpServer(int port);
        ~HttpServer();
        void run();

    private:
        int port_;
        int server_fd_;
        struct sockaddr_in address_;

        void init_server();
        void handle_req(int client_fd);
};

#endif
