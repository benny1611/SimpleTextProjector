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
#include "Base64.h"
#include <fstream>

using Poco::Util::Application;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPResponse;
using Poco::JSON::Parser;
using Poco::JSON::Object;
using Poco::Dynamic::Var;
using Poco::Exception;

void handleText(std::string& textValue, Application& app);
std::string handleFont(std::string& fontPathValue, Application& app);
int handleFontSize(float fontSizeValue, Application& app);
std::string handleTextColor(Var& colorString, Application& app);
bool handleStream(bool value, Application& app);

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
		if (uri == "/" || uri == "/index.html") {
			

			std::string line;

			// Read from the text file
			std::ifstream indexFileStream("www/index.html");
			if (!indexFileStream.good()) {
				indexFileStream.close();
				response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
				response.send();
			}
			response.setStatus(HTTPResponse::HTTP_OK);
			std::ostream& ostr = response.send();

			while (getline(indexFileStream, line)) {
				ostr << line;
			}

			// Close the file
			indexFileStream.close();
		}
		else {
			response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
			response.send();
		}
		
	} else {
		response.setChunkedTransferEncoding(true);
		response.setContentType("application/json");
		std::ostream& ostr = response.send();
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
	if (pObject->has("stream")) {
		bool shouldStream = pObject->getValue<bool>("stream");
		bool success = handleStream(shouldStream, app);
		if (!success) {
			std::string error = "Error: could not ";
			if (shouldStream) {
				error += "start";
			}
			else {
				error += "stop";
			}
			error += " the streaming server";
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

std::string handleTextColor(Var& colorVar, Application& app) {
	std::string error = "";
	std::string errorMessage = "Error: the color must be a JSON object in the form: \"font_color\": {\"R\": <value>, \"G\": <value>, \"B\": <value>, \"A\": <value>}, all values are unsigned chars";
	Object::Ptr colorObj = colorVar.extract<Object::Ptr>();
	if (!colorObj->has("R") || !colorObj->has("G") || !colorObj->has("B")) {
		error = errorMessage;
	}
	if (!error.empty()) {
		return error;
	}
	unsigned char R;
	unsigned char G;
	unsigned char B;
	unsigned char A;
	try {
		R = colorObj->getValue<unsigned char>("R");
		G = colorObj->getValue<unsigned char>("G");
		B = colorObj->getValue<unsigned char>("B");
		A = colorObj->getValue<unsigned char>("A");
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
	app.logger().information("Here's your color: R: " + std::to_string(R) + ", G:" + std::to_string(G) + ", B: " + std::to_string(B) + ", A:" + std::to_string(A));
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

bool handleStream(bool value, Application& app) {
	streamingServerMutex.lock();
	if (value && !isServerRunning) {
		// start the server

		// check if it's still there
		if (screenStreamer != nullptr) {
			delete screenStreamer;
		}

		screenStreamer = new ScreenStreamer();
		screenStreamer->startSteaming();

		isServerRunning = true;
	}
	else if (!value && isServerRunning) {
		// stop the server & cleanup

		if (screenStreamer != nullptr) {
			screenStreamer->stopStreaming();
			delete screenStreamer;
			screenStreamer = nullptr;
		}

		isServerRunning = false;
	}
	else {
		return false;
	}
	streamingServerMutex.unlock();

	return true;
}