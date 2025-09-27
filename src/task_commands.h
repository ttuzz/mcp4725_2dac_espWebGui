#pragma once

#include <TaskScheduler.h>
#include <WebSerial.h>
#include "MCP4725.h"
#include "Wire.h"

// External references
extern MCP4725 dac1;
extern MCP4725 dac2;
extern AsyncWebSocket ws;

// Task Scheduler
Scheduler taskScheduler;

// Command queue yapısı
struct CommandTask {
  String command;
  String params;
  bool active;
};

#define MAX_COMMAND_QUEUE 5
CommandTask commandQueue[MAX_COMMAND_QUEUE];
int queueHead = 0;
int queueTail = 0;

// Empedans task değişkenleri
volatile int dac1_target_impedance = 0;
volatile int dac2_target_impedance = 0;

// Task fonksiyonları
void taskI2CScan();
void taskDacInfo();
void taskSystemStatus();
void taskWifiInfo();
void taskProcessCommand();
void taskSetDac1Impedance();
void taskSetDac2Impedance();

// Task nesneleri
Task tI2CScan(200, TASK_ONCE, &taskI2CScan);        // I2C tarama task'i
Task tDacInfo(100, TASK_ONCE, &taskDacInfo);        // DAC bilgi task'i  
Task tSystemStatus(100, TASK_ONCE, &taskSystemStatus); // Sistem durum task'i
Task tWifiInfo(100, TASK_ONCE, &taskWifiInfo);      // WiFi bilgi task'i
Task tProcessCommand(50, TASK_FOREVER, &taskProcessCommand); // Komut işleyici
Task tSetDac1Impedance(100, TASK_ONCE, &taskSetDac1Impedance); // DAC1 empedans task'i
Task tSetDac2Impedance(100, TASK_ONCE, &taskSetDac2Impedance); // DAC2 empedans task'i

// Komut kuyruğuna ekleme
bool addCommandToQueue(String cmd, String params = "") {
  int nextTail = (queueTail + 1) % MAX_COMMAND_QUEUE;
  if (nextTail == queueHead) {
    return false; // Queue full
  }
  
  commandQueue[queueTail].command = cmd;
  commandQueue[queueTail].params = params;
  commandQueue[queueTail].active = true;
  queueTail = nextTail;
  
  return true;
}

// Komut kuyruğundan alma
CommandTask getCommandFromQueue() {
  CommandTask cmd = {"", "", false};
  
  if (queueHead != queueTail) {
    cmd = commandQueue[queueHead];
    commandQueue[queueHead].active = false;
    queueHead = (queueHead + 1) % MAX_COMMAND_QUEUE;
  }
  
  return cmd;
}

// Task Scheduler başlatma
void initTaskScheduler() {
  taskScheduler.init();
  
  // Task'leri scheduler'a ekle
  taskScheduler.addTask(tI2CScan);
  taskScheduler.addTask(tDacInfo);
  taskScheduler.addTask(tSystemStatus);
  taskScheduler.addTask(tWifiInfo);
  taskScheduler.addTask(tProcessCommand);
  taskScheduler.addTask(tSetDac1Impedance);
  taskScheduler.addTask(tSetDac2Impedance);
  
  // Komut işleyici task'ini başlat
  tProcessCommand.enable();
  
  WebSerial.println("✅ Task Scheduler başlatıldı");
}

// Empedans task'larını tetikleme fonksiyonları
void triggerDac1ImpedanceTask(int impedance) {
  dac1_target_impedance = impedance;
  tSetDac1Impedance.restartDelayed();
}

void triggerDac2ImpedanceTask(int impedance) {
  dac2_target_impedance = impedance;
  tSetDac2Impedance.restartDelayed();
}

// WebSocket ile DAC durumunu broadcast et
void broadcastDacStatus() {
  String response = "{";
  response += "\"dac1\":" + String(dac1.getValue()) + ",";
  response += "\"dac2\":" + String(dac2.getValue()) + ",";
  response += "\"dac1_impedance\":" + String(dac1.readPowerDownModeDAC()) + ",";
  response += "\"dac2_impedance\":" + String(dac2.readPowerDownModeDAC());
  response += "}";
  
  ws.textAll(response);
  WebSerial.println("📡 WebSocket broadcast: " + response);
}

