#include "version.h"
#include <ProgramOptions.hxx>
#include <chrono>
#include <ctime>
#include <future>
#include <iostream>
#include <kr/base64.h>
#include <kr/krstring.h>
#include <kr/timer.h>
#include <mutex>
#include <set>
#include <stdio.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <time.h>
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
std::mutex conMutex;


string generateMessage(int count)
{
    int64_t timestamp = time(0);
    string msg = "";
    msg += R"({"timestamp":)" + kr::itostr(timestamp) +
           R"(, "id":"5555", "payload":[)";

    for (int i = 0; i < count; ++i)
    {

        int id = rand() % 100;
        msg += R"({"id":")" + kr::itostr(id) + R"(", "timestamp":)" +
               kr::itostr(timestamp - rand() % 10000) +
               R"(, data:"asdasdads"},)";
    }

    // remove last ,
    msg.pop_back();

    msg += R"(]})";

    return msg;
}
//-----------------------------------------------------------------------------
void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    std::cout << "received: " + msg->get_payload() << std::endl;
    // webSocketServer.send(hdl, "test", websocketpp::frame::opcode::text);
    if (msg->get_payload() == "exit")
    {
        connections.clear();
        webSocketServer.stop();
    }
}
//-----------------------------------------------------------------------------
void on_open(websocketpp::connection_hdl hdl)
{
    cout << "on_open" << endl;
    lock_guard<mutex> lock(conMutex);
    connections.insert(hdl);
}
//-----------------------------------------------------------------------------
void on_close(websocketpp::connection_hdl hdl)
{
    cout << "on_close" << endl;
    lock_guard<mutex> lock(conMutex);
    connections.erase(hdl);
}
//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    po::parser parser;

    parser["delay"].abbreviation('d').type(po::i32).fallback(100).description(
        "Delay between messages [ns]");
    parser["count"].abbreviation('c').type(po::i32).fallback(100).description(
        "numer of submessages in payload");
    parser["port"].abbreviation('p').type(po::u32).fallback(9000);
    parser["verbose"].abbreviation('v').description("Verbose output");
    parser["version"].description("Prints version");

    parser["help"]
        .abbreviation('?')
        .abbreviation('h')
        .description("print this help screen")
        .callback([&] { std::cout << parser << '\n'; });

    parser(argc, argv);
    if (parser["help"].size())
        return 0;
    if (parser["version"].size())
    {
        cout << "websocketpp example server" << endl;
        cout << Version::getVersionLong() << endl;
        return 0;
    }

    int delay = parser["delay"].get().i32;
    int count = parser["count"].get().i32;
    bool verbose = parser["verbose"].size();
    srand(time(NULL));

    webSocketServer.set_message_handler(&on_message);
    webSocketServer.set_open_handler(&on_open);
    webSocketServer.set_close_handler(&on_close);
    webSocketServer.set_listen_backlog(64);

    if (not verbose)
        webSocketServer.clear_access_channels(websocketpp::log::alevel::all);


    webSocketServer.init_asio();
    webSocketServer.listen(parser["port"].get().u32);
    webSocketServer.start_accept();

    bool done = false;
    auto sendThread = async(std::launch::async, [delay, count, verbose,
                                                    &done]() {
        kr::SpeedTimer t;
        int laps = 0;
        long sentBytes = 0;
        long limit = 0;
        t.start();
        while (not done)
        {
            if (verbose)
                cout << "sending data" << endl;

            std::this_thread::sleep_for(std::chrono::nanoseconds(delay * 1000));

            string msg = generateMessage(count);
            sentBytes += msg.length();
            long lastPacket = msg.length() / 1024;

            lock_guard<mutex> lock(conMutex);
            for (auto& con : connections)
            {
                webSocketServer.send(
                    con, msg, websocketpp::frame::opcode::text);
            }

            laps++;
            if (laps > limit)
            {
                t.stop();
                laps = 0;

                // identify correct frequency of reporting
                double time = t.getSec();
                if (time < 0.5)
                    limit += 1 + limit / 5;
                else
                    limit -= 1 + limit / 5;

                double speed = double(sentBytes) / t.getSec() / 1024;
                t.start();
                sentBytes = 0;
                if (speed > 10000)
                    cout << "Data are sent at" << speed / 1024
                         << "MB/s, last packet: " << lastPacket << "kB" << endl;
                else
                    cout << "Data are sent at" << speed
                         << "kB/s, last packet: " << lastPacket << "kB" << endl;
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
