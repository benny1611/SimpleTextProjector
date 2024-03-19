#pragma once

#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequestImpl.h"
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
protected:
	void initialize(Application& self);
	void uninitialize();
	void defineOptions(OptionSet& options);
	void handleOption(const std::string& name, const std::string& value);
	int main(const std::vector<std::string>& args);
};


class HTTPSCommandRequestHandler : public HTTPRequestHandler {
public:
	HTTPSCommandRequestHandler() {

	}

	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
		Application& app = Application::instance();
		app.logger().information("Request from: " + request.clientAddress().toString());

		SecureStreamSocket socket = static_cast<HTTPServerRequestImpl&>(request).socket();
		if (socket.havePeerCertificate()) {
			X509Certificate cert = socket.peerCertificate();
			app.logger().information("Client certificate: " + cert.subjectName());
		}
		else {
			app.logger().information("No client certificate available.");
		}

		response.setChunkedTransferEncoding(true);
		response.setContentType("application/json");
		std::ostream& ostr = response.send();
		ostr << "{\"success\": true}";
	}
};

class HTTPSCommandRequestHandlerFactory : public HTTPRequestHandlerFactory {
public:
	HTTPSCommandRequestHandlerFactory() {
	}
	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) {
		if (request.getURI() == "/") {
			return new HTTPSCommandRequestHandler();
		}
		else {
			return 0;
		}
	}
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