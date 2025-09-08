#include "azure_iot.h"
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <az_core.h>
#include <az_iot_hub_client.h>
#include <time.h>
#include "secrets.h"
#include "pumps.h"
#include "weather.h"
#include "mbedtls/base64.h"
#include "mbedtls/md.h"
#include <ArduinoJson.h>

// Root CA for Azure IoT Hub in your region: DigiCert Global Root G2
static const char *AZURE_ROOT_CA_PEM =
"-----BEGIN CERTIFICATE-----\n"
"MIIEtjCCA56gAwIBAgIQCv1eRG9c89YADp5Gwibf9jANBgkqhkiG9w0BAQsFADBh\n"
"MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
"d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH\n"
"MjAeFw0yMjA0MjgwMDAwMDBaFw0zMjA0MjcyMzU5NTlaMEcxCzAJBgNVBAYTAlVT\n"
"MR4wHAYDVQQKExVNaWNyb3NvZnQgQ29ycG9yYXRpb24xGDAWBgNVBAMTD01TRlQg\n"
"UlMyNTYgQ0EtMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMiJV34o\n"
"eVNHI0mZGh1Rj9mdde3zSY7IhQNqAmRaTzOeRye8QsfhYFXSiMW25JddlcqaqGJ9\n"
"GEMcJPWBIBIEdNVYl1bB5KQOl+3m68p59Pu7npC74lJRY8F+p8PLKZAJjSkDD9Ex\n"
"mjHBlPcRrasgflPom3D0XB++nB1y+WLn+cB7DWLoj6qZSUDyWwnEDkkjfKee6ybx\n"
"SAXq7oORPe9o2BKfgi7dTKlOd7eKhotw96yIgMx7yigE3Q3ARS8m+BOFZ/mx150g\n"
"dKFfMcDNvSkCpxjVWnk//icrrmmEsn2xJbEuDCvtoSNvGIuCXxqhTM352HGfO2JK\n"
"AF/Kjf5OrPn2QpECAwEAAaOCAYIwggF+MBIGA1UdEwEB/wQIMAYBAf8CAQAwHQYD\n"
"VR0OBBYEFAyBfpQ5X8d3on8XFnk46DWWjn+UMB8GA1UdIwQYMBaAFE4iVCAYlebj\n"
"buYP+vq5Eu0GF485MA4GA1UdDwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcD\n"
"AQYIKwYBBQUHAwIwdgYIKwYBBQUHAQEEajBoMCQGCCsGAQUFBzABhhhodHRwOi8v\n"
"b2NzcC5kaWdpY2VydC5jb20wQAYIKwYBBQUHMAKGNGh0dHA6Ly9jYWNlcnRzLmRp\n"
"Z2ljZXJ0LmNvbS9EaWdpQ2VydEdsb2JhbFJvb3RHMi5jcnQwQgYDVR0fBDswOTA3\n"
"oDWgM4YxaHR0cDovL2NybDMuZGlnaWNlcnQuY29tL0RpZ2lDZXJ0R2xvYmFsUm9v\n"
"dEcyLmNydDA9BgNVHSAENjA0MAsGCWCGSAGG/WwCATAHBgVngQwBATAIBgZngQwB\n"
"AgEwCAYGZ4EMAQICMAgGBmeBDAECAzANBgkqhkiG9w0BAQsFAAOCAQEAdYWmf+AB\n"
"klEQShTbhGPQmH1c9BfnEgUFMJsNpzo9dvRj1Uek+L9WfI3kBQn97oUtf25BQsfc\n"
"kIIvTlE3WhA2Cg2yWLTVjH0Ny03dGsqoFYIypnuAwhOWUPHAu++vaUMcPUTUpQCb\n"
"eC1h4YW4CCSTYN37D2Q555wxnni0elPj9O0pymWS8gZnsfoKjvoYi/qDPZw1/TSR\n"
"penOgI6XjmlmPLBrk4LIw7P7PPg4uXUpCzzeybvARG/NIIkFv1eRYIbDF+bIkZbJ\n"
"QFdB9BjjlA4ukAg2YkOyCiB8eXTBi2APaceh3+uBLIgLk8ysy52g2U3gP7Q26Jlg\n"
"q/xKzj3O9hFh/g==\n"
"-----END CERTIFICATE-----\n";

