#pragma once
#ifndef SCREENRECORDER_H
#define SCREENRECORDER_H

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <cstring>
#include <math.h>
#include <string.h>
#include <sstream>
#include <set>

#define __STDC_CONSTANT_MACROS

//FFMPEG LIBRARIES
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"

#include "libavdevice/avdevice.h"

#include "libavfilter/avfilter.h"
#include "libavfilter/avfilter_internal.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"

#include "libavformat/avformat.h"
#include "libavformat/avio.h"

	// libav resample

#include "libavutil/opt.h"
#include "libavutil/common.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
#include "libavutil/pixdesc.h"
#include "libavutil/file.h"

// lib swresample

#include "libswscale/swscale.h"

}

#include "rtc/rtc.hpp"

#include "Poco/JSON/Object.h"
#include "Poco/JSON/Stringifier.h"
#include "Poco/Task.h"
#include "Poco/Event.h"
#include "Poco/Mutex.h"
#include "Poco/Thread.h"
#include "Poco/Exception.h"
#include "Poco/Logger.h"
#include "Poco/Net/WebSocket.h"

using Poco::Task;
using Poco::Event;
using Poco::Mutex;
using Poco::Thread;
using Poco::Exception;
using Poco::Logger;
using Poco::JSON::Object;
using Poco::Net::WebSocket;

struct Receiver {
	std::shared_ptr<rtc::PeerConnection> conn;
	std::shared_ptr<rtc::Track> track;
	WebSocket* client;
	std::string offer;
	bool isConnected = false;
};

class ScreenStreamer {
private:
	bool shouldStream = false;
	const rtc::SSRC ssrc = 42;
	Task* task;
	Mutex* mutex;
	Event* stopEvent;
	Logger* appLogger;
	std::set <std::shared_ptr<Receiver>> receivers;
	void getReceiver(const WebSocket& client, std::shared_ptr<Receiver>& recv);
	std::string peerStateToString(rtc::PeerConnection::State state);
	std::string gatheringStateToString(rtc::PeerConnection::GatheringState state);
public:

	ScreenStreamer(Task* tsk, Event *stop_event, Mutex* mtx, Logger* logger);
	~ScreenStreamer();

	int startSteaming();
	void stopStreaming();
	bool isStreaming();
	int handle_write(uint8_t* buf, int buf_size);

	void registerReceiver(WebSocket& client, Event* offerEvent);
	std::string getOffer(WebSocket& client);
	int setAnswer(WebSocket& client, Object::Ptr answerJSON);
};

#endif