// Ana komut işleyici task
void taskProcessCommand() {
  CommandTask cmd = getCommandFromQueue();
  
  if (cmd.active) {
    WebSerial.println("🔄 İşleniyor: " + cmd.command);
    
    if (cmd.command == "i2c_scan") {
      tI2CScan.restartDelayed();
    }
    else if (cmd.command == "dac_info") {
      tDacInfo.restartDelayed();
    }
    else if (cmd.command == "status") {
      tSystemStatus.restartDelayed();
    }
    else if (cmd.command == "wifi_info") {
      tWifiInfo.restartDelayed();
    }
    else if (cmd.command == "reset") {
      WebSerial.println("ESP8266 yeniden başlatılıyor...");
      delay(1000);
      ESP.restart();
    }
    else if (cmd.command == "free_heap") {
      WebSerial.println("Boş bellek: " + String(ESP.getFreeHeap()) + " bytes");
    }
    else if (cmd.command == "uptime") {
      uint32_t seconds = millis() / 1000;
      uint32_t minutes = seconds / 60;
      uint32_t hours = minutes / 60;
      seconds = seconds % 60;
      minutes = minutes % 60;
      
      WebSerial.printf("Çalışma süresi: %02d:%02d:%02d\n", hours, minutes, seconds);
    }
    else if (cmd.command == "clear") {
      WebSerial.print("\033[2J\033[H");
      WebSerial.println("=== ESP8266 WebSerial Terminal ===");
      WebSerial.println("Task-based komut sistemi aktif");
    }
  }
}

// I2C Scan Task - NON-BLOCKING
void taskI2CScan() {
  static byte currentAddress = 1;
  static int nDevices = 0;
  static bool scanStarted = false;
  
  if (!scanStarted) {
    WebSerial.println("=== I2C Tarama Başlatılıyor (Task-based) ===");
    WebSerial.println("Adres aralığı: 0x01 - 0x7F");
    WebSerial.println("");
    currentAddress = 1;
    nDevices = 0;
    scanStarted = true;
    
    // Task'i tekrar çalıştır
    tI2CScan.setInterval(20); // Her 20ms'de bir adres
    tI2CScan.setIterations(126); // 0x01-0x7E = 126 adres
    return;
  }
  
  // Her iterasyonda bir adres tara
  if (currentAddress < 0x7f) {
    Wire.beginTransmission(currentAddress);
    byte error = Wire.endTransmission();
    
    if (error == 0) {
      WebSerial.printf("✓ 0x%02X", currentAddress);
      
      // Bilinen cihazları tanı
      if (currentAddress == 0x60 || currentAddress == 0x61) {
        WebSerial.print(" (MCP4725 DAC)");
      } else if (currentAddress == 0x70 || currentAddress == 0x71) {
        WebSerial.print(" (TCA9548A)");  
      } else if (currentAddress == 0x48) {
        WebSerial.print(" (ADS1115)");
      } else if (currentAddress == 0x68) {
        WebSerial.print(" (DS3231)");
      } else if (currentAddress == 0x76 || currentAddress == 0x77) {
        WebSerial.print(" (BME280)");
      }
      WebSerial.println("");
      
      nDevices++;
    }
    
    currentAddress++;
  }
  
  // Son adres tarandıysa sonuçları göster
  if (currentAddress >= 0x7f) {
    WebSerial.println("");
    if (nDevices == 0) {
      WebSerial.println("❌ Hiç I2C cihazı bulunamadı!");
    } else {
      WebSerial.printf("✅ Toplam %d adet cihaz bulundu.\n", nDevices);
    }
    WebSerial.println("Tarama tamamlandı (Non-blocking).");
    
    // Task'i durdur
    scanStarted = false;
    tI2CScan.disable();
  }
}

// DAC Info Task
void taskDacInfo() {
  WebSerial.println("=== DAC Bilgileri (Task-based) ===");
  WebSerial.println("DAC #1 (0x60):");
  WebSerial.println("  Değer: " + String(dac1.getValue()) + "/4095");
  WebSerial.println("  Voltaj: " + String(dac1.getVoltage(3.3), 3) + "V");
  WebSerial.println("  mV: " + String(dac1.getMilliVolt(3.3)) + "mV");
  WebSerial.println("  Bağlı: " + String(dac1.isConnected() ? "Evet" : "Hayır"));
  
  WebSerial.println("DAC #2 (0x61):");
  WebSerial.println("  Değer: " + String(dac2.getValue()) + "/4095");
  WebSerial.println("  Voltaj: " + String(dac2.getVoltage(3.3), 3) + "V");
  WebSerial.println("  mV: " + String(dac2.getMilliVolt(3.3)) + "mV");
  WebSerial.println("  Bağlı: " + String(dac2.isConnected() ? "Evet" : "Hayır"));
}

// System Status Task
void taskSystemStatus() {
  WebSerial.println("=== Sistem Durumu (Task-based) ===");
  WebSerial.println("IP: " + WiFi.localIP().toString());
  WebSerial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");
  WebSerial.println("Uptime: " + String(millis()/1000) + " saniye");
  WebSerial.println("Free Heap: " + String(ESP.getFreeHeap()) + " bytes");
  WebSerial.println("DAC #1: " + String(dac1.getValue()) + " (" + String(dac1.getVoltage(3.3), 3) + "V)");
  WebSerial.println("DAC #2: " + String(dac2.getValue()) + " (" + String(dac2.getVoltage(3.3), 3) + "V)");
}