static WiFiClientSecure s_tlsClient;
static PubSubClient    s_mqtt(s_tlsClient);
static az_iot_hub_client s_hub_client;
static String s_method_sub_topic;
static String s_twin_patch_sub_topic;
static String s_twin_resp_sub_topic;

static bool s_enabled = false;
static bool s_connected = false;
static unsigned long s_lastPublishMs = 0;
static bool s_timeSynced = false;
extern float maxPumpTemp; // from main.cpp
extern void saveSettings();

static String build_mqtt_username()
{
  char buff[256];
  size_t len = 0;
  az_result r = az_iot_hub_client_get_user_name(&s_hub_client, buff, sizeof(buff), &len);
  if (r != AZ_OK) return String("");
  buff[len] = 0;
  return String(buff);
}

static String url_encode(const String& s)
{
  const char* hex = "0123456789ABCDEF";
  String out;
  for (size_t i=0;i<s.length();++i){
    char c = s[i];
    if (('a'<=c && c<='z')||('A'<=c && c<='Z')||('0'<=c && c<='9')||c=='-'||c=='_'||c=='.'||c=='~'){
      out += c;
    } else {
      out += '%'; out += hex[(c>>4)&0xF]; out += hex[c&0xF];
    }
  }
  return out;
}

static bool base64_decode(const uint8_t* in, size_t in_len, uint8_t* out, size_t out_size, size_t* out_len)
{
  size_t olen = 0;
  int rc = mbedtls_base64_decode(out, out_size, &olen, in, in_len);
  if (rc != 0) return false;
  if (out_len) *out_len = olen;
  return true;
}

static bool base64_encode(const uint8_t* in, size_t in_len, char* out, size_t out_size, size_t* out_len)
{
  size_t olen = 0;
  int rc = mbedtls_base64_encode((unsigned char*)out, out_size, &olen, in, in_len);
  if (rc != 0) return false;
  if (out_len) *out_len = olen;
  return true;
}

static bool hmac_sha256(const uint8_t* key, size_t key_len, const uint8_t* msg, size_t msg_len, uint8_t out[32])
{
  const mbedtls_md_info_t* md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
  if (!md) return false;
  int rc = mbedtls_md_hmac(md, key, key_len, msg, msg_len, out);
  return rc == 0;
}

static bool ensure_time()
{
  if (s_timeSynced) return true;
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");
  for (int i=0;i<20;i++){
    time_t now = time(nullptr);
    if (now > 1600000000) { s_timeSynced = true; break; }
    delay(500);
  }
  return s_timeSynced;
}

// Build SAS token: SharedAccessSignature sr=...&sig=...&se=...
static String build_sas_token(uint32_t ttl_sec = 3600)
{
  if (!ensure_time()) return String("");
  uint64_t se = (uint64_t)time(nullptr) + ttl_sec;

  // Get string-to-sign from SDK
  uint8_t sig_buf[256];
  az_span sig_span = az_span_create(sig_buf, sizeof(sig_buf));
  az_span out_sig;
  if (az_iot_hub_client_sas_get_signature(&s_hub_client, se, sig_span, &out_sig) != AZ_OK) return String("");

  // Decode device key (base64) to bytes
  uint8_t key_bin[64]; size_t key_len = 0;
  if (!base64_decode((const uint8_t*)AZ_DEVICE_KEY, strlen(AZ_DEVICE_KEY), key_bin, sizeof(key_bin), &key_len)) return String("");

  // HMAC-SHA256 over out_sig
  uint8_t hmac[32];
  if (!hmac_sha256(key_bin, key_len, az_span_ptr(out_sig), (size_t)az_span_size(out_sig), hmac)) return String("");

  // Base64 encode HMAC result
  char sig_b64[128]; size_t sig_b64_len = 0;
  if (!base64_encode(hmac, sizeof(hmac), sig_b64, sizeof(sig_b64), &sig_b64_len)) return String("");
  az_span sig_b64_span = az_span_create((uint8_t*)sig_b64, (int32_t)sig_b64_len);

  // Build MQTT password via SDK
  char pass[512]; size_t pass_len = 0;
  if (az_iot_hub_client_sas_get_password(&s_hub_client, se, sig_b64_span, AZ_SPAN_EMPTY, pass, sizeof(pass), &pass_len) != AZ_OK) return String("");
  pass[pass_len] = 0;
  return String(pass);
}

