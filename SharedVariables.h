#pragma once
#ifndef SHARED_VARIABLES
#define SHARED_VARIABLES
#endif // !SHARED_VARIABLES
#include<iostream>
#include <set>
#include "Poco/Net/WebSocket.h"
#include "Poco/Mutex.h"
#include "Poco/TaskManager.h"
#include "ScreenStreamer.h"
#include "ScreenStreamerTask.h"


using Poco::Net::WebSocket;
using Poco::Mutex;

extern Mutex textMutex;
extern Mutex clientSetMutex;
extern Mutex streamingServerMutex;
extern Poco::TaskManager* taskManager;
extern ScreenStreamerTask* screenStreamerTask;
extern unsigned char textColorR;
extern unsigned char textColorG;
extern unsigned char textColorB;
extern unsigned char textColorA;
extern char* text;
extern float fontSize;
extern std::string fontPath;
extern std::set<WebSocket> clients;
extern bool isServerRunning;