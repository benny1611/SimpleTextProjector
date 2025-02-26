#pragma once
#include "ScreenStreamerTask.h"

ScreenStreamerTask::ScreenStreamerTask(Mutex* mutex, Logger* appLogger, int argc, char** argv) : Task("ScreenStreamerTask") {
	this->screenStreamer = new ScreenStreamer(this, &stopEvent, mutex, appLogger);
	this->mtx = mutex;
}

void ScreenStreamerTask::runTask() {
	this->screenStreamer->startSteaming();
}

int ScreenStreamerTask::registerReceiver(WebSocket& client, Event* offerEvent) {
	return this->screenStreamer->registerReceiver(client, offerEvent);
}

std::string ScreenStreamerTask::getOffer(WebSocket& client) {
	return this->screenStreamer->getOffer(client);
}

int ScreenStreamerTask::setAnswer(WebSocket& client, Object::Ptr answer) {
	return this->screenStreamer->setAnswer(client, answer);
}

void ScreenStreamerTask::cancel() {
	this->screenStreamer->stopStreaming();
}

Event* ScreenStreamerTask::getStopEvent() {
	return &this->stopEvent;
}