#pragma once
#ifndef SHARED_VARIABLES
#define SHARED_VARIABLES
#endif // !SHARED_VARIABLES
#include<iostream>
#include <set>
#include <map>
#include "Poco/Net/WebSocket.h"
#include "Poco/Mutex.h"
#include "Poco/TaskManager.h"
#include "ScreenStreamer.h"
#include "ScreenStreamerTask.h"
#include "TextBoxRenderer.h"

using Poco::Net::WebSocket;
using Poco::Mutex;

extern Mutex textMutex;
extern Mutex clientSetMutex;
extern Mutex streamingServerMutex;
extern Poco::TaskManager* taskManager;
extern ScreenStreamerTask* screenStreamerTask;
extern std::set<WebSocket> clients;
extern bool isServerRunning;
extern std::map<int, TextBoxRenderer*> renderers;
extern float backgroundColorR;
extern float backgroundColorG;
extern float backgroundColorB;
extern float backgroundColorA;