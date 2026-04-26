// ===========================
// ESP32-CAM SIMPLE TRAFFIC LIGHT
// Menu-Based Service Selection
// v3.0 - CLEAN & ERROR-FREE
// ===========================
#define CAMERA_MODEL_AI_THINKER

#include <Arduino.h>
#include "esp_camera.h"
#include <EEPROM.h>

// ===========================
// GPIO DEFINITIONS
// ===========================
#define LED_RED 12              // Status indicator (always available)
#define LED_GREEN 13            // Green LED (SERVICE 1) / LDR input (SERVICE 2)
#define LED_STREET_LIGHT 14     // Street light LED (SERVICE 2 only)
#define PIN_TDS 4               // TDS Water Sensor Analog Input (SERVICE 4)
#define PIN_BUZZER 2            // Buzzer for Service 4 alerts
#define PIN_MQ135 15            // MQ135 Air Quality Analog Input (SERVICE 3)

// Camera pins for ESP32-CAM
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

// ===========================
// SERVICE DEFINITIONS
// ===========================
#define SERVICE_MENU 0
#define SERVICE_EMERGENCY 1
#define SERVICE_STREETLIGHT 2
#define SERVICE_AIRQUALITY 3
#define SERVICE_WATERQUALITY 4

// ===========================
// STATE VARIABLES
// ===========================
int current_service = SERVICE_MENU;  // Currently active service
unsigned long service_start_time = 0;
unsigned long last_service_action = 0;
unsigned long last_service_switch = 0; // Cooldown timer
bool emergency_active = false;
unsigned long emergency_start_time = 0;

// Safety flags
bool camera_initialized = false;
int last_ldr_state = -1;  // Track LDR state changes (for debounce)
unsigned long last_menu_message = 0;  // Menu timeout message timer

// ===========================
// UTILITY FUNCTIONS
// ===========================
void displayMenu() {
  Serial.println("\n╔═══════════════════════════════════════════╗");
  Serial.println("║    ESP32-CAM TRAFFIC LIGHT SYSTEM         ║");
  Serial.println("║         Menu - Select A Service            ║");
  Serial.println("╠═══════════════════════════════════════════╣");
  Serial.println("║                                           ║");
  Serial.println("║  1) 🚨 EMERGENCY VEHICLE DETECTION       ║");
  Serial.println("║     (Camera + Green LED alert)           ║");
  Serial.println("║                                           ║");
  Serial.println("║  2) 🌙 ADAPTIVE STREET LIGHTING           ║");
  Serial.println("║     (LDR sensor + Street Light)          ║");
  Serial.println("║                                           ║");
  Serial.println("║  3) 🌬️ AIR QUALITY MONITORING             ║");
  Serial.println("║     (MQ135 Gas Sensor)                    ║");
  Serial.println("║                                           ║");
  Serial.println("║  4) 💧 WATER QUALITY MONITORING           ║");
  Serial.println("║     (TDS Purity Sensor)                   ║");
  Serial.println("║                                           ║");
  Serial.println("╠═══════════════════════════════════════════╣");
  Serial.print("║ Enter service number (1-4): ");
  Serial.println("              ║");
  Serial.println("╚═══════════════════════════════════════════╝\n");
}

void printServiceHeader(const char* service_name) {
  Serial.println("\n╔═══════════════════════════════════════════╗");
  Serial.print("║ ✓ SERVICE ACTIVE: ");
  Serial.println(service_name);
  Serial.println("║                                           ║");
  Serial.println("║ Press '0' to return to menu              ║");
  Serial.println("║ Press '1'-'4' to switch service          ║");
  Serial.println("╚═══════════════════════════════════════════╝\n");
}

void clearSerialBuffer() {
  while (Serial.available()) {
    Serial.read();
  }
}

