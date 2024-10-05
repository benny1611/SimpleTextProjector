#pragma once
#include "Poco/Task.h"
#include "Poco/Event.h"
#include "Poco/Mutex.h"
#include "Poco/JSON/Object.h"
#include "ScreenStreamer.h"

using Poco::Event;
using Poco::Mutex;
using Poco::JSON::Object;

class ScreenStreamerTask : public Poco::Task {
public:
	ScreenStreamerTask(Mutex* mutex, int argc, char** argv);
	void runTask();
	std::string getOffer();
	int setAnswer(Object::Ptr answer);
	void cancel(); // TODO: IMPLEMENT For cancellation to work, the task's runTask() method must periodically call isCancelled() and react accordingly. 
private:
	ScreenStreamer* screenStreamer;
	Event stopEvent;
};