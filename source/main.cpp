#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>
#include <websocketpp/config/core.hpp>
#include <websocketpp/server.hpp>


#include "version.h"


#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

using namespace std;

typedef websocketpp::server<websocketpp::config::asio> server;
server print_server;

void on_message(websocketpp::connection_hdl hdl, server::message_ptr msg)
{
    std::cout << msg->get_payload() << std::endl;

    print_server.send(hdl, "test", websocketpp::frame::opcode::text);
}

bool on_validate(websocketpp::connection_hdl hdl)
{
    cout << "connected" <<endl;
    return true;
}
void on_preinit(websocketpp::connection_hdl hdl)
{
    cout << "pre" <<endl;
}
void on_postinit(websocketpp::connection_hdl hdl)
{
    cout << "post" <<endl;
}
void on_open(websocketpp::connection_hdl hdl)
{
    cout << "connected" <<endl;
}

void on_close(websocketpp::connection_hdl hdl)
{
    cout << "closed" <<endl;
}

int main()
{

    print_server.set_message_handler(&on_message);
    print_server.set_open_handler(&on_open);
    print_server.set_close_handler(&on_close);
    print_server.set_validate_handler(&on_validate);
    print_server.set_tcp_pre_init_handler(&on_preinit);
    print_server.set_tcp_post_init_handler(&on_postinit);
    print_server.set_listen_backlog(64);
    


    print_server.init_asio();
    print_server.listen(9003);
    print_server.start_accept();

    print_server.run();
}
