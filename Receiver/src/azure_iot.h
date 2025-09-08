#pragma once
#include <Arduino.h>

// Initialize Azure IoT Hub client (no-op if credentials are empty)
void azure_iot_begin();

// Run periodic MQTT loop and publish (call frequently from a task)
void azure_iot_loop();

// Whether Azure is configured (based on secrets)
bool azure_iot_is_enabled();

// One-shot publish of telemetry (safe to call; it rate-limits internally)
void azure_publish_telemetry();

// Send reported twin snapshot
void azure_send_reported_twin();


