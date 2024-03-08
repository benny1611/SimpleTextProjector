#include "TCPServer.h"
#include "spdlog/spdlog.h"
#include "tinythread.h"
#include "Session.h"

using Mutex = tthread::mutex;
using Thread = tthread::thread;
using asio::ip::tcp;

void runServer(void* arg);

Mutex* _textMutex;
char** _text;
std::string* _fontPath;
int _port;
bool keepRunnung = true;

void TCPServer::start() {
    Thread serverThread(runServer, 0);
    serverThread.detach();
}

void TCPServer::stop() {
    keepRunnung = false;
}

TCPServer::TCPServer(int port, Mutex* mutex, char** text, std::string* fontPath) {
	_port = port;
	_textMutex = mutex;
	_text = text;
    _fontPath = fontPath;
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
                    std::make_shared<Session>(std::move(socket_), _textMutex, _text, _fontPath)->start();
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