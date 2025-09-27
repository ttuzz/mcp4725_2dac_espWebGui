# ESP8266 MCP4725 DAC Web Controller

🚀 **Thread-Safe ESP8266 Web Server** for dual MCP4725 DAC control with real-time WebSocket communication

![Main Interface](assets/1.png)

## ⚡ Features

- 🎛️ **Dual MCP4725 DAC Control** - 12-bit precision, 0-3.3V output
- 🔄 **Thread-Safe Task System** - Non-blocking I2C operations  
- 🌐 **Real-time WebSocket** - Instant control feedback
- 💾 **Voltage Memory** - Auto-restore after empedans changes
- 📡 **WebSerial Terminal** - Built-in browser console
- 🔧 **OTA Updates** - Over-the-air firmware updates

![DAC Control](assets/2.png)

## 🛠️ Hardware

**ESP8266** (NodeMCU v2) + **2x MCP4725 DAC** modules

```
ESP8266        MCP4725
--------       -------
D1 (GPIO5) →   SCL
D2 (GPIO4) →   SDA
3.3V       →   VCC
GND        →   GND
```

**I2C Addresses:** DAC#1=0x60, DAC#2=0x61

![Hardware Setup](assets/3.png)

## 🚀 Quick Start

```bash
# Clone & Build
git clone <repository-url>
cd e8266_web
platformio run --target upload

# WiFi Setup
1. Connect to "ESP8266-Setup" hotspot
2. Configure WiFi credentials
3. Access: http://<ESP8266-IP>/
```

![WebSerial Terminal](assets/4.png)

## 🎮 Usage

### **Web Interface**
- 🎛️ **Sliders**: Real-time voltage control (0-3.3V)
- ⚡ **Empedans Modes**: Normal/1kΩ/100kΩ/500kΩ pull-down
- 💾 **Auto-Restore**: Voltage memory after empedans changes

### **WebSocket API**
```javascript
// Control DACs
ws.send("dac1:2048");              // Set DAC1 value
ws.send("dac1_impedance:2");       // Set empedans mode

// Receive updates
ws.onmessage = (e) => {
    const data = JSON.parse(e.data);
    // data.dac1, dac2, dac1_impedance, dac2_impedance
};
```

### **WebSerial Commands**
```
help         - Command list
i2c_scan     - Device discovery  
dac_info     - DAC status
status       - System info
reset        - Restart ESP8266
```

## 🏗️ Architecture

**Thread-Safe Task System** prevents ESP8266 crashes:
- ⚡ **WebSocket handlers** → Queue commands (interrupt-safe)
- 🔄 **Task scheduler** → Execute I2C operations (main loop)
- 💾 **Voltage memory** → Auto-restore system
- 🛡️ **Crash prevention** → No blocking operations in interrupts

```cpp
// Thread-safe empedans control
triggerDac1ImpedanceTask(mode);  // Queue → Task → Execute
```

## 🐛 Troubleshooting

| Problem | Solution |
|---------|----------|
| 🚫 DAC not responding | Check I2C connections, run `i2c_scan` |
| 📡 WebSocket disconnects | Check WiFi signal, monitor heap memory |
| ⚡ No voltage output | Empedans in power-down mode, set to "Normal" |
| 📤 Upload fails | Hold FLASH button, check USB cable |

## 📈 Performance

- **Memory**: RAM 47.7%, Flash 42.9%
- **Response**: WebSocket <10ms, DAC <5ms
- **I2C**: Non-blocking incremental scan

## 🔄 Development

```bash
platformio run                    # Build
platformio run --target upload    # Upload
platformio device monitor         # Serial monitor
```

**WebSerial debugging**: `http://<IP>/webserial`

---

⭐ **Thread-Safe • Task-Based • Real-Time DAC Control** ⭐