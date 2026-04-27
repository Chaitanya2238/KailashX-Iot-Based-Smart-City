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
bool warning_active = false;
unsigned long emergency_start_time = 0;
unsigned long warning_start_time = 0;

// Safety flags
bool camera_initialized = false;
int last_ldr_state = -1;  // Track LDR state changes (for debounce)
unsigned long last_menu_message = 0;  // Menu timeout message timer

// ===========================
// UTILITY FUNCTIONS
// ===========================
void resetToSafeState() {
  // Force all indicator LEDs to OFF and OUTPUT mode
  // This prevents LDR or other sensors from "back-driving" the LEDs
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_STREET_LIGHT, OUTPUT);
  
  digitalWrite(LED_RED, LOW);
  digitalWrite(LED_GREEN, LOW);
  digitalWrite(LED_STREET_LIGHT, LOW);
  
  // Also ensure buzzer is off
  pinMode(PIN_BUZZER, OUTPUT);
  digitalWrite(PIN_BUZZER, LOW);
}

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
  Serial.println("⏳ Initializing Emergency Detection (Simulated Mode)...");
  
  resetToSafeState(); // Ensure clean slate
  
  // Setup GPIO - ONLY for this service
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  
  // Default state: Red ON (Monitoring), Green OFF
  digitalWrite(LED_RED, HIGH);
  digitalWrite(LED_GREEN, LOW);
  
  Serial.println("   ✓ GPIO 12: RED LED (OUTPUT) - Monitoring Status");
  Serial.println("   ✓ GPIO 13: GREEN LED (OUTPUT) - Priority Status");
  Serial.println("   ✓ Simulated Camera: READY (Hardware bypassed for stability)");

  printServiceHeader("EMERGENCY DETECTION");
  
  // Mark as initialized so the service loop runs, 
  // but we skip the actual hardware esp_camera_init()
  camera_initialized = true; 
  
  service_start_time = millis();
  // Set last action to 7.5 seconds ago so the first capture happens in 7.5 seconds
  last_service_action = millis() - 7500; 
  emergency_active = false;
  warning_active = false;
}

void runService1() {
  if (!camera_initialized) {
    delay(100);
    return;
  }
  
  // --- SAFETY LED SYNC ---
  // Ensure LED states match active modes to prevent leakage or interference
  if (warning_active || emergency_active) {
    digitalWrite(LED_RED, LOW);
    digitalWrite(LED_GREEN, HIGH);
  }

  // --- 2-STAGE WORKFLOW LOGIC ---
  
  // Check if Warning period has expired (10s)
  if (warning_active && (millis() - warning_start_time >= 10000)) {
    warning_active = false;
    emergency_active = true;
    emergency_start_time = millis();
    
    // Stage 2: Priority (30s)
    // Red remains OFF, Green remains ON
    Serial.println("DATA:STATE:PRIORITY");
    Serial.println("🟢 Priority Lane Active (30s Countdown)");
  }
  
  // Check if Emergency period has expired (30s)
  if (emergency_active && (millis() - emergency_start_time >= 30000)) {
    emergency_active = false;
    
    // Recovery: Back to Normal
    digitalWrite(LED_RED, HIGH);
    digitalWrite(LED_GREEN, LOW);
    Serial.println("DATA:STATE:NORMAL");
    Serial.println("🚦 Recovery - Returning to Monitoring Mode");
    last_service_action = millis(); // Reset capture timer to start 15s delay AFTER recovery
  }
  
  // Handle Manual Reset
  if (Serial.available() > 0) {
    int raw = Serial.peek();
    if (raw == 'R') {
        // Let loop handle the refresh command
    } else {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();
        if (cmd == "RESET") {
          warning_active = false;
          emergency_active = false;
          digitalWrite(LED_RED, HIGH);
          digitalWrite(LED_GREEN, LOW);
          Serial.println("DATA:STATE:NORMAL");
          Serial.println("🔄 Manual Reset - System Normalized");
          last_service_action = millis(); // Reset capture timer
        }
    }
  }

  // Capture simulation every 15 seconds
  if (millis() - last_service_action >= 15000 && !emergency_active && !warning_active) {
    last_service_action = millis();
    
    Serial.println("📸 Capturing image for detection...");
    
    // Realistic simulation of processing delay
    delay(500); 
    
    // PRINT REALISTIC LOGS (Purely Simulated)
    int sim_size = random(3000, 5001);
    Serial.print("   Image size: ");
    Serial.print(sim_size);
    Serial.println(" bytes");
    Serial.println("   ✓ Valid JPEG frame");
    
    static int capture_count = 0;
    capture_count++;
    
    // HARDCODED DETECTION: Trigger every 2nd capture
    if (capture_count % 2 == 0) {
      Serial.println("\n🚨 EMERGENCY VEHICLE DETECTED (HARDCODED)");
      Serial.println("   Vehicle: Ambulance");
      Serial.println("   Confidence: 98.6%");
      
      // Stage 1: Warning (10s)
      warning_active = true;
      warning_start_time = millis();
      
      digitalWrite(LED_RED, LOW);
      digitalWrite(LED_GREEN, HIGH);
      
      Serial.println("DATA:STATE:WARNING");
      Serial.println("🟡 Warning Phase: Lanes 2 & 3 slowing down (10s)");
    } else {
      Serial.println("   ✓ Analysis complete: No emergency detected");
    }
  }
}

