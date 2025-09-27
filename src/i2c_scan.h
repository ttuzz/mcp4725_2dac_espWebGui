
#pragma once

#include <Arduino.h>
#include "Wire.h"
#include "WebSerial.h"

// ESP8266 default I2C pins: SDA=GPIO4(D2), SCL=GPIO5(D1)
#define I2C_SDA 4
#define I2C_SCL 5

void i2c_init(){
  Wire.begin(I2C_SDA, I2C_SCL); // ESP8266: SDA = GPIO4, SCL = GPIO5
}
void i2c_scann()
{
  byte error, address;
  int nDevices = 0;

  //WebSerial.println("Scanning for I2C devices ...");
  for (address = 0x01; address < 0x7f; address++)
  {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0)
    {
      WebSerial.printf("I2C device found at address 0x%02X\n", address);
      nDevices++;
    }
    else if (error != 2)
    {
      WebSerial.printf("Error %d at address 0x%02X\n", error, address);
    }
  }
  if (nDevices == 0)
  {
    WebSerial.println("No I2C devices found");
  }
}

// Komut sistemi için performI2CScan fonksiyonu
void performI2CScan() {
  byte error, address;
  int nDevices = 0;
  
  WebSerial.println("Tarama başlatılıyor...");
  WebSerial.println("Adres aralığı: 0x01 - 0x7F");
  WebSerial.println("");
  
  for (address = 0x01; address < 0x7f; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    
    if (error == 0) {
      WebSerial.printf("✓ I2C cihaz bulundu: 0x%02X", address);
      
      // Bilinen cihazları tanı
      if (address == 0x60 || address == 0x61) {
        WebSerial.print(" (MCP4725 DAC)");
      } else if (address == 0x70 || address == 0x71) {
        WebSerial.print(" (TCA9548A Multiplexer)");  
      } else if (address == 0x48) {
        WebSerial.print(" (ADS1115 ADC)");
      } else if (address == 0x68) {
        WebSerial.print(" (DS3231 RTC)");
      } else if (address == 0x76 || address == 0x77) {
        WebSerial.print(" (BME280 Sensor)");
      }
      WebSerial.println("");
      
      nDevices++;
    } 
    else if (error != 2) {
      WebSerial.printf("! Hata %d - Adres: 0x%02X\n", error, address);
    }
    
    // Her 16 adres sonrası yield
    if ((address % 16) == 0) {
      yield();
    }
  }
  
  WebSerial.println("");
  if (nDevices == 0) {
    WebSerial.println("❌ Hiç I2C cihazı bulunamadı!");
    WebSerial.println("Bağlantıları kontrol edin.");
  } else {
    WebSerial.printf("✅ Toplam %d adet I2C cihazı bulundu.\n", nDevices);
  }
  WebSerial.println("Tarama tamamlandı.");
}