// ===========================
// SERVICE 1: EMERGENCY DETECTION
// ===========================
void initService1() {
  Serial.println("⏳ Initializing Emergency Detection...");
  
  // Setup GPIO - ONLY for this service
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_STREET_LIGHT, INPUT);  // Disable street light GPIO (safe state)
  
  digitalWrite(LED_RED, HIGH);    // RED on (status)
  digitalWrite(LED_GREEN, LOW);   // GREEN off (no emergency)
  
  Serial.println("   ✓ GPIO configured for SERVICE 1");
  Serial.println("   ✓ GPIO 12: RED status LED (OUTPUT)");
  Serial.println("   ✓ GPIO 13: GREEN emergency LED (OUTPUT)");
  Serial.println("   ✓ GPIO 14: Disabled (INPUT - safe state)");
  
  // Setup Camera
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  // Use smaller frame size and quality first (safer)
  config.frame_size = FRAMESIZE_QVGA;  // 320x240 instead of VGA
  config.jpeg_quality = 12;             // Higher number = more compression
  config.fb_count = 1;
  
  // Try DRAM first (more reliable), then PSRAM
  config.fb_location = CAMERA_FB_IN_DRAM;
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("   ⚠ DRAM buffer failed: 0x%x\n", err);
    
    // Try with PSRAM if available
    if (psramFound()) {
      Serial.println("   Attempting PSRAM buffer...");
      config.fb_location = CAMERA_FB_IN_PSRAM;
      err = esp_camera_init(&config);
    }
    
    if (err != ESP_OK) {
      Serial.printf("   ❌ Camera init failed: 0x%x\n", err);
      Serial.println("   Check camera ribbon cable and power");
      camera_initialized = false;
      return;
    }
  }
  
  // Mark camera as successfully initialized
  camera_initialized = true;
  Serial.println("   ✓ Camera initialized");
  Serial.println("   ✓ Ready to capture frames\n");
  
  printServiceHeader("EMERGENCY DETECTION");
  
  service_start_time = millis();
  last_service_action = millis();
  emergency_active = false;
}

void runService1() {
  // Skip if camera failed to initialize
  if (!camera_initialized) {
    delay(100);
    return;
  }
  
  // Check if emergency period has expired
  if (emergency_active && (millis() - emergency_start_time >= 20000)) {
    emergency_active = false;
    digitalWrite(LED_GREEN, LOW);
    Serial.println("🚦 Emergency ended - Green LED OFF");
  }
  
  // Capture image every 15 seconds
  if (millis() - last_service_action >= 15000) {
    last_service_action = millis();
    
    Serial.println("📸 Capturing image for detection...");
    
    // Stabilize camera
    delay(200);
    for (int i = 0; i < 3; i++) {
      camera_fb_t *fb = esp_camera_fb_get();
      if (fb) esp_camera_fb_return(fb);
      delay(100);
    }
    
    // Capture frame
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("   ❌ Frame capture failed");
      return;
    }
    
    Serial.print("   Image size: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
    
    // Validate JPEG
    if (fb->buf[0] == 0xFF && fb->buf[1] == 0xD8) {
      Serial.println("   ✓ Valid JPEG frame");
      
      // SIMULATED DETECTION (for testing without Lambda)
      // In real system: POST to Lambda, parse response
      // For now: Simulate detection every 3rd capture
      static int capture_count = 0;
      capture_count++;
      
      if (capture_count % 3 == 0) {
        Serial.println("\n🚨 EMERGENCY VEHICLE DETECTED (SIMULATED)");
        Serial.println("   Vehicle: Ambulance");
        Serial.println("   Confidence: 95.0%\n");
        
        digitalWrite(LED_GREEN, HIGH);
        emergency_active = true;
        emergency_start_time = millis();
        Serial.println("🚨 Green LED ON - EMERGENCY MODE");
      } else {
        Serial.println("   ✓ No emergency detected");
      }
    } else {
      Serial.println("   ❌ Invalid JPEG header");
    }
    
    esp_camera_fb_return(fb);
  }
}

void cleanupService1() {
  Serial.println("🧹 Cleaning up Service 1...");
  
  // Turn off LEDs
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  
  // Deinitialize camera only if it was initialized
  if (camera_initialized) {
    esp_camera_deinit();
    delay(200);  // Give camera time to deinitialize
    camera_initialized = false;
    Serial.println("   ✓ Camera deinitialized");
  }
  
  Serial.println("   ✓ Service 1 cleanup complete\n");
}

