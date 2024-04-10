#pragma once

#include "Poco/Net/WebSocket.h"
#include "Poco/Net/NetException.h"
#include "Poco/Exception.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequestImpl.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Util/ServerApplication.h"
#include "CommandRequestHandler.h"
#include "SharedVariables.h"

using Poco::Net::WebSocket;
using Poco::Net::WebSocketException;
using Poco::Net::HTTPResponse;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Util::Application;
using Poco::Exception;


class WebSocketRequestHandler : public HTTPRequestHandler {
	/// Handle a WebSocket connection.
private:
	WebSocket* ws;
	void deleteWS() {
		clientSetMutex.lock();
		if (clients.count(*ws)) {
			clients.erase(*ws);
		}
		clientSetMutex.unlock();
		ws->close();
		free(ws);
		Application& app = Application::instance();
		app.logger().information("WebSocket connection closed after timeout");
	}
public:
	void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
		Application& app = Application::instance();
		try {
			ws = new WebSocket(request, response);
			app.logger().information("WebSocket connection established.");

			char buffer[1024];
			int flags;
			int n;
			clientSetMutex.lock();
			if (!clients.count(*ws)) {
				clients.insert(*ws);
			}
			clientSetMutex.unlock();

			do {
				n = ws->receiveFrame(buffer, sizeof(buffer), flags);
				app.logger().information(Poco::format("Frame received (length=%d, flags=0x%x).", n, unsigned(flags)));
				if (n > 0 && (flags & WebSocket::FRAME_OP_BITMASK) != WebSocket::FRAME_OP_CLOSE) {
					buffer[n] = '\0';
					std::string jsonCommand = buffer;
					handleCommand(jsonCommand, *ws);
				}
			} while (n > 0 && (flags & WebSocket::FRAME_OP_BITMASK) != WebSocket::FRAME_OP_CLOSE);

			deleteWS();

			app.logger().information("WebSocket connection closed.");
		} catch (WebSocketException& exc) {

			app.logger().log(exc);
			switch (exc.code())
			{
			case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
				response.set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);
				// fallthrough
			case WebSocket::WS_ERR_NO_HANDSHAKE:
			case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
			case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
				response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
				response.setContentLength(0);
				response.send();
				break;
			}
		} catch (Exception& exp) {
			deleteWS();
		}
	}
};