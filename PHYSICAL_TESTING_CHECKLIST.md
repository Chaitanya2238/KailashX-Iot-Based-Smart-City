# 🧪 Physical ESP32 Testing Checklist

**Status:** Ready to Hardware Deploy ✅

---

## 🔧 PRE-UPLOAD CHECKLIST

### 1. Secrets Configuration
- [ ] Copy `include/secrets.h.example` to `include/secrets.h`
- [ ] Fill in **WiFi SSID**
- [ ] Fill in **WiFi Password**
- [ ] Fill in **AWS_IOT_ENDPOINT** (format: `xxxxx.iot.region.amazonaws.com`)
- [ ] Fill in **AWS_IOT_THING_NAME** (your AWS IoT thing name)
- [ ] Fill in **API_GATEWAY_URL** (Lambda endpoint URL)
- [ ] Fill in **AWS_CERT_CA** (Amazon Root CA 1 certificate)
- [ ] Fill in **AWS_CERT_CRT** (Device certificate)
- [ ] Fill in **AWS_CERT_PRIVATE** (Private key)

### 2. Hardware Setup
- [ ] ESP32-CAM connected to USB via CH340 adapter
- [ ] Red LED (GPIO 12) connected with resistor
- [ ] Green LED (GPIO 13) connected with resistor
- [ ] Street Light LED (GPIO 14) connected with resistor
- [ ] LDR Sensor module wired to GPIO 13 (digital out)
- [ ] LDR module has potentiometer installed & adjusted
- [ ] Camera ribbon cable fully inserted (golden springs visible)
- [ ] Antenna/WiFi extension connected (if using)
- [ ] All wires secured and not floating

### 3. platformio.ini Configuration
- [ ] Check `upload_port` - should match your COM port
- [ ] Run: `platformio device list` to verify your device
- [ ] Update `upload_port = COM?` if needed
- [ ] Verify `board = esp32cam`
- [ ] Verify `framework = arduino`

### 4. Build Configuration
- [ ] Clean previous builds: `platformio run --target clean`
- [ ] Verify all libraries installed: `platformio lib list`
- [ ] Expected libraries:
  - [ ] ArduinoJson 7.4.2+
  - [ ] PubSubClient 2.8.0+

---

## 📤 UPLOAD PROCEDURE

### Step 1: Connect Device
```bash
1. Plug ESP32-CAM into USB adapter
2. Plug USB adapter into computer
3. Wait 2 seconds for USB to enumerate
4. Open Device Manager (Windows) or `ls /dev/ttyUSB*` (Linux)
5. Note the COM port (should be COM3-COM9)
```

### Step 2: Compile & Upload
```bash
# Option A: Using VS Code PlatformIO
1. Press Ctrl+Alt+U (shortcut to upload)
2. or: Click "Upload" in PlatformIO sidebar
3. Wait 2-3 minutes for compilation

# Option B: Using Terminal
platformio run -t upload
```

### Step 3: Monitor Serial Output
```bash
# Option A: Using VS Code PlatformIO
1. Press Ctrl+Alt+M (shortcut to monitor)
2. or: Click "Serial Monitor" in PlatformIO sidebar
3. Set baud rate: 115200

# Option B: Using Terminal
platformio device monitor --baud 115200
```

---

## ✅ BOOT SEQUENCE VERIFICATION

### Expected Serial Output (SERVICE 1: Emergency Detection)

```
╔════════════════════════════════════════╗
║        ESP32-CAM INITIALIZATION        ║
╚════════════════════════════════════════╝

🔄 Starting system...
📦 EEPROM initialized
  └─ Loaded service: 1 (Emergency Detection)

🔧 GPIO initialized
  ├─ Red LED (GPIO 12): Status
  └─ Green LED (GPIO 13): Alert

📷 Camera initializing...
  ├─ Resolution: 800x600 (SVGA)
  └─ JPEG quality: 12

🌐 Connecting to WiFi: YOUR_SSID
  └─ Connected! IP: 192.168.X.X

🔐 Connecting to AWS IoT...
  └─ Connected!
  └─ Subscribed to: esp32/service/set

╔════════════════════════════════════════╗
║       SYSTEM READY - MONITORING        ║
║    Service: EMERGENCY DETECTION ✓      ║
╚════════════════════════════════════════╝

(Waiting 15 seconds for first capture...)
```

### Expected Serial Output (SERVICE 2: Street Lighting)

