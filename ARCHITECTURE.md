# SIMPLE SERVICE ARCHITECTURE

## Problem with Current System
❌ MQTT auto-connects on startup → SSL crashes  
❌ Multiple services fighting for GPIO 13  
❌ Certificates loading in memory → Out of RAM  
❌ WiFi connects before user selects service → Unnecessary  
❌ Boot loop crashes  
❌ Over-complicated with too many features  

## New Solution: SIMPLE SWITCH-CASE SYSTEM

### Boot Flow (Ultra-Simple)
```
Power ON
  ↓
Print menu to Serial
  ↓
Wait for Serial Input (user types 1, 2, or 3)
  ↓
Save choice to EEPROM
  ↓
Initialize ENERGY that service's GPIO only
  ↓
Loop: Run selected service forever
  ↓
(Optional: Listen for new service number to switch)
```

### Menu Display
```
════════════════════════════════════════
      ESP32-CAM TRAFFIC LIGHT
════════════════════════════════════════

Available Services:
  1) 🚨 Emergency Vehicle Detection
  2) 🌙 Adaptive Street Lighting
  3) 🔦 LED Test Demo

Enter service number (1-3): _

════════════════════════════════════════
```

### Service Definitions
```cpp
#define SERVICE_NONE 0
#define SERVICE_EMERGENCY 1      // Camera + Green LED
#define SERVICE_STREETLIGHT 2    // LDR + Street Light
#define SERVICE_DEMO 3           // Toggle LEDs manually
```

### GPIO Allocation (FINAL - NO CHANGES)
```
GPIO 12: RED LED (Status indicator - always available)
GPIO 13: GREEN LED (SERVICE 1) or LDR INPUT (SERVICE 2)
GPIO 14: STREET LIGHT LED (SERVICE 2 only)
GPIO 0-39: Camera pins (SERVICE 1 only)
```

### How It Works

#### Service 1: Emergency Detection
```
setup() → Initialize Camera + GPIO12/13 (RED ON, GREEN OFF)
loop() → Capture image every 15s → Check for vehicle → If detected: GREEN ON for 20s
cleanup() → Stop camera, safe GPIO
```

#### Service 2: Street Lighting
```
setup() → Initialize GPIO13 as INPUT (LDR) + GPIO 14 as OUTPUT
loop() → Read LDR every 2s → GPIO14 follows darkness: ON when dark, OFF when bright
cleanup() → Set GPIO to input (safe state)
```

#### Service 3: LED Demo
```
setup() → Initialize GPIO 12, 13, 14 as OUTPUT
loop() → Toggle RED/GREEN/STREET_LIGHT in sequence for testing
cleanup() → Set all LOW
```

### Code Structure
```
src/main.cpp
├── setup()
│   ├── Serial.begin()
│   ├── EEPROM.begin()
│   ├── displayMenu()
│   └── loadServiceFromEEPROM()
├── loop()
│   ├── checkSerialInput() → detect service switch
│   ├── if (SERVICE == 1) runEmergencyService()
│   ├── if (SERVICE == 2) runStreetLightService()
│   └── if (SERVICE == 3) runDemoService()
└── Individual service functions
    ├── runEmergencyService()
    ├── runStreetLightService()
    └── runDemoService()

NO separate files needed initially (keep it simple!)
NO global extern variables
NO WiFi/MQTT code
NO SSL certificates
NO complex init chains
```

### platformio.ini (SIMPLIFIED)
```ini
[env:esp32cam]
platform = espressif32
board = esp32cam
framework = arduino
monitor_speed = 115200

; ONLY REQUIRED SETTINGS
board_build.arduino.memory_type = qio_qspi
build_flags = 
    -DBOARD_HAS_PSRAM
    -mfix-esp32-psram-cache-issue

lib_deps = 
    ArduinoJson  ; (optional, only if using JSON)
    ; NO WiFiClientSecure
    ; NO PubSubClient
    ; NO certificates
```

### Data Flow: Service Switch Example
```
1. Device boots
2. Menu displayed
3. Serial input: User types "2" (Street Lighting)
4. EEPROM[0] = 2
5. GPIO 13 → INPUT, GPIO 14 → OUTPUT
6. Main loop: Only runStreetLightService() executes
7. User at serial: Types "1" (Emergency)
8. EEPROM[0] = 1
9. GPIO 13 → OUTPUT, Camera init
10. Main loop: Only runEmergencyService() executes
```

### Dependencies REMOVED
```
✗ WiFiClientSecure (SSL/TLS) ← ROOT OF ALL EVIL
✗ PubSubClient (MQTT)
✗ AWS IoT certificates
✗ HTTPClient (not needed for local testing)
✗ All the WiFi/MQTT/Lambda code
```

### Advantages of This Design
✅ **Zero SSL errors** - no certificates loaded  
✅ **Zero MQTT issues** - no async complications  
✅ **Zero memory crashes** - each service uses only its GPIO  
✅ **Clean code** - all in main.cpp, easy to follow  
✅ **Easy to debug** - one service at a time  
✅ **Easy to extend** - add new services by adding new function  
✅ **Clear serial debugging** - see exactly what's happening  

### Testing Plan
1. Compile & upload
2. Open Serial Monitor (115200 baud)
3. See menu
4. Enter "1" → Test emergency detection (manual camera input)
5. Enter "2" → Test street lighting (cover LDR with finger)
6. Enter "3" → Test LEDs (should see them blink)
7. Each service works, no crashes, no MQTT/WiFi hanging

---

**Ready to implement this clean version?**
