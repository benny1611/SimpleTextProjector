#include "HandlerList.h"
#include "Handlers.h"

HandlerList::HandlerList(Logger* consoleLogger) {
	this->consoleLogger = consoleLogger;
	registerHandler("text", handleText);
	registerHandler("font_color", handleFontColor);
	registerHandler("font_size", handleFontSize);
	registerHandler("font", handleFont);
	registerHandler("stream", handleStream);
	registerHandler("get", handleGet);
	registerHandler("set", handleSet);
	registerHandler("background_color", handleBGColor);
	registerHandler("monitor", handleMonitor);
	registerHandler("authenticate", handleAuthentication);
	registerHandler("register", handleRegistration);
}

HandlerList::~HandlerList() {

}

void HandlerList::registerHandler(std::string property, Handler handler) {
	std::pair<std::string, Handler> pair(property, handler);
	handlers.insert(pair);
}

void HandlerList::callHandler(std::string property, Object::Ptr jsonObject, WebSocket ws) {
	bool doesPropertyHaveAHandler = handlers.find(property) != handlers.end();
	if (doesPropertyHaveAHandler) {
		Handler h = handlers[property];
		h(jsonObject, ws, consoleLogger);
	}
}