```
╔════════════════════════════════════════╗
║        ESP32-CAM INITIALIZATION        ║
╚════════════════════════════════════════╝

🔄 Starting system...
📦 EEPROM initialized
  └─ Loaded service: 2 (Street Lighting)

💡 Service 2: ADAPTIVE STREET LIGHTING
  └─ LDR Sensor (GPIO 13): Light detection
  └─ Street Light LED (GPIO 14): Light control
  └─ Adjust potentiometer on LDR module for threshold

🌐 Connecting to WiFi: YOUR_SSID
  └─ Connected! IP: 192.168.X.X

🔐 Connecting to AWS IoT...
  └─ Connected!
  └─ Subscribed to: esp32/service/set

╔════════════════════════════════════════╗
║       SYSTEM READY - MONITORING        ║
║    Service: STREET LIGHTING ✓          ║
╚════════════════════════════════════════╝

(Polling LDR every 2 seconds...)
```

---

## 🧪 TEST 1: Initial Boot (SERVICE 1)

**Objective:** Verify ESP32 boots without errors

**Steps:**
1. [ ] Power on device
2. [ ] Observe serial output
3. [ ] Wait for "SYSTEM READY" message
4. [ ] Verify WiFi connects (IP address shows)
5. [ ] Verify MQTT connects (subscribed message shows)

**Expected:**
- ✅ Green LED should be OFF (normal state)
- ✅ Red LED should be ON (status indicator)
- ✅ No error messages in serial
- ✅ Timestamp shows in "SYSTEM READY"

**Pass/Fail:** ________

---

## 🧪 TEST 2: Image Capture (SERVICE 1)

**Objective:** Verify camera captures and uploads images

**Steps:**
1. [ ] Device is powered and ready (from Test 1)
2. [ ] Wait 15 seconds (CAPTURE_INTERVAL)
3. [ ] Observe serial output
4. [ ] Look for "Capturing image #1" message

**Expected Output:**
```
📸 Capturing image #1 for detection...
   Camera stabilized (3 frames flushed)
   Image Size: XXXXX bytes
   ✓ Valid JPEG frame
📤 Uploading to AWS Lambda...
   ✓ HTTP Response: 200
   Response:
   ✓ Normal - No emergency
   Objects: OBJECT1, OBJECT2, ...
```

**Pass/Fail:** ________

**Troubleshooting:**
- If "Camera capture failed!": Check ribbon cable
- If "WiFi disconnected": Check SSID/password in secrets.h
- If "HTTP Response: 400/500": Check API_GATEWAY_URL in secrets.h

---

## 🧪 TEST 3: Test Mode Emergency (SERVICE 1)

**Objective:** Verify test mode triggers emergency detection on images 5-9

**Steps:**
1. [ ] Device is running in SERVICE 1
2. [ ] Wait for images 1-4 (normal, non-emergency)
3. [ ] Image #5 should trigger simulated emergency
4. [ ] Watch Green LED turn ON

