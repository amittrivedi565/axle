#include "server.hpp"
#include "config.hpp"
#include <iostream>
#include <string>

int main() {
    // LOAD CONFIGURATION FILE
    Config::instance().load("../config.txt");

    // GET PORT FROM CONFIG
    std::string port_str = Config::instance().get("GATEWAY_PORT");

    int port = Config::stringToInt(port_str);

    // CREATE HTTP SERVER INSTANCE
    HttpServer server(port);

    // RUN THE SERVER
    server.run();
    return 0;
}