// ===========================
// SERVICE 2: STREET LIGHTING
// ===========================
void initService2() {
  Serial.println("⏳ Initializing Street Lighting...");
  
  // Setup GPIO - ONLY for this service
  pinMode(LED_RED, INPUT);               // Disable red LED GPIO (safe state)
  pinMode(LED_GREEN, INPUT);             // GPIO 13 = LDR input
  pinMode(LED_STREET_LIGHT, OUTPUT);     // GPIO 14 = Street light
  digitalWrite(LED_STREET_LIGHT, LOW);   // Start with light off
  
  // Reset LDR state tracker
  last_ldr_state = -1;
  
  Serial.println("   ✓ GPIO configured for SERVICE 2");
  Serial.println("   ✓ GPIO 12: Disabled (INPUT - safe state)");
  Serial.println("   ✓ GPIO 13: LDR sensor (INPUT)");
  Serial.println("   ✓ GPIO 14: Street Light LED (OUTPUT)\n");
  
  printServiceHeader("STREET LIGHTING");
  
  service_start_time = millis();
  last_service_action = millis();
}

void runService2() {
  // Poll LDR every 2 seconds
  if (millis() - last_service_action >= 2000) {
    last_service_action = millis();
    
    int ldr_value = digitalRead(LED_GREEN);  // Read GPIO 13
    
    // ONLY print when state CHANGES (debounce spam)
    if (ldr_value != last_ldr_state) {
      last_ldr_state = ldr_value;
      
      if (ldr_value == HIGH) {
        // Dark detected
        digitalWrite(LED_STREET_LIGHT, HIGH);
        Serial.println("🌙 Darkness detected - Street Light ON (GPIO 13: HIGH)");
      } else {
        // Bright detected
        digitalWrite(LED_STREET_LIGHT, LOW);
        Serial.println("☀️ Brightness detected - Street Light OFF (GPIO 13: LOW)");
      }
    }
  }
}

void cleanupService2() {
  Serial.println("🧹 Cleaning up Service 2...");
  
  // Turn off street light
  digitalWrite(LED_STREET_LIGHT, LOW);
  
  // Safe GPIO states
  pinMode(LED_RED, INPUT);        // Disable red LED
  pinMode(LED_GREEN, INPUT);      // LDR back to input (safe)
  pinMode(LED_STREET_LIGHT, INPUT); // Disable street light
  
  // Reset state tracker
  last_ldr_state = -1;
  
  Serial.println("   ✓ Service 2 cleanup complete\n");
}

// ===========================
// SERVICE 3: AIR QUALITY MONITORING
// ===========================
void initService3() {
  Serial.println("⏳ Initializing Air Quality Service...");
  
  // Setup GPIO - ONLY for this service
  pinMode(PIN_MQ135, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, INPUT);         // Safe state
  pinMode(LED_STREET_LIGHT, INPUT);  // Safe state
  
  digitalWrite(LED_RED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  
  Serial.println("   ✓ GPIO configured for SERVICE 3");
  Serial.println("   ✓ GPIO 15: MQ135 Analog Input (AO)");
  Serial.println("   ✓ GPIO 2: Buzzer Alert (OUTPUT)");
  Serial.println("   ✓ GPIO 12: RED Warning LED (OUTPUT)");
  Serial.println("   ⚠ MQ135 requires warm-up for accurate readings\n");
  
  printServiceHeader("AIR QUALITY MONITORING");
  
  service_start_time = millis();
  last_service_action = millis();
}

void runService3() {
  // Read sensor every 2 seconds
  if (millis() - last_service_action >= 2000) {
    last_service_action = millis();
    
    int raw_value = analogRead(PIN_MQ135);
    
    // Simple conversion logic for demo (Value to Quality Percentage)
    // MQ135 readings increase with more gas/smoke
    int quality_pct = map(raw_value, 0, 4095, 0, 100);
    
    // Categorize Air Purity
    const char* status = "UNKNOWN";
    if (quality_pct < 20) status = "EXCELLENT";
    else if (quality_pct < 40) status = "GOOD";
    else if (quality_pct < 60) status = "MODERATE";
    else status = "POOR (High Gases)";
    
    // Send Data Flag for Web Dashboard Parsing
    Serial.print("DATA:AIR:");
    Serial.println(quality_pct);
    
    // Detailed monitor output
    Serial.print("🌬️ Air Quality Status: ");
    Serial.print(status);
    Serial.print(" (Value: ");
    Serial.print(quality_pct);
    Serial.println(" %)");
    
    // Buzzer Alert for every reading (Confirmation Beep)
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100); 
    digitalWrite(PIN_BUZZER, LOW);
    
    // Visual warning on hardware (Blink Red LED if POOR)
    if (quality_pct > 60) {
      digitalWrite(LED_RED, !digitalRead(LED_RED));
      Serial.println("   ⚠️ ALERT: POOR AIR QUALITY!");
    } else {
      digitalWrite(LED_RED, LOW);
    }
  }
}

