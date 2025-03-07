#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Task.h"
#include "WebSocketHandler.h"
#include "HandlerList.h"

using Poco::Util::OptionSet;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPServer;
using Poco::Util::Application;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPResponse;


class HTTPCommandServer : public Poco::Util::ServerApplication {
public:
	HTTPCommandServer();
	~HTTPCommandServer();
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

class HTTPCommandRequestHandler : public HTTPRequestHandler {
	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
		handleAuth(request, response);
	}
};

class HTTPCommandRequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
	HTTPCommandRequestHandlerFactory(HandlerList* handlers) {
		this->handlers = handlers;
	}
	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) {
		if (request.find("Upgrade") != request.end() && Poco::icompare(request["Upgrade"], "websocket") == 0) {
			return new WebSocketRequestHandler(handlers);
		}
		else {
			return new HTTPCommandRequestHandler();
		}
	}
private:
	HandlerList* handlers;
};

class HTTPServerTask : public Poco::Task {
public:
	HTTPServerTask(HTTPCommandServer* HTTPSServer, int argc, char** argv) : Task("HTTPServerTask"),
		_HTTPServer(HTTPSServer), _argc(argc), _argv(argv) {}
	void runTask() {
		_HTTPServer->run(_argc, _argv);
	}
private:
	HTTPCommandServer* _HTTPServer;
	int _argc;
	char** _argv;
};