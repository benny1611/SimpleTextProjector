#pragma once
#include "Poco/ErrorHandler.h"
#include "Poco/Exception.h"
#include "Poco/Util/ServerApplication.h"

using Poco::ErrorHandler;
using Poco::Exception;
using Poco::Util::Application;

class CustomErrorHandler : public ErrorHandler {
public:
	void exception() {
		ErrorHandler::exception();
	}
	void exception(const Exception& exc) {
		Application& app = Application::instance();
		if (exc.message().rfind("Handshake failure:: An unknown error occurred while processing the certificate", 0) == 0) {
			app.logger().warning("Client handhake faliure: " + exc.message());
		}
		else {
			ErrorHandler::exception(exc);
		}
	}
	void exception(const std::exception& exc) {
		ErrorHandler::exception(exc);
	}
};