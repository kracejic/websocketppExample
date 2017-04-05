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
// TODO mutex


void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    std::cout << msg->get_payload() << std::endl;

    webSocketServer.send(hdl, "test", websocketpp::frame::opcode::text);

    if (msg->get_payload() == "exit")
    {
        connections.clear();
        webSocketServer.stop();
    }
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

    parser["delay"].abbreviation('d').type(po::u32).fallback(100).description(
        "Delay between messages");
    parser["port"].abbreviation('p').type(po::u32).fallback(9000);
    parser["verbose"].abbreviation('v').description("Verbose output");

    parser["help"]
        .abbreviation('?')
        .abbreviation('h')
        .description("print this help screen")
        .callback([&] { std::cout << parser << '\n'; });

    parser(argc, argv);
    if (parser["help"].size())
        return 0;

    int delay = parser["delay"].get().u32;

    webSocketServer.set_message_handler(&on_message);
    webSocketServer.set_open_handler(&on_open);
    webSocketServer.set_close_handler(&on_close);
    webSocketServer.set_listen_backlog(64);

    if (not parser["verbose"].size())
        webSocketServer.clear_access_channels(websocketpp::log::alevel::all);


    webSocketServer.init_asio();
    webSocketServer.listen(parser["port"].get().u32);
    webSocketServer.start_accept();

    bool done = false;
    auto sendThread = async(std::launch::async, [delay, &done]() {
        while (not done)
        {
            cout << "sending batch" << endl;
            usleep(delay * 1000);
            for (auto& con : connections)
            {
                cout << "  * data:" << endl;
                webSocketServer.send(
                    con, "test", websocketpp::frame::opcode::text);
            }
        }
    });

    cout << "Starting server" << endl;
    webSocketServer.run();
    done = true;
    cout << "Stopping sending thread" << endl;
    sendThread.get();
    cout << "Sending thread stopped" << endl;
}
