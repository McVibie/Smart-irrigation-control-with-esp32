#pragma once
#include <WebServer.h>
#include <memory>
#include "secrets.h"          // defines WEB_USER, WEB_PASS
#include "mbedtls/base64.h"   // base64 decoder

// Simple HTTP Basic Auth helper.
// Returns true if authorized (or if no creds set), otherwise sends 401 and returns false.
inline bool checkAuth(WebServer& server)
{
  // If no credentials configured, allow everything (handy for local dev).
  #ifndef WEB_USER
  #define WEB_USER ""
  #endif
  #ifndef WEB_PASS
  #define WEB_PASS ""
  #endif

  if (String(WEB_USER).length() == 0 && String(WEB_PASS).length() == 0) {
    return true;
  }

  if (!server.hasHeader("Authorization")) {
    server.sendHeader("WWW-Authenticate", "Basic realm=\"LoRa Soil\"");
    server.send(401, "text/plain", "Auth required");
    return false;
  }

  String h = server.header("Authorization");
  if (!h.startsWith("Basic ")) {
    server.send(401, "text/plain", "Bad auth header");
    return false;
  }

  // Base64 decode the part after "Basic "
  String b64 = h.substring(6);
  size_t out_len = 0;

  // First call to get the needed buffer size
  int rc = mbedtls_base64_decode(nullptr, 0, &out_len,
                                 reinterpret_cast<const unsigned char*>(b64.c_str()),
                                 b64.length());
  if (rc != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL && rc != 0) {
    server.send(401, "text/plain", "Auth decode error");
    return false;
  }

  std::unique_ptr<unsigned char[]> buf(new unsigned char[out_len + 1]);
  rc = mbedtls_base64_decode(buf.get(), out_len, &out_len,
                             reinterpret_cast<const unsigned char*>(b64.c_str()),
                             b64.length());
  if (rc != 0) {
    server.send(401, "text/plain", "Auth decode error");
    return false;
  }
  buf[out_len] = 0;
  String userpass = String(reinterpret_cast<char*>(buf.get())); // "user:pass"

  String expected = String(WEB_USER) + ":" + String(WEB_PASS);
  if (userpass == expected) return true;

  server.sendHeader("WWW-Authenticate", "Basic realm=\"LoRa Soil\"");
  server.send(401, "text/plain", "Unauthorized");
  return false;
}