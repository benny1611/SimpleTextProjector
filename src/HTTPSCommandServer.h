#pragma once

#include "Poco/Util/OptionSet.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Net/SecureStreamSocket.h"
#include "Poco/Net/HTTPServerRequestImpl.h"
#include "Poco/Net/X509Certificate.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/SecureServerSocket.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "SharedVariables.h"
#include "Poco/Task.h"
#include "WebSocketHandler.h"
#include "HandlerList.h"

using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Util::Application;
using Poco::Net::SecureStreamSocket;
using Poco::Net::HTTPServerRequestImpl;
using Poco::Net::X509Certificate;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::SecureServerSocket;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::Util::OptionSet;

class HTTPSCommandServer : public Poco::Util::ServerApplication {
public:
	HTTPSCommandServer();
	~HTTPSCommandServer();
	void stop();
protected:
	void initialize(Application& self);
	void uninitialize();
	void defineOptions(OptionSet& options);
	void handleOption(const std::string& name, const std::string& value);
	int main(const std::vector<std::string>& args);
private:
	HTTPServer* srv;
	HandlerList* handlers;
};


class HTTPSCommandRequestHandler : public HTTPRequestHandler {
public:
	HTTPSCommandRequestHandler() {}

	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
		handleAuth(request, response);
	}
};

class HTTPSCommandRequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
	HTTPSCommandRequestHandlerFactory(HandlerList* handlers) {
		this->handlers = handlers;
	}
	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) {
		
		if (request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0) {
			return new WebSocketRequestHandler(handlers);
		} else {
			return new HTTPSCommandRequestHandler();
		}
		//if (request.getURI() == "/") {
	}
private:
	HandlerList* handlers;
};

class HTTPSServerTask : public Poco::Task {
public:
	HTTPSServerTask(HTTPSCommandServer* HTTPSServer, int argc, char** argv) : Task("HTTPSServerTask"),
		_HTTPSServer(HTTPSServer), _argc(argc), _argv(argv) {}
	void runTask() {
		_HTTPSServer->run(_argc, _argv);
	}
private:
	HTTPSCommandServer* _HTTPSServer;
	int _argc;
	char** _argv;
};