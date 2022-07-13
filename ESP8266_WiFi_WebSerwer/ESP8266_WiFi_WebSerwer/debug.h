#pragma once

#include "config.h"

#include <climits>
#include <Arduino.h>

#ifdef ENABLE_DEBUG

#define debug_init()\
    DEBUG_PORT.begin(DEBUG_BAUD);\
    yield();



#define debug(...) DEBUG_PORT.print(__VA_ARGS__)
#define debugln(...) DEBUG_PORT.println(__VA_ARGS__)
#define debugf(...) DEBUG_PORT.printf_P (__VA_ARGS__)
#define debugF(...) DEBUG_PORT.print(F(__VA_ARGS__))
#define debuglnF(...) DEBUG_PORT.println(F(__VA_ARGS__))

#define db(...) DEBUG_PORT.print(__VA_ARGS__)
#define dbln(...) DEBUG_PORT.println(__VA_ARGS__)
#define dbf(...) DEBUG_PORT.printf_P (__VA_ARGS__)
#define dbF(...) DEBUG_PORT.print(F(__VA_ARGS__))
#define dblnF(...) DEBUG_PORT.println(F(__VA_ARGS__))

#define debug_available() DEBUG_PORT.available()
#define debug_read() DEBUG_PORT.read()
#define debug_peek() DEBUG_PORT.peek()

#else /* ifdef ENABLE_DEBUG */

#define debug_init() 0

#define debug(...) 0
#define debugln(...) 0
#define debugf(...) 0
#define debugF(...) 0
#define debuglnF(...) 0

#define debug_available() 0
#define debug_read() 0
#define debug_peek() 0

#endif /* ifdef ENABLE_DEBUG */