**Expected Output (Image #5):**
```
╔══════════════════════════════════════╗
║   TEST MODE: SIMULATED EMERGENCY     ║
╚══════════════════════════════════════╝
🚨 EMERGENCY VEHICLE DETECTED!
   Vehicle: Ambulance
   Confidence: 95.0% (SIMULATED)
```

**Expected Behavior:**
- ✅ Green LED turns ON (full brightness)
- ✅ Red LED turns OFF
- ✅ Emergency message displays
- ✅ Serial shows duration countdown (20 seconds)

**Pass/Fail:** ________

---

## 🧪 TEST 4: Emergency Timeout (SERVICE 1)

**Objective:** Verify LED turns off after 20 seconds

**Steps:**
1. [ ] Image #5 triggered emergency (from Test 3)
2. [ ] Green LED is ON
3. [ ] Wait 20 seconds
4. [ ] Watch Green LED turn OFF

**Expected Output (After 20s):**
```
✓ Emergency period expired
🟢→🔴 Returning to normal state
   Green LED: OFF
   Red LED: ON
```

**Pass/Fail:** ________

---

## 🧪 TEST 5: Service Switching via MQTT

**Objective:** Switch from SERVICE 1 to SERVICE 2 via MQTT command

**Steps:**
1. [ ] Device is running in SERVICE 1
2. [ ] Open MQTT client (AWS IoT Core Test, or MQTT Explorer)
3. [ ] Subscribe to topic: `esp32/service/set`
4. [ ] Publish message: `{"mode": 2}`
5. [ ] Observe device response

**Expected Serial Output:**
```
📬 MQTT Message Received:
   Topic: esp32/service/set
   Payload: {"mode": 2}

🔄 Switching from Service 1 to Service 2
💡 Service 2: ADAPTIVE STREET LIGHTING
  └─ LDR Sensor (GPIO 13): Light detection
  └─ Street Light LED (GPIO 14): Light control

📡 Published to esp32/service/2/status: switched
```

**Expected Behavior:**
- ✅ Red & Green LEDs turn OFF
- ✅ Device switches to street lighting mode
- ✅ EEPROM saves new service (check not lost after power cycle)
- ✅ Serial shows "STREET LIGHTING" mode active

**Pass/Fail:** ________

---

## 🧪 TEST 6: LDR Sensor (SERVICE 2)

**Objective:** Verify LDR sensor detects light changes

**Steps:**
1. [ ] Device is in SERVICE 2 (street lighting)
2. [ ] Observe initial state (bright or dark)
3. [ ] Cover LDR sensor with finger (or hand)
4. [ ] Wait 2 seconds
5. [ ] Observe street light LED ON
6. [ ] Uncover LDR sensor
7. [ ] Wait 2 seconds
8. [ ] Observe street light LED OFF

**Expected Serial Output (Dark):**
```
🌙 Dark detected - Street Light ON
📡 Published to esp32/service/2/status: light_on
```

**Expected Serial Output (Bright):**
```
☀️  Bright detected - Street Light OFF
📡 Published to esp32/service/2/status: light_off
```

**Expected Behavior:**
- ✅ Street Light LED (GPIO 14) turns ON when dark
- ✅ Street Light LED (GPIO 14) turns OFF when bright
- ✅ No repeated messages if state unchanged
- ✅ Transition happens within 2-3 seconds

**Pass/Fail:** ________

**LDR Adjustment:**
If LED doesn't respond properly:
1. [ ] Check that potentiometer on LDR module is connected
2. [ ] Manually adjust potentiometer half-turn at a time
3. [ ] Test again with finger cover
4. [ ] Fine-tune until sensitivity is good

---

## 🧪 TEST 7: EEPROM Persistence

**Objective:** Verify service selection persists across power cycles

**Steps:**
1. [ ] Device is in SERVICE 2 (street lighting)
2. [ ] Verify "STREET LIGHTING" mode in serial
3. [ ] Power OFF device
4. [ ] Wait 10 seconds
5. [ ] Power ON device
6. [ ] Observe boot sequence

**Expected Serial Output:**
```
📦 EEPROM initialized
  └─ Loaded service: 2 (Street Lighting)

💡 Service 2: ADAPTIVE STREET LIGHTING
```

**Expected Behavior:**
- ✅ Device boots in SERVICE 2 (not SERVICE 1)
- ✅ Service persisted to EEPROM
- ✅ No manual reconfiguration needed

**Pass/Fail:** ________

---

## 🧪 TEST 8: Switch Back to SERVICE 1

**Objective:** Return to emergency detection after testing street lighting

**Steps:**
1. [ ] Device is in SERVICE 2
2. [ ] Open MQTT client
3. [ ] Publish message: `{"mode": 1}`
4. [ ] Observe device response

**Expected Serial Output:**
```
📬 MQTT Message Received:
   Topic: esp32/service/set
   Payload: {"mode": 1}

🔄 Switching from Service 2 to Service 1
🚨 Service 1: EMERGENCY VEHICLE DETECTION
  ├─ RED LED (GPIO 12): Status indicator
  └─ GREEN LED (GPIO 13): Emergency alert
```

**Expected Behavior:**
- ✅ Street Light LED (GPIO 14) turns OFF
- ✅ Red LED turns ON (normal state)
- ✅ Device switches back to emergency detection

**Pass/Fail:** ________

---

## 🧪 TEST 9: WiFi Reconnection (SERVICE 1)

**Objective:** Verify device handles WiFi disconnections gracefully

**Steps:**
1. [ ] Device is running and connected (Test 1-2 passed)
2. [ ] Disconnect router power (or WiFi)
3. [ ] Observe serial output
4. [ ] Wait 20+ seconds
5. [ ] Reconnect router
6. [ ] Observe device reconnecting

**Expected Serial Output (During Disconnection):**
```
⚠️  MQTT disconnected - reconnecting...
🌐 Connecting to WiFi: YOUR_SSID
(waiting for WiFi - 0s 5s 10s 15s ...)
```

**Expected After Reconnection:**
```
✓ WiFi Connected!
   IP Address: 192.168.X.X

🔐 Connecting to AWS IoT...
   ✓ Connected!
   ✓ Subscribed to: esp32/service/set
```

**Pass/Fail:** ________

---

## 🧪 TEST 10: Emergency Accuracy (SERVICE 1)

**Objective:** In real-world conditions, test emergency vehicle detection

**Prerequisites:**
- Device operating for 30+ minutes (full stabilization)
- Clear daylight (no glare or shadows)
- Camera pointing at traffic
- AWS Lambda function deployed and running

**Steps:**
1. [ ] Device captures normal traffic images (cars, trucks, buses)
2. [ ] Observe no false positives (emergency LED stays OFF)
3. [ ] Point camera at actual emergency vehicle (ambulance/fire truck)
4. [ ] Observe LED turns ON when vehicle detected

**Expected Behavior:**
- ✅ No emergency triggers on normal vehicles
- ✅ LED triggers within 15 seconds of emergency vehicle
- ✅ Confidence > 85% in Rekognition response
- ✅ No false positives on police cars (unless configured)

**Pass/Fail:** ________

---

## 📊 SUMMARY TABLE

| Test # | Feature | Expected | Status | Notes |
|--------|---------|----------|--------|-------|
| 1 | Boot Sequence | System Ready | __ | Verify all subsystems init |
| 2 | Image Capture | Upload Success | __ | Check API Gateway connection |
| 3 | Test Emergency | Triggered on #5 | __ | Green LED ON |
| 4 | Emergency Timeout | LED OFF after 20s | __ | Timer accuracy |
| 5 | MQTT Switching | Service switches | __ | Check EEPROM | 
| 6 | LDR Control | LED responds to light | __ | May need LDR adjustment |
| 7 | EEPROM Save | Service persists | __ | Power cycle test |
| 8 | Switch Back | Returns to SERVICE 1 | __ | Verify GPIO reconfigured |
| 9 | WiFi Reconnect | Auto-reconnects | __ | Test resilience |
| 10 | Real Traffic | Accurate detection | __ | Field testing |

---

## 🐛 COMMON ISSUES & FIXES

### Issue 1: "Camera capture failed!"
**Cause:** Camera ribbon not fully seated
**Fix:**
1. Power off device
2. Remove camera ribbon
3. Look for 2 gold springs on camera PCB (should be fully visible)
4. Reinsert ribbon until springs are compressed again
5. Power on and test

### Issue 2: "WiFi disconnected" on every startup
**Cause:** Incorrect SSID/password in secrets.h
**Fix:**
1. Verify secrets.h has correct WiFi SSID
2. Verify password is correct (case-sensitive)
3. Ensure WiFi is 2.4GHz (5GHz not supported on ESP32-CAM)
4. Recompile and upload

### Issue 3: "HTTP Response: 400"
**Cause:** API Gateway URL incorrect in secrets.h
**Fix:**
1. Log into AWS API Gateway console
2. Find your Lambda integration
3. Find the invoke URL (should end with /upload or /detect)
4. Copy exact URL and paste into secrets.h

### Issue 4: "HTTP Response: 500"
**Cause:** Lambda function error or AWS credentials issue
**Fix:**
1. Check AWS Lambda CloudWatch logs for errors
2. Verify Lambda has permission to call Rekognition
3. Verify AWS_IOT_THING_NAME matches IoT thing in AWS console
4. Check AWS certificates are valid (not expired)

### Issue 5: "Invalid JPEG header"
**Cause:** Camera distorted frame or memory corruption
**Fix:**
1. Increase camera stabilization delay: `delay(300);` instead of `delay(200);`
2. Add more frame flushes: `for (i=0; i<5; i++)` instead of `3`
3. Check camera ribbon connection
4. Check power supply (needs 5V, 2A minimum)

### Issue 6: MQTT service switch doesn't work
**Cause:** Not subscribed to correct topic or JSON format wrong
**Fix:**
1. Verify MQTT topic: `esp32/service/set`
2. Verify JSON format: `{"mode": 1}` or `{"mode": 2}`
3. Check MQTT client is connected to AWS IoT
4. Check device is subscribed to topic (should show in serial)

### Issue 7: LDR doesn't respond to light
**Cause:** Potentiometer on LDR module not adjusted
**Fix:**
1. Locate the small potentiometer on LDR module
2. Slowly turn it while covering/uncovering sensor
3. Adjust until GPIO 13 reads HIGH in dark, LOW in bright
4. Test by printing GPIO state in serial monitor

### Issue 8: "Image too small" or "Image too large"
**Cause:** Camera resolution or lighting conditions
**Fix:**
- **Too Small (< 5KB):** 
  - Increase JPEG quality (reduce value: try 10 instead of 12)
  - Check lighting (camera needs light)
  - Check for camera lens cap
  
- **Too Large (> 5MB):**
  - Decrease JPEG quality (increase value: try 15 instead of 12)
  - Reduce resolution (try FRAMESIZE_VGA instead of SVGA)
  - Check available PSRAM (flash chip might be full)

---

## 📋 SIGN-OFF

**All Tests Completed:** _________ (Date)

**All Tests Passed:** ✅ [ ] YES  ❌ [ ] NO

**Ready for Production Deployment:** ✅ [ ] YES  ❌ [ ] NO

**Issues Remaining:** 
```
_______________________________________________________
_______________________________________________________
_______________________________________________________
```

**Notes:**
```
_______________________________________________________
_______________________________________________________
_______________________________________________________
```

---

**Document Version:** 2.0  
**Last Updated:** April 8, 2026  
**Valid For:** v2.0 Multi-Service Architecture