void cleanupService1() {
  Serial.println("🧹 Cleaning up Service 1...");
  resetToSafeState();
  camera_initialized = false;
  Serial.println("   ✓ Service 1 cleanup complete\n");
}

// ===========================
// SERVICE 2: STREET LIGHTING
// ===========================
void initService2() {
  Serial.println("⏳ Initializing Street Lighting...");
  
  resetToSafeState(); // Start clean
  
  // Setup GPIO - Street light on GPIO 14
  pinMode(LED_STREET_LIGHT, OUTPUT);     // GPIO 14 = Street light
  digitalWrite(LED_STREET_LIGHT, LOW);   // Start with light off
  
  // Note: GPIO 13 (Green LED) is shared with LDR. 
  // We will pulse-read it in runService2 to keep the LED off.
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  
  // Reset LDR state tracker
  last_ldr_state = -1;
  
  Serial.println("   ✓ GPIO configured for SERVICE 2");
  Serial.println("   ✓ GPIO 13: LDR sensor (Shared with Green LED - Pulse Mode)");
  Serial.println("   ✓ GPIO 14: Street Light LED (OUTPUT)\n");
  
  printServiceHeader("STREET LIGHTING");
  
  service_start_time = millis();
  last_service_action = millis();
}

void runService2() {
  // Poll LDR every 2 seconds
  if (millis() - last_service_action >= 2000) {
    last_service_action = millis();
    
    // --- PULSE READ TRICK ---
    // Switch to input just long enough to read the LDR value, 
    // then immediately back to OUTPUT LOW to keep the Green LED off.
    pinMode(LED_GREEN, INPUT);
    delayMicroseconds(50); // Small stabilization delay
    int ldr_value = digitalRead(LED_GREEN);  // Read GPIO 13
    pinMode(LED_GREEN, OUTPUT);
    digitalWrite(LED_GREEN, LOW); // Force LED OFF
    
    // ONLY print when state CHANGES (debounce spam)
    if (ldr_value != last_ldr_state) {
      last_ldr_state = ldr_value;
      
      if (ldr_value == HIGH) {
        // Dark detected
        digitalWrite(LED_STREET_LIGHT, HIGH);
        Serial.println("DATA:LIGHT:ON");
        Serial.println("🌙 Darkness detected - Street Light ON");
      } else {
        // Bright detected
        digitalWrite(LED_STREET_LIGHT, LOW);
        Serial.println("DATA:LIGHT:OFF");
        Serial.println("☀️ Brightness detected - Street Light OFF");
      }
    }
  }
}

void cleanupService2() {
  Serial.println("🧹 Cleaning up Service 2...");
  resetToSafeState();
  last_ldr_state = -1;
  Serial.println("   ✓ Service 2 cleanup complete\n");
}

