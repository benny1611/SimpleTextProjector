#include "CommandRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Util/ServerApplication.h"

using Poco::Util::Application;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;

void handle(HTTPServerRequest& request, HTTPServerResponse& response) {
	Application& app = Application::instance();
	app.logger().information("Request from: " + request.clientAddress().toString());

	response.setChunkedTransferEncoding(true);
	response.setContentType("application/json");
	std::ostream& ostr = response.send();
	std::string method = request.getMethod();

	if (method == "POST") {
		ostr << "{\"success\": true}";
	}
	else {
		ostr << "{\"success\": false}";
	}

	
}