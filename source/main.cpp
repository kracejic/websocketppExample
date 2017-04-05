#include "version.h"
#include <ProgramOptions.hxx>
#include <future>
#include <iostream>
#include <set>
#include <stdio.h>
#include <string>
#include <vector>

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/config/core.hpp>
#include <websocketpp/server.hpp>

using namespace std;
// using ws = websocketpp;

using server = websocketpp::server<websocketpp::config::asio>;
server webSocketServer;
using Connections = std::set<websocketpp::connection_hdl,
    std::owner_less<websocketpp::connection_hdl>>;

Connections connections;


void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    std::cout << msg->get_payload() << std::endl;

    webSocketServer.send(hdl, "test", websocketpp::frame::opcode::text);
}
//-----------------------------------------------------------------------------
void on_open(websocketpp::connection_hdl hdl)
{
    cout << "connected" << endl;
    connections.insert(hdl);
}
//-----------------------------------------------------------------------------
void on_close(websocketpp::connection_hdl hdl)
{
    cout << "closed" << endl;
    connections.erase(hdl);
}
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    po::parser parser;
    parser["optimization"].abbreviation('O').type(po::u32).fallback(
        0); // if --optimization is not explicitly specified, assume 0

    parser["include-path"] // corresponds to --include-path
        .abbreviation('I') // corresponds to -I
        .type(po::string)  // expects a string
        .multi();          // allows multiple arguments for the same option

    parser(argc, argv);

    webSocketServer.set_message_handler(&on_message);
    webSocketServer.set_open_handler(&on_open);
    webSocketServer.set_close_handler(&on_close);
    webSocketServer.set_listen_backlog(64);


    webSocketServer.init_asio();
    webSocketServer.listen(9004);
    webSocketServer.start_accept();

    // async(std::launch::async, []() {
    //     // webSocketServer.websocketpp::transport::asio::basic_socket::endpoint::is_secure
    //     while (true)
    //     {
    //         sleep(1);
    //         for (auto& con : connections)
    //         {
    //             webSocketServer.send(con, "test", websocketpp::frame::opcode::text);
    //         }
    //     }
    // });

    webSocketServer.run();
}
