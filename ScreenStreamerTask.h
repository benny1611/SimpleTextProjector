#pragma once
#include "Poco/Task.h"
#include "Poco/Event.h"
#include "Poco/Mutex.h"
#include "Poco/JSON/Object.h"
#include "Poco/Net/WebSocket.h"
#include "Poco/Logger.h"
#include "ScreenStreamer.h"

using Poco::Event;
using Poco::Mutex;
using Poco::Logger;
using Poco::JSON::Object;
using Poco::Net::WebSocket;

class ScreenStreamerTask : public Poco::Task {
public:
	ScreenStreamerTask(Mutex* mutex, Logger* appLogger, int argc, char** argv);
	void runTask();
	void registerReceiver(WebSocket& client, Event* offerEvent);
	std::string getOffer(WebSocket& client);
	int setAnswer(WebSocket& client, Object::Ptr answer);
	void cancel(); // TODO: IMPLEMENT For cancellation to work, the task's runTask() method must periodically call isCancelled() and react accordingly. 
	Event* getStopEvent();
private:
	ScreenStreamer* screenStreamer;
	Event stopEvent;
	Mutex* mtx;
};