// WiFi Info Task
void taskWifiInfo() {
  WebSerial.println("=== WiFi Bilgileri (Task-based) ===");
  WebSerial.println("SSID: " + WiFi.SSID());
  WebSerial.println("IP: " + WiFi.localIP().toString());
  WebSerial.println("Gateway: " + WiFi.gatewayIP().toString());
  WebSerial.println("DNS: " + WiFi.dnsIP().toString());
  WebSerial.println("MAC: " + WiFi.macAddress());
  WebSerial.println("RSSI: " + String(WiFi.RSSI()) + " dBm");
}

// DAC1 Empedans Task
void taskSetDac1Impedance() {
  String modeNames[] = {"Normal (~1Ω)", "1kΩ Pull-Down", "100kΩ Pull-Down", "500kΩ Pull-Down"};
  
  WebSerial.println("🔧 DAC1 Empedans ayarlanıyor: " + modeNames[dac1_target_impedance]);
  
  // Son DAC değerini hatırla (Normal mode'a dönüş için)
  static int lastDac1Value = 0;
  
  // Eğer Normal mode'dan çıkıyorsak, son değeri sakla
  if (dac1.readPowerDownModeDAC() == 0 && dac1_target_impedance != 0) {
    lastDac1Value = dac1.getValue();
    WebSerial.println("💾 DAC1 son değer kaydedildi: " + String(lastDac1Value));
  }
  
  // RAM'de geçici empedans değişikliği (EEPROM'a yazma)
  int result = dac1.writePowerDownMode(dac1_target_impedance);
  
  if (result == 0) {
    WebSerial.println("✅ DAC #1 Empedans: " + modeNames[dac1_target_impedance]);
    
    // Eğer Normal mode'a dönüyorsak, son değeri geri yükle
    if (dac1_target_impedance == 0 && lastDac1Value > 0) {
      delay(10); // Empedans değişiminin tamamlanması için kısa bekleme
      dac1.setValue(lastDac1Value);
      float voltage = dac1.getVoltage(3.3);
      WebSerial.println("🔄 DAC1 son voltaj geri yüklendi: " + String(voltage, 3) + "V (" + String(lastDac1Value) + ")");
    }
    
    // WebSocket ile güncel durum gönder
    broadcastDacStatus();
  } else {
    WebSerial.println("❌ DAC #1 Empedans HATASI!");
    
    // Error response gönder
    String response = "{\"error\":\"DAC1 empedans ayarlanamadı\"}";
    ws.textAll(response);
  }
}

// DAC2 Empedans Task
void taskSetDac2Impedance() {
  String modeNames[] = {"Normal (~1Ω)", "1kΩ Pull-Down", "100kΩ Pull-Down", "500kΩ Pull-Down"};
  
  WebSerial.println("🔧 DAC2 Empedans ayarlanıyor: " + modeNames[dac2_target_impedance]);
  
  // Son DAC değerini hatırla (Normal mode'a dönüş için)
  static int lastDac2Value = 0;
  
  // Eğer Normal mode'dan çıkıyorsak, son değeri sakla
  if (dac2.readPowerDownModeDAC() == 0 && dac2_target_impedance != 0) {
    lastDac2Value = dac2.getValue();
    WebSerial.println("💾 DAC2 son değer kaydedildi: " + String(lastDac2Value));
  }
  
  // RAM'de geçici empedans değişikliği (EEPROM'a yazma)
  int result = dac2.writePowerDownMode(dac2_target_impedance);
  
  if (result == 0) {
    WebSerial.println("✅ DAC #2 Empedans: " + modeNames[dac2_target_impedance]);
    
    // Eğer Normal mode'a dönüyorsak, son değeri geri yükle
    if (dac2_target_impedance == 0 && lastDac2Value > 0) {
      delay(10); // Empedans değişiminin tamamlanması için kısa bekleme
      dac2.setValue(lastDac2Value);
      float voltage = dac2.getVoltage(3.3);
      WebSerial.println("🔄 DAC2 son voltaj geri yüklendi: " + String(voltage, 3) + "V (" + String(lastDac2Value) + ")");
    }
    
    // WebSocket ile güncel durum gönder
    broadcastDacStatus();
  } else {
    WebSerial.println("❌ DAC #2 Empedans HATASI!");
    
    // Error response gönder
    String response = "{\"error\":\"DAC2 empedans ayarlanamadı\"}";
    ws.textAll(response);
  }
}

// TaskScheduler loop - main.cpp'de çağrılacak
void runTaskScheduler() {
  taskScheduler.execute();
}