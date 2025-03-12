#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Net/WebSocket.h"
#include "HandlerList.h"

using Poco::Net::WebSocket;
using Poco::Util::Application;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;


void handleAuth(HTTPServerRequest& request, HTTPServerResponse& response, std::string url);
void handleCommand(std::string jsonCommand, WebSocket ws, HandlerList* handlers);