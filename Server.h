#pragma once

#if defined(_WIN32)
#define NOGDI             // All GDI defines and routines
#define NOUSER            // All USER defines and routines
#endif

#include "Poco/Net/TCPServer.h"

#if defined(_WIN32)           // raylib uses these names as function parameters
#undef near
#undef far
#endif

class Server {
public:
	void setPort(int port);
	void start();
	void stop();
private:
	int _port;
};

