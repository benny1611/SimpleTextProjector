#include "CustomErrorHandler.h"
#include "Poco/Util/ServerApplication.h"

using Poco::Util::Application;

void CustomErrorHandler::exception() {
	ErrorHandler::exception();
}

void CustomErrorHandler::exception(const Exception& exc) {
	if (exc.message().rfind("Handshake failure:: An unknown error occurred while processing the certificate", 0) == 0) {
		Application& app = Application::instance();
		app.logger().warning("Client handhake faliure: " + exc.message());
	}
	else {
		ErrorHandler::exception(exc);
	}
}
void CustomErrorHandler::exception(const std::exception& exc) {
	ErrorHandler::exception(exc);
}