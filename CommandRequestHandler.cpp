#include "CommandRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Object.h"
#include "Poco/Dynamic/Var.h"
#include "Poco/Exception.h"
#include "Poco/Base64Decoder.h"
#include "Poco/StreamCopier.h"
#include "SharedVariables.h"
#include "Base64.h"

using Poco::Util::Application;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::JSON::Parser;
using Poco::JSON::Object;
using Poco::Dynamic::Var;
using Poco::Exception;
using Poco::Base64Decoder;
using Poco::StreamCopier;

void handleText(std::string& textValue, Application& app);

void handleAuth(HTTPServerRequest& request, HTTPServerResponse& response) {
	Application& app = Application::instance();
	app.logger().information("Request from: " + request.clientAddress().toString());

	response.setChunkedTransferEncoding(true);
	response.setContentType("application/json");
	std::ostream& ostr = response.send();
	std::string method = request.getMethod();

	if (method == "POST") {
		std::istream& bodyStream = request.stream();
		int length = request.getContentLength();
		char* buffer = new char[length];
		buffer[length] = '\0';
		bodyStream.read(buffer, length);
		std::string body(buffer);
		std::cout << "Here: " << body << std::endl;
		ostr << "{\"success\": true}";
	}
	else {
		ostr << "{\"success\": false}";
	}	
}

void handleCommand(std::string jsonCommand, WebSocket ws) {
	clientSetMutex.lock();
	if (!clients.count(ws)) {
		clientSetMutex.unlock();
		std::string errorMessage = "Error: User not authenicated";
		ws.sendFrame(errorMessage.c_str(), errorMessage.length());
	} else {
		clientSetMutex.unlock();
	}
	Application& app = Application::instance();
	app.logger().information("Your JSON Command: " + jsonCommand);

	Object::Ptr pObject;
	try {
		Parser jsonParser;
		Var result = jsonParser.parse(jsonCommand);
		app.logger().information("Var: " + result);
		pObject = result.extract<Object::Ptr>();
	} catch (Exception ex) {
		std::string error = "Could not get JSON: " + ex.message();
		app.logger().error(error);
		ws.sendFrame(error.c_str(), error.length());
		return;
	}
	if (pObject->has("text")) {
		std::string textValue = pObject->getValue<std::string>("text");
		handleText(textValue, app);
	}
}

void handleText(std::string& textValue, Application& app) {
	app.logger().information("Here's your encoded text: " + textValue);
	textMutex.lock();
	char* result = new char[textValue.size()];
	int outLen;
	macaron::Base64::Decode(textValue, result, outLen);
	result[outLen] = '\0';
	text = result;
	textMutex.unlock();

	std::string outputLogString(result);
	app.logger().information("Here's your decoded text: " + outputLogString);
}