bool azure_iot_is_enabled()
{
  return strlen(AZ_IOTHUB_HOST) > 0 && strlen(AZ_DEVICE_ID) > 0 && strlen(AZ_DEVICE_KEY) > 0;
}

void azure_iot_begin()
{
  s_enabled = azure_iot_is_enabled();
  if (!s_enabled) {
    Serial.println("[Azure] Disabled (missing secrets)");
    return;
  }

  // TLS: for bring-up, optionally skip CA validation. Only set one of these.
#if AZ_TLS_INSECURE
  s_tlsClient.setInsecure();
  // Hard reset all PKI material to avoid any residual parsing
  s_tlsClient.setCACert(nullptr);
  s_tlsClient.setCertificate(nullptr);
  s_tlsClient.setPrivateKey(nullptr);
#else
  // Secure mode: use embedded root CA PEM (see AZURE_ROOT_CA_PEM above)
  s_tlsClient.setCACert(AZURE_ROOT_CA_PEM);
#endif
  s_mqtt.setServer(AZ_IOTHUB_HOST, 8883);
  s_mqtt.setBufferSize(1024);  // Reduced from 2048
  s_mqtt.setKeepAlive(60);
  s_mqtt.setSocketTimeout(15);
  // install callback
  s_mqtt.setCallback([](char* topic, byte* payload, unsigned int length){
    // Convert inputs
    String t = String(topic);
    String body; body.reserve(length+1); for (unsigned int i=0;i<length;i++) body += (char)payload[i];

    // 1) Methods
    az_iot_hub_client_method_request method_req;
    if (az_iot_hub_client_methods_parse_received_topic(&s_hub_client,
        az_span_create((uint8_t*)t.c_str(), (int)t.length()), &method_req) == AZ_OK) {
      String method = String((const char*)az_span_ptr(method_req.name), (unsigned)az_span_size(method_req.name));
      int status = 200;
      // Handle known methods with small JSON payloads
      StaticJsonDocument<256> doc; DeserializationError de = deserializeJson(doc, body);
      if (de) { status = 400; }
      else {
        if (method == "setPumpEnabled") {
          int pump = doc["pump"] | -1; bool st = doc["state"] | false;
          if (pump>=0 && pump<4) { g_persist.enabled[pump]=st; save_pump_enabled(pump, st); }
          else status = 400;
        } else if (method == "setPumpOverride") {
          int pump = doc["pump"] | -1; bool man = doc["manual"] | false; int val = doc["value"] | 0; if (val<0) val=0; if (val>100) val=100;
          if (pump>=0 && pump<4) {
            g_persist.manual[pump]=man; g_persist.overridePct[pump]=(uint8_t)val; g_manualStartMs[pump]=man?millis():0; save_pump_override(pump, man, (uint8_t)val);
          } else status = 400;
        } else if (method == "setCutoff") {
          int pump = doc["pump"] | -1; int val = doc["value"] | -1; if (val<0) val=0; if (val>100) val=100;
          if (pump>=0 && pump<4) { g_persist.cutoffPct[pump]=(uint8_t)val; save_cutoff_pct(pump, (uint8_t)val); }
          else status = 400;
        } else if (method == "setDurations") {
          uint32_t lo = doc["lockoutMs"] | 0; uint32_t ma = doc["manualMs"] | 0;
          if (lo>=60000 && ma>=60000) { g_lockoutDurationMs=lo; g_manualDurationMs=ma; save_durations(lo, ma); } else status = 400;
        } else if (method == "setMaxTemp") {
          double v = doc["value"] | 22.0; if (v<-20.0) v=-20.0; if (v>60.0) v=60.0; maxPumpTemp=(float)v; saveSettings();
        } else if (method == "setLocation") {
          double lat = doc["lat"] | 0.0; double lon = doc["lon"] | 0.0; const char* city = doc["city"] | "";
          save_location((float)lat,(float)lon,String(city)); forceFetchWeather(); forceFetchRain();
        } else {
          status = 404;
        }
      }
      // Build response topic
      char resp_topic[256]; size_t resp_len=0;
      if (az_iot_hub_client_methods_response_get_publish_topic(&s_hub_client, method_req.request_id, status, resp_topic, sizeof(resp_topic), &resp_len) == AZ_OK) {
        resp_topic[resp_len]=0; const char* ok="{\"status\":\"ok\"}"; s_mqtt.publish(resp_topic, ok);
      }
      azure_send_reported_twin();
      return;
    }

    // 2) Twin desired/response
    az_iot_hub_client_twin_response twin_resp;
    if (az_iot_hub_client_twin_parse_received_topic(&s_hub_client,
        az_span_create((uint8_t*)t.c_str(), (int)t.length()), &twin_resp) == AZ_OK) {
      // Only process desired properties (patch or full GET response)
      StaticJsonDocument<512> doc; if (deserializeJson(doc, body)) return;
      // Apply known fields if present
      if (doc.containsKey("maxTemp")) { double v = doc["maxTemp"]; if (v<-20.0) v=-20.0; if (v>60.0) v=60.0; maxPumpTemp=(float)v; saveSettings(); }
      if (doc.containsKey("durations")) {
        JsonObject d = doc["durations"]; uint32_t lo = d["lockoutMs"] | g_lockoutDurationMs; uint32_t ma = d["manualMs"] | g_manualDurationMs;
        if (lo>=60000 && ma>=60000) { g_lockoutDurationMs=lo; g_manualDurationMs=ma; save_durations(lo,ma); }
      }
      if (doc.containsKey("cutoffPct")) { JsonArray a = doc["cutoffPct"]; for (int i=0;i<4 && i<(int)a.size();++i){ int v=a[i] | (int)g_persist.cutoffPct[i]; if(v<0)v=0; if(v>100)v=100; g_persist.cutoffPct[i]=(uint8_t)v; save_cutoff_pct(i,(uint8_t)v);} }
      if (doc.containsKey("enabled")) { JsonArray a = doc["enabled"]; for (int i=0;i<4 && i<(int)a.size();++i){ bool v=a[i] | g_persist.enabled[i]; g_persist.enabled[i]=v; save_pump_enabled(i,v);} }
      if (doc.containsKey("manual")) { JsonArray a = doc["manual"]; for (int i=0;i<4 && i<(int)a.size();++i){ bool v=a[i] | g_persist.manual[i]; g_persist.manual[i]=v; save_pump_override(i,v,g_persist.overridePct[i]); g_manualStartMs[i]=v?millis():0; } }
      if (doc.containsKey("override")) { JsonArray a = doc["override"]; for (int i=0;i<4 && i<(int)a.size();++i){ int v=a[i] | (int)g_persist.overridePct[i]; if(v<0)v=0; if(v>100)v=100; g_persist.overridePct[i]=(uint8_t)v; save_pump_override(i,g_persist.manual[i],(uint8_t)v);} }
      if (doc.containsKey("location")) { JsonObject o=doc["location"]; double lat=o["lat"]|0.0; double lon=o["lon"]|0.0; const char* city=o["city"]|""; save_location((float)lat,(float)lon,String(city)); forceFetchWeather(); forceFetchRain(); }
      azure_send_reported_twin();
      return;
    }
  });
  // Initialize hub client for topic/username building
  az_span hostname = az_span_create((uint8_t*)AZ_IOTHUB_HOST, strlen(AZ_IOTHUB_HOST));
  az_span deviceid = az_span_create((uint8_t*)AZ_DEVICE_ID, strlen(AZ_DEVICE_ID));
  az_result rc_init = az_iot_hub_client_init(&s_hub_client, hostname, deviceid, NULL);
  if (rc_init != AZ_OK) {
    Serial.printf("[Azure] hub_client_init failed: 0x%08X\n", (unsigned int)rc_init);
    s_enabled = false;
    return;
  }
  Serial.println("[Azure] Initialized");
}

