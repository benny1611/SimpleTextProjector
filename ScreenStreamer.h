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

using Poco::Task;
using Poco::Event;
using Poco::Mutex;
using Poco::Thread;
using Poco::Exception;
using Poco::JSON::Object;

class ScreenStreamer {
private:
	bool shouldStream = false;
	const rtc::SSRC ssrc = 42;
	std::string offer;
	std::shared_ptr<rtc::Track> track;
	std::shared_ptr<rtc::PeerConnection> pc;
	Task* task;
	Mutex* mutex;
	Event* stopEvent;
public:

	ScreenStreamer(Task* tsk, Event *stop_event, Mutex* mtx);
	~ScreenStreamer();

	int startSteaming();
	void stopStreaming();
	int setAnswer(Object::Ptr answerJSON);
	bool isStreaming();
	int handle_write(uint8_t* buf, int buf_size);
	std::string getOffer();
};

#endif