void cleanupService3() {
  Serial.println("🧹 Cleaning up Service 3...");
  
  // Turn off warning LED and Buzzer
  digitalWrite(LED_RED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  
  // Safe GPIO states
  pinMode(PIN_MQ135, INPUT);
  pinMode(PIN_BUZZER, INPUT);
  pinMode(LED_RED, INPUT);
  
  Serial.println("   ✓ Service 3 cleanup complete\n");
}

// ===========================
// SERVICE 4: WATER QUALITY MONITORING
// ===========================
void initService4() {
  Serial.println("⏳ Initializing Water Quality Service...");
  
  // Setup GPIO - ONLY for this service
  pinMode(PIN_TDS, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, INPUT);         // Safe state
  pinMode(LED_STREET_LIGHT, INPUT);  // Safe state
  
  digitalWrite(LED_RED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  
  Serial.println("   ✓ GPIO configured for SERVICE 4");
  Serial.println("   ✓ GPIO 4: TDS Analog Input (AO)");
  Serial.println("   ✓ GPIO 2: Buzzer Alert (OUTPUT)");
  Serial.println("   ✓ GPIO 12: RED Warning LED (OUTPUT)");
  Serial.println("   ⚠ Ensure TDS probe is submerged for accurate readings\n");
  
  printServiceHeader("WATER QUALITY MONITORING");
  
  service_start_time = millis();
  last_service_action = millis();
}

void runService4() {
  // Read sensor every 2 seconds
  if (millis() - last_service_action >= 2000) {
    last_service_action = millis();
    
    int raw_value = analogRead(PIN_TDS);
    
    // Simple conversion logic (Voltage to PPM)
    // Formula: TDS = (0.5 * Voltage^3 - 5.7 * Voltage^2 + 192.2 * Voltage)
    // For demo, we use a simpler linear estimation:
    float voltage = raw_value * (3.3 / 4095.0);
    int ppm = (int)(voltage * 1000 / 2.3); 
    
    // Categorize Water Purity
    const char* status = "UNKNOWN";
    if (ppm < 50) status = "IDEAL (RO/Distilled)";
    else if (ppm < 170) status = "GOOD (Drinking)";
    else if (ppm < 400) status = "FAIR (Hard Water)";
    else status = "POOR (Contaminated)";
    
    // Send Data Flag for Web Dashboard Parsing
    Serial.print("DATA:TDS:");
    Serial.println(ppm);
    
    // Detailed monitor output
    Serial.print("💧 Water Quality Status: ");
    Serial.print(status);
    Serial.print(" (Value: ");
    Serial.print(ppm);
    Serial.println(" ppm)");
    
    // Buzzer Alert for every reading (Confirmation Beep)
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100); 
    digitalWrite(PIN_BUZZER, LOW);
    
    // Visual warning on hardware (Blink Red LED if POOR)
    if (ppm > 400) {
      digitalWrite(LED_RED, !digitalRead(LED_RED));
    } else {
      digitalWrite(LED_RED, LOW);
    }
  }
}

void cleanupService4() {
  Serial.println("🧹 Cleaning up Service 4...");
  
  // Turn off warning LED and Buzzer
  digitalWrite(LED_RED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  
  // Safe GPIO states
  pinMode(PIN_TDS, INPUT);
  pinMode(PIN_BUZZER, INPUT);
  pinMode(LED_RED, INPUT);
  
  Serial.println("   ✓ Service 4 cleanup complete\n");
}

// ===========================
// SETUP
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n");
  Serial.println("╔═══════════════════════════════════════════╗");
  Serial.println("║    ESP32-CAM TRAFFIC LIGHT SYSTEM         ║");
  Serial.println("║            STARTING UP                     ║");
  Serial.println("╚═══════════════════════════════════════════╝\n");
  
  // Initialize EEPROM
  EEPROM.begin(512);
  Serial.println("✓ EEPROM initialized");
  
  // Initialize ALL GPIO to safe states (LOW/INPUT)
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_STREET_LIGHT, OUTPUT);
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_STREET_LIGHT, LOW);
  Serial.println("✓ All GPIOs initialized to safe state (LOW)");
  
  // ALWAYS start at menu - user must select service
  // LEDs will be OFF until user selects
  current_service = SERVICE_MENU;
  Serial.println("✓ Starting in menu mode - waiting for service selection\n");
  
  // Do NOT initialize any service
  // Do NOT turn on any LEDs
  // User will select service via serial input
  
  delay(500);
}

