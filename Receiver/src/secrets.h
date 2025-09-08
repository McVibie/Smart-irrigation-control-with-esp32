#pragma once
// COPY THIS FILE TO secrets.h (and add secrets.h to .gitignore)

// Wi-Fi
#define WIFI_SSID "Wifi ssid"
#define WIFI_PASS "wifi pass"

// Web Basic Auth (protects /setPump and /setPumpOverride)
#define WEB_USER "admin"
#define WEB_PASS "admin"

// OpenWeatherMap
#define OWM_API_KEY "api key"

// Azure IoT Hub (fill in only if using cloud; leave empty to disable)
#ifndef AZ_IOTHUB_HOST
#define AZ_IOTHUB_HOST "device"
#endif
#ifndef AZ_DEVICE_ID
#define AZ_DEVICE_ID  "device id"
#endif
#ifndef AZ_DEVICE_KEY
#define AZ_DEVICE_KEY "device key="
#endif

// For first connection bring-up only: set to 1 to skip TLS cert validation.
// Set to 0 and use the real DigiCert Global Root G2 CA once verified.
#ifndef AZ_TLS_INSECURE
#define AZ_TLS_INSECURE 0
#endif

// Email Settings for Gmail SMTP
// Gmail requires App Password for IoT devices (2FA must be enabled)
#define EMAIL_SMTP_SERVER "smtp.gmail.com"
#define EMAIL_SMTP_PORT 587
#define EMAIL_SENDER "email"
#define EMAIL_PASSWORD "email password key"
#define EMAIL_RECIPIENT "receiver"

