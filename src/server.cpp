#include "server.hpp"
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <fcntl.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

HttpServer::HttpServer(int port) {
    port_ = port;
}

HttpServer::~HttpServer() {
    std::cout << "SERVER SHUTTING DOWN...\n";
}

/**
 * CREATES A SIMPLE HTTP SERVER
 */
void HttpServer::init_server() {
    /**
     * CREATE SOCKET WITH SPECIFIED ADDRESS
     * SOCK_STREAM FOR BI-DIRECTIONAL COMMUNICATION
     */
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd_ == -1) {
        perror("SOCKET INIT FAILED");
        exit(EXIT_FAILURE);
    }

    /**
     * SET SO_REUSEADDR TO ALLOW REUSE OF THE SAME ADDRESS & PORT
     */
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("SETSOCKOPT FAILED");
        exit(EXIT_FAILURE);
    }

    /**
     * MAKE SERVER SOCKET NON-BLOCKING FOR I/O MULTIPLEXING
     */
    fcntl(server_fd_, F_SETFL, O_NONBLOCK);

    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = htons(port_);

    /**
     * BIND THE SOCKET TO THE PORT
     */
    if (bind(server_fd_, (struct sockaddr*)&address_, sizeof(address_)) < 0) {
        perror("BIND FAILED");
        exit(EXIT_FAILURE);
    }

    /**
     * LISTEN FOR INCOMING CONNECTIONS
     */
    if (listen(server_fd_, 10) < 0) {
        perror("LISTEN FAILED");
        exit(EXIT_FAILURE);
    }

    std::cout << "SERVER LISTENING ON PORT " << port_ << std::endl;
}

/**
 * HANDLES INCOMING CLIENT REQUESTS
 * EXAMPLE HTTP REQUEST FORMAT:
 * POST /api/users HTTP/1.1\r\n
 * HOST: localhost:8080\r\n
 * CONTENT-TYPE: application/json\r\n
 * CONTENT-LENGTH: 49\r\n
 * \r\n
 * {"name":"John Doe","email":"john@example.com"}
 */
void HttpServer::handle_req(int client_fd) {
    char buffer[BUFFER_SIZE];
    int valread = read(client_fd, buffer, BUFFER_SIZE - 1);
    
    if (valread <= 0) {
        close(client_fd);
        return;
    }

    /**
     * STORE INCOMING DATA INTO BUFFER AND CONVERT TO STRING
     */
    buffer[valread] = '\0';
    std::string request(buffer);
    std::cout << "REQUEST RECEIVED: " << request << std::endl;

    /**
     * PARSE REQUEST LINE USING ISSTRINGSTREAM
     * EXTRACT METHOD, PATH, AND HTTP VERSION
     * EXAMPLE: GET /service-name HTTP/1.1
     */
    std::istringstream req_stream(request);
    std::string method, path, version;
    req_stream >> method >> path >> version;
}

void HttpServer::run() {
    init_server();

    /**
     * CREATE A KQUEUE INSTANCE
     * RETURNS A FILE DESCRIPTOR REPRESENTING THE EVENT QUEUE
     */
    int kq = kqueue();
    if (kq == -1) {
        perror("KQUEUE FAILED");
        exit(EXIT_FAILURE);
    }

    /**
     * INITIALIZE KEVENT STRUCT TO MONITOR EVENTS
     * server_fd_ IS THE SOCKET FD TO WATCH
     * EVFILT_READ MONITORS READABLE EVENTS (INCOMING CONNECTIONS)
     * EV_ADD | EV_ENABLE ADDS AND ENABLES THE EVENT
     */
    struct kevent change;
    EV_SET(&change, server_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);

    if (kevent(kq, &change, 1, nullptr, 0, nullptr) == -1) {
        perror("KEVENT ADD SERVER_FD FAILED");
        exit(EXIT_FAILURE);
    }

    struct kevent events[MAX_EVENTS];

    /* EVENT LOOP */
    while (true) {
        /**
         * WAIT FOR EVENTS USING KEVENT
         * events ARRAY WILL STORE TRIGGERED EVENTS
         * MAX_EVENTS SPECIFIES MAXIMUM EVENTS TO RETRIEVE
         * nev RETURNS NUMBER OF EVENTS TRIGGERED
         */
        int nev = kevent(kq, nullptr, 0, events, MAX_EVENTS, nullptr);
        if (nev == -1) {
            perror("KEVENT WAIT FAILED");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nev; ++i) {
            /**
             * events[i].ident IS THE FILE DESCRIPTOR ASSOCIATED WITH THE EVENT
             * CHECK IF IT IS SERVER_FD_ (NEW CONNECTION) OR CLIENT FD (DATA)
             */
            int fd = events[i].ident;
            if (fd == server_fd_) {
                /**
                 * ACCEPT INCOMING CONNECTION
                 */
                int client_fd = accept(server_fd_, nullptr, nullptr);
                if (client_fd == -1) {
                    perror("ACCEPT FAILED");
                    continue;
                }

                /**
                 * MAKE CLIENT SOCKET NON-BLOCKING
                 * PREVENTS SLOW CLIENTS FROM BLOCKING SERVER
                 */
                fcntl(client_fd, F_SETFL, O_NONBLOCK);

                /**
                 * REGISTER CLIENT SOCKET WITH KQUEUE
                 * KERNEL WILL NOTIFY WHEN DATA IS READY TO READ
                 */
                struct kevent client_event;
                EV_SET(&client_event, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
                kevent(kq, &client_event, 1, nullptr, 0, nullptr);
            } else {
                /**
                 * HANDLE CLIENT REQUEST
                 */
                handle_req(fd);
            }
        }
    }
}
