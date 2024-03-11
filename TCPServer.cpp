#include "TCPServer.h"
#include "spdlog/spdlog.h"
#include "tinythread.h"
#include "Session.h"

using Thread = tthread::thread;
using asio::ip::tcp;

void runServer(void* arg);

int _port;

void TCPServer::start() {
    Thread serverThread(runServer, 0);
    serverThread.detach();
}

void TCPServer::stop() {
    //TODO: Implement
}

TCPServer::TCPServer(int port) {
	_port = port;
}

class Server {
public:
    Server(asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        socket_(io_context) {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(socket_,
            [this](std::error_code ec)
            {
                if (!ec) {
                    std::make_shared<Session>(std::move(socket_))->start();
                }

                do_accept();
            });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

void runServer(void* arg) {
    try {
        asio::io_context io_context;

        Server s(io_context, _port);

        io_context.run();
    }
    catch (std::exception& e) {
        spdlog::error("Exception: {}", e.what());
    }
}