#pragma once
#include <ESPAsyncWebServer.h>
#include <WebSerial.h>
#include "MCP4725.h"
#include "task_commands.h"

// External references to DAC objects
extern MCP4725 dac1;
extern MCP4725 dac2;
extern AsyncWebSocket ws;

// WebSocket event handler function
void handleWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
    WebSerial.println("WebSocket client connected");
    
    // Send current DAC values to new client
    String dacStatus = "{\"dac1\":" + String(dac1.getValue()) + ",\"dac2\":" + String(dac2.getValue()) + "}";
    client->text(dacStatus);
  } 
  else if (type == WS_EVT_DISCONNECT) {
    Serial.printf("WebSocket client #%u disconnected\n", client->id());
    WebSerial.println("WebSocket client disconnected");
  }
  else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
      String message = "";
      for (size_t i = 0; i < len; i++) {
        message += (char)data[i];
      }
      
      Serial.println("WebSocket received: " + message);
      
      // Parse DAC control messages
      if (message.startsWith("dac1:")) {
        int value = message.substring(5).toInt();
        value = constrain(value, 0, 4095);
        dac1.setValue(value);
        float voltage = dac1.getVoltage(3.3);
        WebSerial.println("DAC #1: " + String(voltage, 2) + "V (değer: " + String(value) + ")");
        
        // Broadcast to all clients
        String response = "{\"dac1\":" + String(value) + "}";
        ws.textAll(response);
      }
      else if (message.startsWith("dac2:")) {
        int value = message.substring(5).toInt();
        value = constrain(value, 0, 4095);
        dac2.setValue(value);
        float voltage = dac2.getVoltage(3.3);
        WebSerial.println("DAC #2: " + String(voltage, 2) + "V (değer: " + String(value) + ")");
        
        // Broadcast to all clients
        String response = "{\"dac2\":" + String(value) + "}";
        ws.textAll(response);
      }
      // Handle impedance control messages - TASK-BASED SİSTEM
      else if (message.startsWith("dac1_impedance:")) {
        int mode = message.substring(15).toInt();
        mode = constrain(mode, 0, 3);
        
        // Empedans task'ini tetikle
        triggerDac1ImpedanceTask(mode);
        WebSerial.println("✅ DAC #1 Empedans task tetiklendi: Mode " + String(mode));
      }
      else if (message.startsWith("dac2_impedance:")) {
        int mode = message.substring(15).toInt();
        mode = constrain(mode, 0, 3);
        
        // Empedans task'ini tetikle
        triggerDac2ImpedanceTask(mode);
        WebSerial.println("✅ DAC #2 Empedans task tetiklendi: Mode " + String(mode));
      }
    }
  }
  else if (type == WS_EVT_ERROR) {
    Serial.printf("WebSocket error: %s\n", (char*)data);
    WebSerial.println("WebSocket error occurred");
  }
}

// JSON status endpoint handler
void handleJSONRequest(AsyncWebServerRequest *request) {
  String json = "{";
  json += "\"durum\":\"tamam\",";
  json += "\"cihaz\":\"ESP8266\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"mac\":\"" + WiFi.macAddress() + "\",";
  json += "\"sinyal_gucu\":" + String(WiFi.RSSI()) + ",";
  json += "\"calisma_suresi\":" + String(millis()) + ",";
  json += "\"bos_bellek\":" + String(ESP.getFreeHeap()) + ",";
  json += "\"dac1_deger\":" + String(dac1.getValue()) + ",";
  json += "\"dac2_deger\":" + String(dac2.getValue()) + ",";
  json += "\"dac1_empedans\":\"Güvenli Mod\",";
  json += "\"dac2_empedans\":\"Güvenli Mod\"";
  json += "}";
  request->send(200, "application/json; charset=utf-8", json);
}

// Main webpage handler
void handleWebPageRequest(AsyncWebServerRequest *request) {
  String html = String(webpage_html);
  html.replace("%IP_ADDRESS%", WiFi.localIP().toString());
  request->send(200, "text/html", html);
}

// WebSerial komut işleme sistemi - TASK-BASED
void handleWebSerialCommand(uint8_t *data, size_t len) {
  String command = "";
  for (size_t i = 0; i < len; i++) {
    command += (char)data[i];
  }
  command.trim();
  command.toLowerCase();
  
  WebSerial.println(">>> " + command);
  
  if (command == "help" || command == "?") {
    WebSerial.println("=== Kullanılabilir Komutlar (Task-based) ===");
    WebSerial.println("help         - Bu yardım menüsü");
    WebSerial.println("i2c_scan     - I2C cihaz taraması (Non-blocking)");
    WebSerial.println("status       - Sistem durumu");
    WebSerial.println("dac_info     - DAC bilgileri");
    WebSerial.println("reset        - ESP8266'yı yeniden başlat");
    WebSerial.println("free_heap    - Boş bellek miktarı");
    WebSerial.println("uptime       - Çalışma süresi");
    WebSerial.println("wifi_info    - WiFi bilgileri");
    WebSerial.println("clear        - Ekranı temizle");
    WebSerial.println("queue_status - Komut kuyruğu durumu");
  }
  else if (command == "queue_status") {
    int queueCount = (queueTail - queueHead + MAX_COMMAND_QUEUE) % MAX_COMMAND_QUEUE;
    WebSerial.println("=== Komut Kuyruğu Durumu ===");
    WebSerial.println("Kuyruktaki komut sayısı: " + String(queueCount));
    WebSerial.println("Maksimum kapasite: " + String(MAX_COMMAND_QUEUE));
  }
  else {
    // Komutu task kuyruğuna ekle
    if (addCommandToQueue(command)) {
      WebSerial.println("✅ Komut kuyruğa eklendi: " + command);
    } else {
      WebSerial.println("❌ Komut kuyruğu dolu! Tekrar deneyin.");
    }
  }
}