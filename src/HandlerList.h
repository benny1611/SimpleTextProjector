#pragma once

#include <unordered_map>
#include <string>
#include <functional>

#include "Poco/JSON/Object.h"
#include "Poco/Logger.h"
#include "Poco/Net/WebSocket.h"

using Poco::Logger;
using Poco::JSON::Object;
using Poco::Net::WebSocket;

using Handler = std::function<void(Object::Ptr jsonObject, WebSocket ws, Logger* consoleLogger)>;

class HandlerList {
public:
	HandlerList(Logger* consoleLogger);
	~HandlerList();

	void callHandler(std::string property, Object::Ptr jsonObject, WebSocket ws);
private:
	Logger* consoleLogger;
	std::unordered_map<std::string, Handler> handlers;
	void registerHandler(std::string, Handler);
};
