#include "wifi_manager.h"
#include <ESPAsyncWebServer.h>
#include "index.h"

const char *http_username = "stechiez";
const char *http_password = "test1234";

AsyncWebServer server(80); // Membuat instance AsyncWebServer

void setupWiFi()
{
  WiFi.mode(WIFI_STA);
  WiFiManager wm;

  bool res = wm.autoConnect("AutoConnectAP", "password");
  if (!res)
  {
    Serial.println("Gagal terhubung");
    // ESP.restart(); // Uncomment jika ingin merestart
  }
  else
  {
    Serial.println("Terhubung ke WiFi!");
  }

  // Mengatur rute server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        if (!request->authenticate(http_username, http_password))
            return request->requestAuthentication();
        request->send_P(200, "text/html", index_html, NULL); });

  // Tambahkan rute lainnya di sini

  server.begin();
}

void handleWebRequests(AsyncWebServer &server)
{
  server.onNotFound([](AsyncWebServerRequest *request)
                    { request->send(404, "text/plain", "Tidak ditemukan"); });
}
