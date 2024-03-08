#pragma once
#include "tinythread.h"

using Mutex = tthread::mutex;

class TCPServer {
public:
	TCPServer(int port, Mutex* mutex, char** text, std::string* fontPath);
	void start();
	void stop();
};

