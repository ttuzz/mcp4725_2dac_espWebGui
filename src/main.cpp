#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESPConnect.h>
#include <WebSerial.h>
#include <AsyncElegantOTA.h>
#include "i2c_scan.h"
#include "MCP4725.h"
#include "webpages.h"
#include "web_handlers.h"
#include "task_commands.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

// MCP4725 DAC'ler - bulunan adresler için
MCP4725 dac1(0x60);  // 0x60 adresindeki MCP4725
MCP4725 dac2(0x61);  // 0x61 adresindeki MCP4725

void setup() {
  Serial.begin(115200);
  Serial.println();
  
  Serial.println("=== ESP8266 Web Server Starting ===");
  Serial.println("Features: ESPConnect + WebSerial + OTA + I2C Scanner");
  
  // I2C başlat
  i2c_init();
  Serial.println("I2C initialized (SDA=D2/GPIO4, SCL=D1/GPIO5)");
  
  // MCP4725 DAC'leri başlat
  Serial.println("\n=== MCP4725 DAC Initialization ===");
  if (dac1.begin()) {
    Serial.println("✓ MCP4725 #1 (0x60) connected");
    dac1.setValue(0);  // 0V'dan başlat
    WebSerial.println("MCP4725 #1 (0x60) hazır");
  } else {
    Serial.println("✗ MCP4725 #1 (0x60) not found");
    WebSerial.println("MCP4725 #1 (0x60) bulunamadı");
  }
  
  if (dac2.begin()) {
    Serial.println("✓ MCP4725 #2 (0x61) connected");
    dac2.setValue(0);  // 0V'dan başlat
    WebSerial.println("MCP4725 #2 (0x61) hazır");
  } else {
    Serial.println("✗ MCP4725 #2 (0x61) not found");
    WebSerial.println("MCP4725 #2 (0x61) bulunamadı");
  }

  /*
    AutoConnect AP
    Configure SSID and password for Captive Portal
  */
  ESPConnect.autoConnect("ESP8266-Config");

  /* 
    Begin connecting to previous WiFi
    or start autoConnect AP if unable to connect
  */
  if(ESPConnect.begin(&server)){
    Serial.println("Connected to WiFi");
    Serial.println("IP Address: " + WiFi.localIP().toString());
    Serial.println("MAC Address: " + WiFi.macAddress());
  } else {
    Serial.println("Failed to connect to WiFi");
  }

  // Web sayfaları - temiz ve basit!
  server.on("/", HTTP_GET, handleWebPageRequest);
  server.on("/json", HTTP_GET, handleJSONRequest);

  // WebSocket event handler
  ws.onEvent(handleWebSocketEvent);
  
  server.addHandler(&ws);

  // WebSerial başlat
  WebSerial.begin(&server);
  WebSerial.onMessage(handleWebSerialCommand);
  
  // OTA başlat
  AsyncElegantOTA.begin(&server);
  
  // HTTP Server başlat
  server.begin();
  
  Serial.println("✓ HTTP Server started on port 80");
  Serial.println("✓ WebSerial available at /webserial");
  Serial.println("✓ OTA available at /update");
  Serial.println("\n=== System Ready ===\n");
  
  // Task Scheduler'ı initialize et
  initTaskScheduler();
  
  // WebSerial karşılama mesajı
  delay(500); // WebSerial tamamen başlayana kadar bekle
  WebSerial.println("=== ESP8266 WebSerial Terminal ===");
  WebSerial.println("MCP4725 DAC Kontrolü v0.4.0 (Thread-Safe + TaskScheduler)");
  WebSerial.println("Komutlar için 'help' yazın");
  WebSerial.println("⚡ Thread-safe empedans kontrolü aktif");
  WebSerial.println("🔄 Non-blocking TaskScheduler aktif");
  WebSerial.println("");
}

void loop() {
  // CRITICAL: Watchdog besle - sistem çökmesini önle
  ESP.wdtFeed();
  yield();
  
  // Temel servis loop'ları - NON-BLOCKING
  WebSerial.loop();
  AsyncElegantOTA.loop();
  
  // WebSocket cleanup - zamanlı işlem
  static unsigned long lastWSCleanup = 0;
  if (millis() - lastWSCleanup > 10000) {
    ws.cleanupClients();
    lastWSCleanup = millis();
  }
  
  // Task Scheduler'ı çalıştır - TÜM İŞLEMLER TASK-BASED
  runTaskScheduler();
  
  // Ana loop frekansı - CPU'ya nefes ver
  delay(10);
}
