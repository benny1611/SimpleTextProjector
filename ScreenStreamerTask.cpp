#pragma once
#include "ScreenStreamerTask.h"

ScreenStreamerTask::ScreenStreamerTask(Mutex* mutex, int argc, char** argv) : Task("ScreenStreamerTask") {
	this->screenStreamer = new ScreenStreamer(this, &stopEvent, mutex);
}

void ScreenStreamerTask::runTask() {
	this->screenStreamer->startSteaming();
}

std::string ScreenStreamerTask::getOffer() {
	return this->screenStreamer->getOffer();
}

int ScreenStreamerTask::setAnswer(Object::Ptr answer) {
	return this->screenStreamer->setAnswer(answer);
}

void ScreenStreamerTask::cancel() {
	this->screenStreamer->stopStreaming();
	stopEvent.wait();
	delete screenStreamer;
}