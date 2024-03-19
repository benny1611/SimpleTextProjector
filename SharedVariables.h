#pragma once
#ifndef SHARED_VARIABLES
#define SHARED_VARIABLES
#endif // !SHARED_VARIABLES
#include<iostream>
#include "tinythread.h"

using Mutex = tthread::mutex;

extern Mutex textMutex;
extern char* text;
extern float fontSize;
extern std::string fontPath;
