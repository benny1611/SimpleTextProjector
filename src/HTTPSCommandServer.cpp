#include "HTTPSCommandServer.h"

HTTPSCommandServer::HTTPSCommandServer(std::string url) : url(url) {
	Poco::Net::initializeSSL();
	initialize(*this);
	Application& app = Application::instance();
	handlers = new HandlerList(&app.logger());
}

HTTPSCommandServer::~HTTPSCommandServer() {
	Poco::Net::uninitializeSSL();
	delete handlers;
	delete this;
}

void HTTPSCommandServer::initialize(Application& self) {
	loadConfiguration(); // load default configuration files, if present
	ServerApplication::initialize(self);
}

void HTTPSCommandServer::uninitialize() {
	ServerApplication::uninitialize();
}

void HTTPSCommandServer::defineOptions(OptionSet& options) {
	ServerApplication::defineOptions(options);
}

void HTTPSCommandServer::handleOption(const std::string& name, const std::string& value) {
	ServerApplication::handleOption(name, value);
}

int HTTPSCommandServer::main(const std::vector<std::string>& args) {
	Application& app = Application::instance();

	unsigned short port = (unsigned short)config().getInt("HTTPSCommandServer.HTTPSport", 9443);

	// set-up a server socket
	SecureServerSocket svs(port);
	// set-up a HTTPServer instance
	srv = new HTTPServer(new HTTPSCommandRequestHandlerFactory(handlers, url), svs, new HTTPServerParams);
	// start the HTTPServer
	srv->start();
	// wait for CTRL-C or kill
	waitForTerminationRequest();

	return Application::EXIT_OK;
}

void HTTPSCommandServer::stop() {
	srv->stop();
}