// ===========================
// MAIN LOOP
// ===========================
void loop() {
  // Check for serial input (menu or service switch)
  if (Serial.available()) {
    char input = Serial.read();
    
    // Ignore input during cooldown after service switch (prevent rapid switching)
    if (millis() - last_service_switch < 500) {
      return;  // Ignore input, device still initializing new service
    }
    
    if (input == '0') {
      // Return to menu
      Serial.println("\n⏸ Returning to menu...");
      
      if (current_service == SERVICE_EMERGENCY) {
        cleanupService1();
      } else if (current_service == SERVICE_STREETLIGHT) {
        cleanupService2();
      } else if (current_service == SERVICE_AIRQUALITY) {
        cleanupService3();
      } else if (current_service == SERVICE_WATERQUALITY) {
        cleanupService4();
      }
      
      current_service = SERVICE_MENU;
      last_service_switch = millis();
      clearSerialBuffer();
    }
    else if (input >= '1' && input <= '4') {
      // Switch to new service
      int new_service = input - '0';
      
      if (new_service != current_service) {
        Serial.println("\n🔄 Switching service...");
        
        // Cleanup old service
        if (current_service == SERVICE_EMERGENCY) {
          cleanupService1();
        } else if (current_service == SERVICE_STREETLIGHT) {
          cleanupService2();
        } else if (current_service == SERVICE_AIRQUALITY) {
          cleanupService3();
        } else if (current_service == SERVICE_WATERQUALITY) {
          cleanupService4();
        }
        
        delay(200);  // Let GPIO settle
        
        // Save and init new service
        current_service = new_service;
        EEPROM.write(0, new_service);
        if (EEPROM.commit()) {
          Serial.println("   ✓ Service saved to EEPROM");
        } else {
          Serial.println("   ❌ EEPROM save failed");
        }
        
        if (current_service == SERVICE_EMERGENCY) {
          initService1();
        } else if (current_service == SERVICE_STREETLIGHT) {
          initService2();
        } else if (current_service == SERVICE_AIRQUALITY) {
          initService3();
        } else if (current_service == SERVICE_WATERQUALITY) {
          initService4();
        }
        
        last_service_switch = millis();
        clearSerialBuffer();
      }
    }
    // Ignore other input silently (don't spam the user)
  }
  
  // Run active service or menu
  if (current_service == SERVICE_MENU) {
    displayMenu();
    
    // Show helpful timeout message every 5 seconds if waiting
    if (millis() - last_menu_message >= 5000) {
      last_menu_message = millis();
      Serial.println("⏱️  Waiting for input... (enter 1, 2, 3, or 4)\n");
    }
    
    delay(1000);  // Slow polling in menu
  } else if (current_service == SERVICE_EMERGENCY) {
    runService1();
  } else if (current_service == SERVICE_STREETLIGHT) {
    runService2();
  } else if (current_service == SERVICE_AIRQUALITY) {
    runService3();
  } else if (current_service == SERVICE_WATERQUALITY) {
    runService4();
  }
  
  delay(100);  // Brief delay to prevent CPU spinning
}