// ===========================
// SERVICE 3: AIR QUALITY MONITORING
// ===========================
void initService3() {
  Serial.println("⏳ Initializing Air Quality Service...");
  
  resetToSafeState(); // Start clean
  
  // Setup GPIO - ONLY for this service
  pinMode(PIN_MQ135, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(LED_RED, OUTPUT);
  
  // FORCE GPIO 13 (Green LED/LDR) to be LOW OUTPUT to prevent LDR leakage
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  
  digitalWrite(LED_RED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  
  Serial.println("   ✓ GPIO configured for SERVICE 3");
  Serial.println("   ✓ GPIO 15: MQ135 Analog Input (AO)");
  Serial.println("   ✓ GPIO 2: Buzzer Alert (OUTPUT)");
  Serial.println("   ✓ GPIO 12: RED Warning LED (OUTPUT)");
  Serial.println("   ✓ GPIO 13: Forced LOW (Prevent LDR Leakage)");
  Serial.println("   ⚠ MQ135 requires warm-up for accurate readings\n");
  
  printServiceHeader("AIR QUALITY MONITORING");
  
  service_start_time = millis();
  last_service_action = millis();
}

void runService3() {
  // Read sensor every 2 seconds
  if (millis() - last_service_action >= 2000) {
    last_service_action = millis();

    // 1. Get average reading for stability
    long sum = 0;
    for(int i=0; i<5; i++) {
      sum += analogRead(PIN_MQ135);
      delay(10);
    }
    int raw_value = sum / 5;

    // --- SIMPLE OPTIMIZED MQ135 CALCULATION ---
    // Mapping based on user room baseline: 3600 ADC ≈ 150 AQI
    
    // 1. Calculate PPM (Simple linear approximation for CO2/VOCs)
    // Range 0-4095 maps to roughly 0-2000 PPM
    float ppm = (raw_value / 4095.0) * 2000.0;
    
    // 2. Calculate AQI (Direct mapping optimized for user's sensor behavior)
    // 3600 ADC maps to ~150-160 AQI
    int aqi = map(raw_value, 0, 4095, 0, 185);

    // 3. Ensure values stay in realistic bounds
    if (aqi < 0) aqi = 0;
    if (ppm < 350) ppm = 350 + (raw_value % 20); // Minimum atmospheric CO2

    // Categorize (Adjusted to user preference: 150 is Moderate)
    const char* status = "UNKNOWN";
    if (aqi <= 80) status = "Good";
    else if (aqi <= 170) status = "Moderate";
    else if (aqi <= 250) status = "Sensitive";
    else if (aqi <= 350) status = "Unhealthy";
    else status = "Hazardous";

    // Send Data Flag for Web Dashboard Parsing
    Serial.print("DATA:AIR:");
    Serial.print(ppm);
    Serial.print(":");
    Serial.println(aqi);

    // Detailed monitor output
    Serial.print("🌬️ Air Quality: ");
    Serial.print(status);
    Serial.print(" | PPM: ");
    Serial.print(ppm);
    Serial.print(" | AQI: ");
    Serial.print(aqi);
    Serial.print(" (Raw: ");
    Serial.print(raw_value);
    Serial.println(")");

    // Buzzer Alert for every reading (Confirmation Beep)
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);
    
    // Alert if AQI is truly high (> 250)
    if (aqi > 250) {
      digitalWrite(LED_RED, HIGH);
      if (aqi > 350) {
        Serial.println("   ⚠️ ALERT: AIR QUALITY HAZARD!");
      }
    } else {
      digitalWrite(LED_RED, LOW);
    }
  }
}

void cleanupService3() {
  Serial.println("🧹 Cleaning up Service 3...");
  resetToSafeState();
  Serial.println("   ✓ Service 3 cleanup complete\n");
}

// ===========================
// SERVICE 4: WATER QUALITY MONITORING
// ===========================
void initService4() {
  Serial.println("⏳ Initializing Water Quality Service...");
  
  resetToSafeState(); // Start clean
  
  // Setup GPIO - ONLY for this service
  pinMode(PIN_TDS, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(LED_RED, OUTPUT);

  // FORCE GPIO 13 (Green LED/LDR) to be LOW OUTPUT to prevent LDR leakage
  pinMode(LED_GREEN, OUTPUT);
  digitalWrite(LED_GREEN, LOW);
  
  digitalWrite(LED_RED, LOW);
  digitalWrite(PIN_BUZZER, LOW);
  
  Serial.println("   ✓ GPIO configured for SERVICE 4");
  Serial.println("   ✓ GPIO 4: TDS Analog Input (AO)");
  Serial.println("   ✓ GPIO 2: Buzzer Alert (OUTPUT)");
  Serial.println("   ✓ GPIO 12: RED Warning LED (OUTPUT)");
  Serial.println("   ✓ GPIO 13: Forced LOW (Prevent LDR Leakage)");
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

    // CALIBRATED TDS CALCULATION (Quadratic fit for your hardware)
    // Offset: 400 raw (Dry baseline)
    // Scale: 514 raw -> 150 ppm, 1200 raw -> 1500 ppm
    int x = raw_value - 400;
    if (x < 0) x = 0; 
    
    // Formula: ppm = 0.000823 * x^2 + 1.216 * x
    int ppm = (int)(0.000823 * x * x + 1.216 * x);

    // Combined Water Quality Logic
    const char* status = "UNKNOWN";
    if (ppm <= 10) status = "DRY / NO WATER";
    else if (ppm < 50) status = "IDEAL (RO/Distilled) - Low Minerals / Flat Taste";
    else if (ppm < 150) status = "EXCELLENT (Drinking) - Perfect Mineral Balance";
    else if (ppm < 300) status = "GOOD (Drinking) - Safe with Pleasant Mineral Taste";
    else if (ppm < 500) status = "FAIR (Acceptable) - Hard Water / Metallic Taste";
    else if (ppm < 1000) status = "POOR (Contaminated) - High Minerals / Needs Treatment";
    else status = "UNACCEPTABLE - Unfit for Regular Drinking";

    // Send Data Flag for Web Dashboard Parsing
    Serial.print("DATA:TDS:");
    Serial.println(ppm);

    // Detailed monitor output
    Serial.print("💧 Water Quality: ");
    Serial.print(status);
    Serial.print(" (");
    Serial.print(ppm);
    Serial.println(" ppm)");

    // Buzzer Alert for every reading (Confirmation Beep)
    digitalWrite(PIN_BUZZER, HIGH);
    delay(100);
    digitalWrite(PIN_BUZZER, LOW);

    // Alert if POOR or worse
    if (ppm >= 500) {
      Serial.println("   ⚠️ ALERT: POOR WATER QUALITY!");
    }
  }
}

void cleanupService4() {
  Serial.println("🧹 Cleaning up Service 4...");
  resetToSafeState();
  Serial.println("   ✓ Service 4 cleanup complete\n");
}

// ===========================
// SETUP
// ===========================
void setup() {
  // CRITICAL: Power-on delay for hardware stability
  // This helps prevent brownouts during initial power-up
  delay(1000);
  
  Serial.begin(115200);
  Serial.println("\n\n🚀 System Booting...");
  Serial.println("╔═══════════════════════════════════════════╗");
  Serial.println("║    ESP32-CAM TRAFFIC LIGHT SYSTEM         ║");
  Serial.println("║            STARTING UP                     ║");
  Serial.println("╚═══════════════════════════════════════════╝\n");
  
  // Initialize EEPROM
  EEPROM.begin(512);
  Serial.println("✓ EEPROM initialized");
  
  // Initialize ALL GPIO to safe states (LOW/OUTPUT)
  resetToSafeState();
  Serial.println("✓ All GPIOs initialized to safe state (LOW)");
  
  // ALWAYS start at menu - user must select service
  // LEDs will be OFF until user selects
  current_service = SERVICE_MENU;
  Serial.println("✓ Starting in menu mode - waiting for service selection");
  Serial.println("READY: System initialized. Connect dashboard now.");
  
  delay(500);
}

// ===========================
// MAIN LOOP
// ===========================
void loop() {
  // Check for serial input (menu or service switch)
  if (Serial.available()) {
    int raw = Serial.read();
    if (raw == -1) return;
    char input = (char)raw;

    // Ignore whitespace
    if (input == '\r' || input == '\n' || input == ' ') {
      return;
    }

    // Check for cooldown - consume but discard if too soon
    if (millis() - last_service_switch < 500) {
      return;
    }

    // Handle various inputs
    if (input == '0' || input == '!') {
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
        
        resetToSafeState(); // Ensure all LEDs are OFF in menu
        current_service = SERVICE_MENU;
        last_service_switch = millis();
        clearSerialBuffer();
      }
      else if (input >= '1' && input <= '4') {
        // Switch to new service
        int new_service = input - '0';
        
        if (new_service != current_service) {
          Serial.print("\n🔄 Switching to service ");
          Serial.println(new_service);
          
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
            Serial.println("   ✓ Service preference saved");
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
      else if (input == 'r' || input == 'R') {
        // Re-announce current state to any newly connected client
        Serial.println("\n🔄 Status Refresh Requested");
        Serial.println("READY: System initialized. Dashboard synchronized.");
        Serial.print("STATUS:SERVICE:");
        Serial.println(current_service);
        
        if (current_service == SERVICE_MENU) {
          displayMenu();
        } else if (current_service == SERVICE_EMERGENCY) {
          printServiceHeader("EMERGENCY DETECTION");
        } else if (current_service == SERVICE_STREETLIGHT) {
          printServiceHeader("STREET LIGHTING");
        } else if (current_service == SERVICE_AIRQUALITY) {
          printServiceHeader("AIR QUALITY MONITORING");
        } else if (current_service == SERVICE_WATERQUALITY) {
          printServiceHeader("WATER QUALITY MONITORING");
        }
        
        // Reset action timers so next reading fires immediately
        last_service_action = 0;
        last_menu_message = 0;
      }
      else if (input == '?') {
        // Status query
        Serial.print("STATUS:SERVICE:");
        Serial.println(current_service);
      }
      // Ignore \r, \n and other characters
    }
  
  // Run active service or menu
  if (current_service == SERVICE_MENU) {
    // Show menu every 10 seconds only (less spam)
    if (millis() - last_menu_message >= 10000) {
      last_menu_message = millis();
      displayMenu();
      Serial.println("READY: Waiting for service selection (1-4)...");
    }
  } else if (current_service == SERVICE_EMERGENCY) {
    runService1();
  } else if (current_service == SERVICE_STREETLIGHT) {
    runService2();
  } else if (current_service == SERVICE_AIRQUALITY) {
    runService3();
  } else if (current_service == SERVICE_WATERQUALITY) {
    runService4();
  }
  
  delay(50);  // Responsive loop
}
