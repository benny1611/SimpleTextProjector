#pragma once
#ifndef SHARED_VARIABLES
#define SHARED_VARIABLES
#endif // !SHARED_VARIABLES
#include<iostream>
#include <set>
#include "Poco/Net/WebSocket.h"
#include "Poco/Mutex.h"

using Poco::Net::WebSocket;
using Poco::Mutex;

extern Mutex textMutex;
extern Mutex clientSetMutex;
extern char* text;
extern float fontSize;
extern std::string fontPath;
extern std::set<WebSocket> clients;