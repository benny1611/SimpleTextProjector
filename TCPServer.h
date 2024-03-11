#pragma once

class TCPServer {
public:
	TCPServer(int port);
	void start();
	void stop();
};

