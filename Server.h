#pragma once
#include "Poco/Mutex.h"
#include "Poco/Dynamic/Var.h"

using Poco::Mutex;
using Poco::Dynamic::Var;

class Server {
public:
	Server(int port, Mutex* mutex, char** text);
	void start();
	void stop();
private:
	int _port;
};

