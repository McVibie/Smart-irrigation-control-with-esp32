// web_server.h
#pragma once
#include <WebServer.h>
#include "pumps.h"

extern WebServer server;
extern String g_sensorNames[10];

void web_setup();
void web_poll();