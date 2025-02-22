#include "CommandRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPResponse.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Object.h"
#include "Poco/Dynamic/Var.h"

#include "Poco/Exception.h"
#include "SharedVariables.h"
#include <fstream>

using Poco::Util::Application;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPResponse;
using Poco::JSON::Parser;
using Poco::JSON::Object;
using Poco::Dynamic::Var;
using Poco::Exception;


void handleAuth(HTTPServerRequest& request, HTTPServerResponse& response) {
	Application& app = Application::instance();
	app.logger().information("Request from: " + request.clientAddress().toString());

	std::string method = request.getMethod();
	std::string uri = request.getURI();

	if (method == "POST") {
		response.setChunkedTransferEncoding(true);
		response.setContentType("application/json");
		std::ostream& ostr = response.send();
		std::istream& bodyStream = request.stream();
		int length = request.getContentLength();
		char* buffer = new char[length];
		buffer[length] = '\0';
		bodyStream.read(buffer, length);
		std::string body(buffer);
		std::cout << "Here: " << body << std::endl;
		ostr << "{\"success\": true}";
	} else if (method == "GET") {
		if (uri == "/live" || uri == "/live/index.html") {
			
			std::string line;

			// Read from the text file
			std::ifstream indexFileStream("www/index.html");
			if (!indexFileStream.good()) {
				app.logger().error("Could not find the index.html file");
				indexFileStream.close();
				response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
				response.send();
			}
			response.setStatus(HTTPResponse::HTTP_OK);
			std::ostream& ostr = response.send();

			while (getline(indexFileStream, line)) {
				ostr << line;
				ostr << "\r\n";
			}

			// Close the file
			indexFileStream.close();
		}
		else {
			response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
			std::ostream& ostr = response.send();
			ostr << "Could not find the requested page, go to localhost/live to see the live stream";
		}
		
	} else {
		response.setChunkedTransferEncoding(true);
		response.setContentType("application/json");
		std::ostream& ostr = response.send();
		ostr << "{\"success\": false}";
	}	
}

void handleCommand(std::string jsonCommand, WebSocket ws, HandlerList* handlers) {
	clientSetMutex.lock();
	if (!clients.count(ws)) {
		clientSetMutex.unlock();
		std::string errorMessage = "{\"error\": true, \"message\": \"Error: User not authenicated\"";
		ws.sendFrame(errorMessage.c_str(), errorMessage.length());
	} else {
		clientSetMutex.unlock();
	}
	Application& app = Application::instance();
	//app.logger().information("Your JSON Command: " + jsonCommand);

	Object::Ptr pObject = nullptr;
	try {
		Parser jsonParser;
		Var result = jsonParser.parse(jsonCommand);
		app.logger().information("Var: " + result);
		pObject = result.extract<Object::Ptr>();
	} catch (Exception ex) {
		std::string error = "{ \"error\": true, \"message\": \"Could not get JSON: " + ex.message() + "\"}";
		app.logger().error(error);
		ws.sendFrame(error.c_str(), error.length());
		return;
	}
	if (pObject != nullptr) {
		for (Object::Iterator it = pObject->begin(); it != pObject->end(); it++) {
			handlers->callHandler(it->first, pObject, ws);
		}
	}
}