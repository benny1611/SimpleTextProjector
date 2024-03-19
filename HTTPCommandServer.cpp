#include "HTTPCommandServer.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"

using Poco::Net::ServerSocket;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;

HTTPCommandServer::HTTPCommandServer() {
	initialize(*this);
}

HTTPCommandServer::~HTTPCommandServer() {
	delete this;
}

void HTTPCommandServer::initialize(Application& self) {
	loadConfiguration(); // load default configuration files, if present
	ServerApplication::initialize(self);
}

void HTTPCommandServer::uninitialize() {
	ServerApplication::uninitialize();
}

void HTTPCommandServer::defineOptions(OptionSet& options) {
	ServerApplication::defineOptions(options);
}

void HTTPCommandServer::handleOption(const std::string& name, const std::string& value) {
	ServerApplication::handleOption(name, value);
}

int HTTPCommandServer::main(const std::vector<std::string>& args) {
	unsigned short port = (unsigned short)config().getInt("HTTPCommandServer.port", 9980);
	// set-up a server socket
	ServerSocket svs(port);
	// set-up a HTTPServer instance
	HTTPServer srv(new HTTPCommandRequestHandlerFactory(), svs, new HTTPServerParams);
	// start the HTTPServer
	srv.start();
	// wait for CTRL-C or kill
	waitForTerminationRequest();
	// Stop the HTTPServer
	srv.stop();

	return Application::EXIT_OK;
}