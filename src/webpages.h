#pragma once
#include <Arduino.h>

// Web sayfası HTML içeriği
const char webpage_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset='UTF-8'>
  <title>DAC Kontrol</title>
  <meta name='viewport' content='width=device-width, initial-scale=1'>
  <style>
    body{font-family:Arial,sans-serif;margin:0;padding:20px;background:#f5f5f5;}
    .container{max-width:600px;margin:0 auto;background:white;padding:30px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}
    h1{text-align:center;color:#333;margin-bottom:30px;}
    .dac-group{margin:30px 0;padding:20px;background:#f9f9f9;border-radius:6px;}
    .dac-title{font-size:18px;font-weight:bold;color:#555;margin-bottom:15px;}
    .slider{width:100%;height:8px;border-radius:4px;background:#ddd;outline:none;-webkit-appearance:none;}
    .slider::-webkit-slider-thumb{appearance:none;width:20px;height:20px;border-radius:50%;background:#4CAF50;cursor:pointer;}
    .slider::-moz-range-thumb{width:20px;height:20px;border-radius:50%;background:#4CAF50;cursor:pointer;border:none;}
    .voltage{font-size:28px;font-weight:bold;color:#4CAF50;text-align:center;margin:15px 0;}
    .input-row{display:flex;gap:10px;margin-top:15px;}
    .input-field{flex:1;padding:8px 12px;border:2px solid #ddd;border-radius:4px;font-size:14px;text-align:center;}
    .input-field:focus{outline:none;border-color:#4CAF50;}
    .input-field::placeholder{color:#999;}
    .impedance-row{display:flex;gap:10px;margin-top:10px;align-items:center;}
    .impedance-label{font-size:12px;color:#666;min-width:60px;}
    .impedance-select{flex:1;padding:8px 12px;border:2px solid #ddd;border-radius:4px;font-size:14px;background:white;}
    .impedance-select:focus{outline:none;border-color:#4CAF50;}
    .status{text-align:center;padding:10px;border-radius:4px;margin-bottom:20px;}
    .connected{background:#d4edda;color:#155724;}
    .disconnected{background:#f8d7da;color:#721c24;}
    .footer{text-align:center;margin-top:30px;font-size:12px;color:#999;}
  </style>
</head>
<body>
  <div class='container'>
    <h1>DAC Kontrol</h1>
    <div class='status connected' id='connection'>Bağlanıyor...</div>
    
    <div class='dac-group'>
      <div class='dac-title'>DAC #1</div>
      <input type='range' min='0' max='4095' value='0' class='slider' id='dac1Slider'>
      <div class='voltage' id='dac1Voltage'>0.00V</div>
      <div class='input-row'>
        <input type='number' min='0' max='3300' placeholder='mV girin' class='input-field' id='dac1mV'>
        <input type='number' min='0' max='4095' placeholder='DAC değeri' class='input-field' id='dac1Raw'>
      </div>
      <div class='impedance-row'>
        <span class='impedance-label'>Empedans:</span>
        <select class='impedance-select' id='dac1Impedance'>
          <option value='0'>Normal (~1Ω)</option>
          <option value='1'>1kΩ Pull-Down</option>
          <option value='2'>100kΩ Pull-Down</option>
          <option value='3'>500kΩ Pull-Down</option>
        </select>
      </div>
    </div>
    
    <div class='dac-group'>
      <div class='dac-title'>DAC #2</div>
      <input type='range' min='0' max='4095' value='0' class='slider' id='dac2Slider'>
      <div class='voltage' id='dac2Voltage'>0.00V</div>
      <div class='input-row'>
        <input type='number' min='0' max='3300' placeholder='mV girin' class='input-field' id='dac2mV'>
        <input type='number' min='0' max='4095' placeholder='DAC değeri' class='input-field' id='dac2Raw'>
      </div>
      <div class='impedance-row'>
        <span class='impedance-label'>Empedans:</span>
        <select class='impedance-select' id='dac2Impedance'>
          <option value='0'>Normal (~1Ω)</option>
          <option value='1'>1kΩ Pull-Down</option>
          <option value='2'>100kΩ Pull-Down</option>
          <option value='3'>500kΩ Pull-Down</option>
        </select>
      </div>
    </div>
    
    <div class='footer'>
      IP: %IP_ADDRESS%
    </div>
  </div>

  <script>
    var gateway = 'ws://' + window.location.hostname + '/ws';
    var websocket;
    var reconnectInterval = null;

    function initWebSocket() {
      console.log('Attempting to connect to WebSocket at: ' + gateway);
      websocket = new WebSocket(gateway);
      
      websocket.onopen = function(event) {
        document.getElementById('connection').innerHTML = 'Bağlandı';
        document.getElementById('connection').className = 'status connected';
        if(reconnectInterval) {
          clearInterval(reconnectInterval);
          reconnectInterval = null;
        }
      };
      
      websocket.onerror = function(error) {
        document.getElementById('connection').innerHTML = 'Bağlantı Hatası';
        document.getElementById('connection').className = 'status disconnected';
      };
      
      websocket.onclose = function(event) {
        document.getElementById('connection').innerHTML = 'Bağlantı Kesildi';
        document.getElementById('connection').className = 'status disconnected';
        if(!reconnectInterval) {
          reconnectInterval = setInterval(initWebSocket, 2000);
        }
      };
      
      websocket.onmessage = function(event) {
        console.log('Received: ' + event.data);
        try {
          var data = JSON.parse(event.data);
          if(data.dac1 !== undefined) {
            document.getElementById('dac1Slider').value = data.dac1;
            updateVoltageDisplay('dac1', data.dac1);
          }
          if(data.dac2 !== undefined) {
            document.getElementById('dac2Slider').value = data.dac2;
            updateVoltageDisplay('dac2', data.dac2);
          }
        } catch(e) {
          console.log('JSON parse error: ', e);
        }
      };
    }

    function updateVoltageDisplay(dac, value) {
      var voltage = (value / 4095 * 3.3).toFixed(2);
      document.getElementById(dac + 'Voltage').innerHTML = voltage + 'V';
    }

    function sendWebSocketData(message) {
      if(websocket && websocket.readyState === WebSocket.OPEN) {
        websocket.send(message);
        console.log('Sent: ' + message);
      } else {
        console.log('WebSocket not connected, cannot send: ' + message);
      }
    }

    function setDACValue(dac, value) {
      value = Math.max(0, Math.min(4095, value));
      sendWebSocketData(dac + ':' + value);
      updateVoltageDisplay(dac, value);
      document.getElementById(dac + 'Slider').value = value;
      document.getElementById(dac + 'Raw').value = value;
      document.getElementById(dac + 'mV').value = Math.round((value / 4095) * 3300);
    }

    // Slider event handlers
    document.getElementById('dac1Slider').addEventListener('input', function() {
      var value = parseInt(this.value);
      setDACValue('dac1', value);
    });

    document.getElementById('dac2Slider').addEventListener('input', function() {
      var value = parseInt(this.value);
      setDACValue('dac2', value);
    });

    // mV input handlers
    document.getElementById('dac1mV').addEventListener('keypress', function(e) {
      if(e.key === 'Enter') {
        var mV = parseInt(this.value);
        if(!isNaN(mV) && mV >= 0 && mV <= 3300) {
          var dacValue = Math.round((mV / 3300) * 4095);
          setDACValue('dac1', dacValue);
        }
      }
    });

    document.getElementById('dac2mV').addEventListener('keypress', function(e) {
      if(e.key === 'Enter') {
        var mV = parseInt(this.value);
        if(!isNaN(mV) && mV >= 0 && mV <= 3300) {
          var dacValue = Math.round((mV / 3300) * 4095);
          setDACValue('dac2', dacValue);
        }
      }
    });

    // Raw DAC value input handlers
    document.getElementById('dac1Raw').addEventListener('keypress', function(e) {
      if(e.key === 'Enter') {
        var dacValue = parseInt(this.value);
        if(!isNaN(dacValue) && dacValue >= 0 && dacValue <= 4095) {
          setDACValue('dac1', dacValue);
        }
      }
    });

    document.getElementById('dac2Raw').addEventListener('keypress', function(e) {
      if(e.key === 'Enter') {
        var dacValue = parseInt(this.value);
        if(!isNaN(dacValue) && dacValue >= 0 && dacValue <= 4095) {
          setDACValue('dac2', dacValue);
        }
      }
    });

    // Impedance change handlers
    document.getElementById('dac1Impedance').addEventListener('change', function() {
      var mode = parseInt(this.value);
      sendWebSocketData('dac1_impedance:' + mode);
      console.log('DAC1 Impedance changed to: ' + mode);
    });

    document.getElementById('dac2Impedance').addEventListener('change', function() {
      var mode = parseInt(this.value);
      sendWebSocketData('dac2_impedance:' + mode);
      console.log('DAC2 Impedance changed to: ' + mode);
    });

    window.addEventListener('load', initWebSocket);
  </script>
</body>
</html>
)rawliteral";