#pragma once
#include "Poco/ErrorHandler.h"
#include "Poco/Exception.h"

using Poco::ErrorHandler;
using Poco::Exception;

class CustomErrorHandler : public ErrorHandler {
public:
	void exception();
	void exception(const Exception& exc);
	void exception(const std::exception& exc);
};