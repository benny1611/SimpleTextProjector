#include "CommandRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/JSON/Parser.h"
#include "Poco/JSON/Object.h"
#include "Poco/Dynamic/Var.h"
#include "Poco/Exception.h"
#include "SharedVariables.h"
#include "Base64.h"
#include <fstream>

using Poco::Util::Application;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::JSON::Parser;
using Poco::JSON::Object;
using Poco::Dynamic::Var;
using Poco::Exception;

void handleText(std::string& textValue, Application& app);
std::string handleFont(std::string& fontPathValue, Application& app);
int handleFontSize(float fontSizeValue, Application& app);
std::string handleTextColor(Var& colorString, Application& app);

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
	if (pObject->has("text_color")) {
		//Object::Ptr color = pObject->getValue<Object::Ptr>("text_color");
		Var color = pObject->get("text_color");
		std::string error = handleTextColor(color, app);
		if (!error.empty()) {
			ws.sendFrame(error.c_str(), error.length());
		}
	}
	if (pObject->has("font")) {
		std::string fontPathValue = pObject->getValue<std::string>("font");
		std::string error = handleFont(fontPathValue, app);
		if (!error.empty()) {
			ws.sendFrame(error.c_str(), error.length());
		}
	}
	if (pObject->has("font_size")) {
		float fontSizeValue = pObject->getValue<float>("font_size");
		bool success = handleFontSize(fontSizeValue, app);
		if (!success) {
			std::string error = "Error: could not set font size to: ";
			error = error + std::to_string(fontSizeValue);
			ws.sendFrame(error.c_str(), error.length());
		}
	}
}

void handleText(std::string& textValue, Application& app) {
	app.logger().debug("Here's your encoded text: " + textValue);
	
	textMutex.lock();
	int comparison = strncmp(textValue.c_str(), text, textValue.length());
	if (comparison == 0) { // text is equal
		textMutex.unlock();
		return;
	}
	char* result = new char[textValue.size()];
	int outLen;
	macaron::Base64::Decode(textValue, result, outLen);
	result[outLen] = '\0';
	delete[] text;
	text = result;
	textMutex.unlock();
	
	std::string outputLogString(result);
	app.logger().debug("Here's your decoded text: {}", outputLogString);
}

std::string handleTextColor(Var& colorString, Application& app) {
	app.logger().information("Here is the color: {}", colorString);
	std::string error = "";
	std::string errorMessage = "Error: the color must be a JSON object in the form: \"font_color\": {\"R\": <value>, \"G\": <value>, \"B\": <value>, \"A\": <value>}, all values are unsigned chars";
	if (!colorString->has("R") || !colorString->has("G") || !colorString->has("B")) {
		error = errorMessage;
	}
	if (!error.empty()) {
		return error;
	}
	int R;
	int G;
	int B;
	int A;
	try {
		R = colorString->getValue<int>("R");
		G = colorString->getValue<int>("G");
		B = colorString->getValue<int>("B");
		A = colorString->getValue<int>("B");
	} catch (Exception e) {
		error = errorMessage;
	}
	if (!error.empty()) {
		return error;
	}
	
	textMutex.lock();
	textColorR = R;
	textColorG = G;
	textColorB = B;
	textColorA = A;
	textMutex.unlock();
	return error;
}

std::string handleFont(std::string& fontPathValue, Application& app) {
	app.logger().debug("Here is the font: {}", fontPathValue);

	std::string fontFullPath = "fonts/" + fontPathValue;

	std::string result = "";

	textMutex.lock();
	if (fontFullPath == fontPath) {
		textMutex.unlock();
		return result;
	}
	try {
		std::ifstream file(fontFullPath);
		if (file.is_open()) {
			file.close();
			fontPath = fontFullPath;
		} else {
			result = "Error: File: \"" + fontFullPath + "\" not found";
		}
	} catch (Exception e) {
		result = e.message();
	}
	textMutex.unlock();
	return result;
}

int handleFontSize(float fontSizeValue, Application& app) {
	if (fontSizeValue > 0) {
		textMutex.lock();
		fontSize = fontSizeValue;
		textMutex.unlock();
		return 1;
	} else {
		return 0;
	}
}