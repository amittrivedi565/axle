#include "server.hpp"
#include <sys/types.h>
#include <sys/event.h>
#include <sys/time.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <fcntl.h>

#define MAX_EVENTS 10
#define BUFFER_SIZE 1024

/**
 * Creates a Simple HTTP Server.
 */
void HttpServer::init_server(){
    /**
     * Create socket with specified address
     * `SOCK_STREAM` bi-directional socket
     */
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd_ == -1){
        perror("socket init failed");
        exit(EXIT_FAILURE);
    }

    /**
     * `setsockopt` means to reuse the same socket addr & port.
     */
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    /**
     * Make the SERVER SOCKET non blocking for i/o multiplexing.
     */
    fcntl(server_fd_, F_SETFL, O_NONBLOCK);

    address_.sin_family = AF_INET;
    address_.sin_addr.s_addr = INADDR_ANY;
    address_.sin_port = htons(port_);

    /**
     * Bind the socket to the port.
     */
    if (bind(server_fd_, (struct sockaddr*)&address_, sizeof(address_)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    /**
     * Listen for incoming requests.
     */
    if (listen(server_fd_, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "server listening on port " << port_ << std::endl;
}

void HttpServer::handle_req(int client_fd) {
    char buffer[BUFFER_SIZE];
    int valread = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (valread > 0) {
        buffer[valread] = '\0';
        std::cout << "received request:\n" << buffer << std::endl;

        const char* response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
        send(client_fd, response, strlen(response), 0);
    }
    close(client_fd);
}

void HttpServer::run() {
    /**
     * Creates a `kqueue` instance
     * Returns a fd that represents the queue
     * It creates kernel level event queue
     */
    int kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
        exit(EXIT_FAILURE);
    }

    /**
     * `kevent` struct contains various attributes and refs
     * 
     * `EV_SET` inits a kevent struct, which tells kernel what events to be monitored
     * 
     * `server_fd_` fd on which events will occur
     * 
     * `EVFILT_READ` monitor for readable events (incoming connections)
     * 
     * `EV_ADD | EV_ENABLE` add events to the queue and enable it
     * 
     * IMPORTANT
     *      `kevent` registers this event with the kernel
     *      `kq` will notify us whenever `server_fd_` is readable, ready for connection
    */
    struct kevent change;
    EV_SET(&change, server_fd_, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);

    if (kevent(kq, &change, 1, nullptr, 0, nullptr) == -1) {
        perror("kevent add server_fd");
        exit(EXIT_FAILURE);
    }

    struct kevent events[MAX_EVENTS];

    /*Event Loop*/
    while (true) {

        /**
        *`kevent` is called to wait for events

        *`events` array to store triggered events

        *`MAX_EVENTS` maximum events to be retreived at once

        * `nev` returns number of events returned by kevent.
        */
        int nev = kevent(kq, nullptr, 0, events, MAX_EVENTS, nullptr);
        if (nev == -1) {
            perror("kevent wait");
            exit(EXIT_FAILURE);
        }

        for (int i = 0; i < nev; ++i) {
            /**
             * `events[i].ident` file descriptor associated with the event
             * Now it checks whethever it is `server_fd_` or `client_sock`
             * 
             * If it is the server socket, it indicates that there is client trying to connect
             * accept the incoming connection.
             */
            int fd = events[i].ident;
            if (fd == server_fd_) {

                /**
                 * Accept the incoming connection
                 */
                int client_fd = accept(server_fd_, nullptr, nullptr);
                if (client_fd == -1) {
                    perror("accept");
                    continue;
                }

                /**
                 * Make the specific client fd non blocking 
                 * This is required since some slow clients will bottleneck the server
                 * We don't have to wait for one client to finish
                 *  NON BLOCKING I/O MULTIPLEXING
                */
                fcntl(client_fd, F_SETFL, O_NONBLOCK);

                /**
                 * Register client with kqueue
                 * Notify us when new data is ready to be read
                 * Now, kernel will notify us whenever client is ready to send data
                 */
                struct kevent client_event;
                EV_SET(&client_event, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
                kevent(kq, &client_event, 1, nullptr, 0, nullptr);
            } else {
                /**
                 * Handle client request, repsonse with appropritate attributes
                 */
                handle_req(fd);
            }
        }
    }
}