static void ensure_connected()
{
  if (!s_enabled) return;
  if (s_mqtt.connected()) { s_connected = true; return; }
  static unsigned long lastAttempt = 0;
  unsigned long now = millis();
  if (now - lastAttempt < 3000) return;
  lastAttempt = now;
  String username = build_mqtt_username();
  String password = build_sas_token();
  if (username.length() == 0 || password.length() == 0) return;
  // Client ID must be device id
  s_connected = s_mqtt.connect(AZ_DEVICE_ID, username.c_str(), password.c_str());
  if (s_connected) {
    // Send twin snapshot on connect
    azure_send_reported_twin();
    // Subscribe to methods and twin using well-known filters
    s_mqtt.subscribe("$iothub/methods/POST/#");
    s_mqtt.subscribe("$iothub/twin/PATCH/properties/desired/#");
    s_mqtt.subscribe("$iothub/twin/res/#");
  } else {
  }
}

void azure_publish_telemetry()
{
  if (!s_enabled || !s_connected) return;
  unsigned long now = millis();
  if (now - s_lastPublishMs < 5000) return; // 5s rate limit
  s_lastPublishMs = now;

  // Use pre-allocated buffer instead of String concatenation
  char payload[1024];
  int offset = 0;
  
  // Timestamps
  time_t tnow = time(nullptr);
  if (tnow > 1600000000) {
    struct tm tmv; gmtime_r(&tnow, &tmv);
    offset += snprintf(payload + offset, sizeof(payload) - offset,
      "{\"ts\":\"%04d-%02d-%02dT%02d:%02d:%02dZ\",\"tsMs\":%llu",
      tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
      tmv.tm_hour, tmv.tm_min, tmv.tm_sec, (uint64_t)tnow * 1000ULL);
  } else {
    offset += snprintf(payload + offset, sizeof(payload) - offset,
      "{\"tsMs\":%lu", (uint32_t)millis());
  }
  
  // Sensor data
  offset += snprintf(payload + offset, sizeof(payload) - offset,
    ",\"tAir\":%s,\"hAir\":%s,\"vBus\":%s,\"i_mA\":%s,\"p_mW\":%s",
    isnan(g_tAir) ? "null" : String(g_tAir,1).c_str(),
    isnan(g_hAir) ? "null" : String(g_hAir,0).c_str(),
    isnan(g_vBus) ? "null" : String(g_vBus,2).c_str(),
    isnan(g_i_mA) ? "null" : String(g_i_mA,1).c_str(),
    isnan(g_p_mW) ? "null" : String(g_p_mW,1).c_str());
  
  // Soil sensors
  offset += snprintf(payload + offset, sizeof(payload) - offset, ",\"soil\":[");
  for (int i = 0; i < 10; i++) {
    offset += snprintf(payload + offset, sizeof(payload) - offset, 
      "%d%s", g_soil[i], (i < 9) ? "," : "");
  }
  
  // Weather
  offset += snprintf(payload + offset, sizeof(payload) - offset,
    "],\"weatherT\":%s,\"weatherH\":%s",
    isnan(w_temp) ? "null" : String(w_temp,1).c_str(),
    isnan(w_hum) ? "null" : String(w_hum,0).c_str());
  
  // Pumps
  offset += snprintf(payload + offset, sizeof(payload) - offset, ",\"pumps\":[");
  unsigned long nowMs2 = millis();
  for (int i = 0; i < 4; i++) {
    int powerPct = g_persist.enabled[i]
      ? (g_persist.manual[i] ? (int)g_persist.overridePct[i] : pwmToPercent(g_pumpPWM[i]))
      : 0;
    unsigned long rem = 0;
    if (g_lockoutUntilMs[i] && (int32_t)(g_lockoutUntilMs[i] - nowMs2) > 0) 
      rem = (unsigned long)(g_lockoutUntilMs[i] - nowMs2);
    
    offset += snprintf(payload + offset, sizeof(payload) - offset,
      "{\"enabled\":%d,\"manual\":%d,\"override\":%d,\"power\":%d,\"cutoff\":%d,\"lockoutMs\":%lu,\"groupAvg\":%d}%s",
      g_persist.enabled[i] ? 1 : 0,
      g_persist.manual[i] ? 1 : 0,
      (int)g_persist.overridePct[i],
      powerPct,
      (int)g_persist.cutoffPct[i],
      rem,
      (int)groupAverageMoisture(i),
      (i < 3) ? "," : "");
  }
  
  // Settings and close
  offset += snprintf(payload + offset, sizeof(payload) - offset,
    "],\"durations\":{\"lockoutMs\":%lu,\"manualMs\":%lu},\"rainNext24h\":%s,\"maxTemp\":%.1f}",
    (unsigned long)g_lockoutDurationMs,
    (unsigned long)g_manualDurationMs,
    g_rainNext24h ? "true" : "false",
    maxPumpTemp);

  // Telemetry topic
  char topic[256]; size_t topic_len = 0;
  az_result rc_topic = az_iot_hub_client_telemetry_get_publish_topic(&s_hub_client, NULL, topic, sizeof(topic), &topic_len);
  if (rc_topic != AZ_OK) {
    return;
  }
  topic[topic_len] = 0;

  s_mqtt.publish(topic, payload);
}

