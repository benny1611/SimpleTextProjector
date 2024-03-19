#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Task.h"
#include "CommandRequestHandler.h"

using Poco::Util::OptionSet;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Util::Application;
using Poco::Net::HTTPRequestHandlerFactory;


class HTTPCommandServer : public Poco::Util::ServerApplication {
public:
	HTTPCommandServer();
	~HTTPCommandServer();
protected:
	void initialize(Application& self);
	void uninitialize();
	void defineOptions(OptionSet& options);
	void handleOption(const std::string& name, const std::string& value);
	int main(const std::vector<std::string>& args);
};

class HTTPCommandRequestHandler : public HTTPRequestHandler {
	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
		handle(request, response);
	}
};

class HTTPCommandRequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
	HTTPCommandRequestHandlerFactory() {
	}
	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) {
		if (request.getURI() == "/") {
			return new HTTPCommandRequestHandler();
		}
		else {
			return 0;
		}
	}
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