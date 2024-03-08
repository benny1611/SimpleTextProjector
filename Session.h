#include "spdlog/spdlog.h"
#include "nlohmann/json.hpp"
#include "Base64.h"
#include "tinythread.h"
#include <memory>
#include <fstream>
#include <asio/ts/internet.hpp>
#include <asio/ts/buffer.hpp>

using asio::ip::tcp;
using json = nlohmann::json;
using Mutex = tthread::mutex;

class Session
    : public std::enable_shared_from_this<Session> {

public:
    Session(tcp::socket socket, Mutex* mutex, char** text, std::string* fontPath)
        : socket_(std::move(socket)), _textMutex(mutex), _text(text), _fontPath(fontPath) {}

    void start() {
        do_read();
    }

private:
    void do_read()
    {
        auto self(shared_from_this());
        socket_.async_read_some(asio::buffer(data_, max_length),
            [this, self](std::error_code ec, std::size_t length)
            {
                if (!ec) {
                    std::string message(data_, length);
                    spdlog::info("Here: {}", message);

                    try {
                        json command = json::parse(message);
                        execute_command(command);
                    }
                    catch (std::exception& ex) {
                        std::string errorMessage = ex.what();
                        strncpy_s(data_, errorMessage.c_str(), errorMessage.length());
                        do_write(errorMessage.length());
                    }

                    do_read();
                }
            });
    }

    void execute_command(json& command) {
        if (command.contains("text")) {
            handleText(command);
        }
        if (command.contains("font")) {
            handleFont(command);
        }
    }

    void handleText(json& command) {
        std::string text;
        try {
            text = command["text"].get<std::string>();
        }
        catch (std::exception& ex) {
            std::string errorMessage = ex.what();
            strncpy_s(data_, errorMessage.c_str(), errorMessage.length());
            do_write(errorMessage.length());
        }

        _textMutex->lock();
        char* result = new char[text.size()];
        int outLen;
        macaron::Base64::Decode(text, result, outLen);
        result[outLen] = '\0';
        *_text = result;
        _textMutex->unlock();
    }

    void handleFont(json& command) {
        std::string fileName;
        try {
            fileName = command["font"].get<std::string>();
        }
        catch (std::exception& ex) {
            std::string errorMessage = ex.what();
            strncpy_s(data_, errorMessage.c_str(), errorMessage.length());
            do_write(errorMessage.length());
            return;
        }
        std::string filePath = "fonts/" + fileName + ".ttf";
        std::ifstream file(filePath);
        if (!file.is_open()) {
            std::string errorMessage = "Error: File not found: " + filePath;
            strncpy_s(data_, errorMessage.c_str(), errorMessage.length());
            do_write(errorMessage.length());
            return;
        } else {
            file.close();
        }
        _textMutex->lock();
        *_fontPath = filePath;
        _textMutex->unlock();
    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());

        asio::async_write(socket_, asio::buffer(data_, length),
            [this, self](std::error_code ec, std::size_t /*length*/)
            {
                if (!ec)
                {
                    //ignore
                    //do_read();
                }
            });
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
    Mutex* _textMutex;
    char** _text;
    std::string* _fontPath;
};