void azure_iot_loop()
{
  if (!s_enabled) return;
  ensure_connected();
  s_mqtt.loop();
}

// Reported twin with key settings and state
void azure_send_reported_twin()
{
  if (!s_enabled || !s_connected) return;
  char topic[256]; size_t topic_len = 0;
  char rid[16]; size_t rid_len = (size_t)snprintf(rid, sizeof(rid), "%lu", (unsigned long)millis());
  az_span rid_span = az_span_create((uint8_t*)rid, (int32_t)rid_len);
  if (az_iot_hub_client_properties_get_reported_publish_topic(&s_hub_client, rid_span, topic, sizeof(topic), &topic_len) != AZ_OK) return;
  topic[topic_len] = 0;

  // Use pre-allocated buffer instead of String concatenation
  char body[512];
  int offset = snprintf(body, sizeof(body), "{\"maxTemp\":%.1f", maxPumpTemp);
  
  offset += snprintf(body + offset, sizeof(body) - offset, 
    ",\"durations\":{\"lockoutMs\":%lu,\"manualMs\":%lu}", 
    (unsigned long)g_lockoutDurationMs, (unsigned long)g_manualDurationMs);
  
  offset += snprintf(body + offset, sizeof(body) - offset, ",\"cutoffPct\":[");
  for (int i = 0; i < 4; i++) {
    offset += snprintf(body + offset, sizeof(body) - offset, 
      "%d%s", (int)g_persist.cutoffPct[i], (i < 3) ? "," : "");
  }
  
  offset += snprintf(body + offset, sizeof(body) - offset, "],\"enabled\":[");
  for (int i = 0; i < 4; i++) {
    offset += snprintf(body + offset, sizeof(body) - offset, 
      "%d%s", (g_persist.enabled[i] ? 1 : 0), (i < 3) ? "," : "");
  }
  
  offset += snprintf(body + offset, sizeof(body) - offset, "],\"manual\":[");
  for (int i = 0; i < 4; i++) {
    offset += snprintf(body + offset, sizeof(body) - offset, 
      "%d%s", (g_persist.manual[i] ? 1 : 0), (i < 3) ? "," : "");
  }
  
  offset += snprintf(body + offset, sizeof(body) - offset, "]}");

  s_mqtt.publish(topic, body);
} 
