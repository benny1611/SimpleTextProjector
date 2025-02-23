#pragma once
#include "SharedVariables.h"
#include "Poco/Base64Decoder.h"

using Poco::Base64Decoder;

void handleText(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	std::string textValue = jsonObject->getValue<std::string>("text");
	consoleLogger->debug("Here's your encoded text: " + textValue);

	std::istringstream iss(textValue);
	std::ostringstream oss;

	Base64Decoder decoder(iss);
	oss << decoder.rdbuf();

	std::string decoded = oss.str();

	textMutex.lock();
	if (decoded != *text) {
		delete text;
		text = new std::string(decoded);

		consoleLogger->debug("Here's your decoded text: {}", *text);
	}
	textMutex.unlock();
}

void handleFontColor(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	std::string error = "";
	std::string errorMessage = "{\"error\": true, \"message\": \"Error: the color must be a JSON object in the form: font_color: R: <value>, G: <value>, B: <value>, A: <value>, all values are unsigned chars\"} ";
	Object::Ptr colorObj = jsonObject->get("font_color").extract<Object::Ptr>();
	if (!colorObj->has("R") || !colorObj->has("G") || !colorObj->has("B") || !colorObj->has("A")) {
		error = errorMessage;
	}
	if (!error.empty()) {
		ws.sendFrame(error.c_str(), error.length());
		return;
	}
	float R;
	float G;
	float B;
	float A;
	try {
		R = colorObj->getValue<float>("R");
		G = colorObj->getValue<float>("G");
		B = colorObj->getValue<float>("B");
		A = colorObj->getValue<float>("A");
	}
	catch (Exception e) {
		error = "{\"error\": true, \"message\": \"" + errorMessage + "\"}";
	}
	
	if (R < 0 || R > 1.0 || G < 0 || G > 1.0 || B < 0 || B > 1.0 || A < 0 || A > 1.0) {
		error = "{\"error\": true, \"message\": \"Values R, G, B and A must be between a float between 0.0 and 1.0\"}";
	}

	if (!error.empty()) {
		ws.sendFrame(error.c_str(), error.length());
		return;
	}

	textMutex.lock();
	textColorR = R;
	textColorG = G;
	textColorB = B;
	textColorA = A;
	textMutex.unlock();
	consoleLogger->information("Here's your color: R: " + std::to_string(R) + ", G:" + std::to_string(G) + ", B: " + std::to_string(B) + ", A:" + std::to_string(A));
}

void handleFontSize(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	float fontSizeValue = jsonObject->getValue<float>("font_size");
	if (fontSizeValue > 0) {
		textMutex.lock();
		fontSize = fontSizeValue;
		textMutex.unlock();
	} else {
		std::string error = "{\"error\": true, \"message\": \"Error: could not set font size to: " + std::to_string(fontSizeValue) + "\"}";
		error = error ;
		ws.sendFrame(error.c_str(), error.length());
	}
}

void handleFont(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	std::string fontPathValue = jsonObject->getValue<std::string>("font");
	consoleLogger->debug("Here is the font: {}", fontPathValue);

	std::string fontFullPath = "fonts/" + fontPathValue;

	std::string result = "";

	textMutex.lock();
	if (fontFullPath != fontPath) {
		try {
			std::ifstream file(fontFullPath);
			if (file.is_open()) {
				file.close();
				fontPath = fontFullPath;
			}
			else {
				std::string error = "{\"error\": true, \"message\": \"Error file " + fontFullPath + " not found\"}";
				ws.sendFrame(error.c_str(), error.length());
			}
		}
		catch (Exception e) {
			std::string error = "{\"error\": true, \"message\": \"" + e.message() + "\"}";
			ws.sendFrame(error.c_str(), error.length());
		}
	}
	textMutex.unlock();
}

void handleStream(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	bool shouldStream = jsonObject->getValue<bool>("stream");

	streamingServerMutex.lock();
	if (shouldStream && !isServerRunning) {
		// start the server

		screenStreamerTask = new ScreenStreamerTask(&streamingServerMutex, consoleLogger, 0, 0);
		taskManager->start(screenStreamerTask);

		isServerRunning = true;
		streamingServerMutex.unlock();
	}
	else if (!shouldStream && isServerRunning) {
		// stop the server & cleanup

		screenStreamerTask->cancel();
		streamingServerMutex.unlock();
		screenStreamerTask->getStopEvent()->wait();

		isServerRunning = false;
	}
	else {
		streamingServerMutex.unlock();

		std::string error = "{\"error\": true, \"message\": \"Error: could not ";
		if (shouldStream) {
			error += "start the streaming server, probably it's already running\"}";
		}
		else {
			error += "stop the streaming server, probably it's already stopped\"}";
		}
		ws.sendFrame(error.c_str(), error.length());
	}
}

void handleGet(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	std::string what = jsonObject->getValue<std::string>("get");
	if (what == "stream") {
		streamingServerMutex.lock();
		std::string isStreamingJSON = "{\"isStreaming\":";
		if (isServerRunning) {
			streamingServerMutex.unlock();
			Event waitForOfferEvent;
			screenStreamerTask->registerReceiver(ws, &waitForOfferEvent);
			waitForOfferEvent.wait();
			isStreamingJSON += "true, \"offer\": ";
			isStreamingJSON += screenStreamerTask->getOffer(ws);
		}
		else {
			streamingServerMutex.unlock();
			isStreamingJSON += "false";
		}
		isStreamingJSON += "}";

		ws.sendFrame(isStreamingJSON.c_str(), isStreamingJSON.length());
	}
	else if (what == "ping") {
		std::string pong = "{\"pong\": true}";
		ws.sendFrame(pong.c_str(), pong.length());
	}
}

void handleSet(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger) {
	std::string what = jsonObject->getValue<std::string>("set");
	if (what == "answer") {
		if (jsonObject->has("answer")) {
			Object::Ptr answer = jsonObject->get("answer").extract<Object::Ptr>();
			streamingServerMutex.lock();
			if (isServerRunning) {
				int result = screenStreamerTask->setAnswer(ws, answer);
				if (result < 0) {
					consoleLogger->debug("Could not connect to the screen streamer");
				}
			}
			streamingServerMutex.unlock();
		}
	}
}