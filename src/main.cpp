// ===========================
// ESP32-CAM EMERGENCY VEHICLE DETECTION
// Simple LED Control System
// v1.1 - STABILITY FIXES
// ===========================
#define CAMERA_MODEL_AI_THINKER

// --- LIBRARIES ---
#include "esp_camera.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- INCLUDE YOUR CREDENTIALS ---
#include "secrets.h"

// ===========================
// GPIO PIN DEFINITIONS
// ===========================
#define LED_RED_PIN 12      // Red LED - Always ON (normal state)
#define LED_GREEN_PIN 13    // Green LED - ON when emergency detected

// ===========================
// TIMING CONFIGURATION
// ===========================
#define CAPTURE_INTERVAL 15000         // Capture every 15 seconds
#define EMERGENCY_LIGHT_DURATION 20000 // Green LED on for 20 seconds when emergency detected

// ===========================
// CAMERA PINS (AI-THINKER)
// ===========================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

// --- GLOBAL VARIABLES ---
WiFiClientSecure net; // Client for MQTT (persistent)
PubSubClient client(net);
unsigned long lastCaptureTime = 0;
unsigned long emergencyStartTime = 0;
bool emergencyActive = false;
int captureCount = 0;   // Track number of images captured

// --- FUNCTION PROTOTYPES ---
void initCamera();
void setupWifi();
void connectAWS();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void uploadImage(camera_fb_t * fb);
void setNormalState();
void setEmergencyState();

// ===========================
// SETUP
// ===========================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);
  delay(500);   // Stabilize serial output
  Serial.println("\n\n╔════════════════════════════════════════╗");
  Serial.println("║  ESP32-CAM Emergency Vehicle System  ║");
  Serial.println("║      Simple LED Control v1.1       ║");
  Serial.println("╚════════════════════════════════════════╝\n");

  // Initialize LED pins
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  
  // Set initial state: Red ON, Green OFF
  setNormalState();
  Serial.println("✓ LEDs Initialized");
  Serial.println("  └─ RED LED (GPIO 12): Always ON (Normal State)");
  Serial.println("  └─ GREEN LED (GPIO 13): ON when Emergency Detected\n");

  // Initialize camera
  initCamera();
  
  // Connect to WiFi
  setupWifi();
  
  // Connect to AWS IoT
  connectAWS();
  
  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║       SYSTEM READY - MONITORING        ║");
  Serial.println("╚════════════════════════════════════════╝\n");
  
  // Initial delay to stabilize
  delay(2000);
}

// ===========================
// MAIN LOOP
// ===========================
void loop() {
  // Maintain MQTT connection
  if (!client.connected()) {
    Serial.println("⚠️  MQTT disconnected - reconnecting...");
    connectAWS();
  }
  client.loop();

  // Check if emergency period has expired
  if (emergencyActive && (millis() - emergencyStartTime >= EMERGENCY_LIGHT_DURATION)) {
    emergencyActive = false;
    setNormalState();
  }

  // Capture and upload image at intervals
  if (millis() - lastCaptureTime >= CAPTURE_INTERVAL) {
    lastCaptureTime = millis();
    captureCount++;   // Increment capture counter
    
    Serial.print("📸 Capturing image #");
    Serial.print(captureCount);
    Serial.println(" for detection...");
    
    // === IMPROVED CAMERA STABILIZATION ===
    // Flush 3 frames and add longer delay for better image quality
    delay(200);
    for (int i = 0; i < 3; i++) {
      camera_fb_t * temp_fb = esp_camera_fb_get();
      if (temp_fb) {
        esp_camera_fb_return(temp_fb);
      }
      delay(100);
    }
    Serial.println("   Camera stabilized (3 frames flushed)");
    
    camera_fb_t * fb = esp_camera_fb_get();
    
    if (!fb) {
      Serial.println("❌ Camera capture failed!");
      return;
    }
    
    Serial.print("   Image Size: ");
    Serial.print(fb->len);
    Serial.println(" bytes");
    
    // Validate JPEG header
    if (fb->buf[0] != 0xFF || fb->buf[1] != 0xD8) {
      Serial.println("❌ Invalid JPEG header - skipping");
      esp_camera_fb_return(fb);
      return;
    }
    
    // Accept smaller images (camera might be in dark room)
    if (fb->len < 5000) {
      Serial.print("❌ Image too small (");
      Serial.print(fb->len);
      Serial.println(" bytes) - likely corrupted");
      esp_camera_fb_return(fb);
      return;
    }
    
    Serial.println("   ✓ Valid JPEG frame");
    
    // TEST MODE: Force emergency detection on images 5-9
    if (captureCount >= 5 && captureCount <= 9) {
      Serial.println("\n╔══════════════════════════════════════╗");
      Serial.println("║   TEST MODE: SIMULATED EMERGENCY     ║");
      Serial.println("╚══════════════════════════════════════╝");
      Serial.println("🚨 EMERGENCY VEHICLE DETECTED!");
      
      // Ambulance for images 5, 6, 7
      if (captureCount >= 5 && captureCount <= 7) {
        Serial.println("   Vehicle: Ambulance");
        Serial.println("   Confidence: 95.0% (SIMULATED)");
      } 
      // Fire Truck for images 8, 9
      else if (captureCount >= 8 && captureCount <= 9) {
        Serial.println("   Vehicle: Fire Truck");
        Serial.println("   Confidence: 92.5% (SIMULATED)");
      }
      
      setEmergencyState();
      esp_camera_fb_return(fb);
      return; // Skip upload during test
    }
    
    uploadImage(fb);
    esp_camera_fb_return(fb);
  }
  
  // Give more time for background WiFi/MQTT tasks
  delay(200);
}

// ===========================
// LED CONTROL FUNCTIONS
// ===========================
void setNormalState() {
  digitalWrite(LED_RED_PIN, HIGH);   // Red ON
  digitalWrite(LED_GREEN_PIN, LOW);  // Green OFF
  Serial.println("🚦 STATE: NORMAL (Red LED ON)");
}

void setEmergencyState() {
  digitalWrite(LED_RED_PIN, LOW);    // Red OFF
  digitalWrite(LED_GREEN_PIN, HIGH); // Green ON
  emergencyActive = true;
  emergencyStartTime = millis();
  Serial.println("🚨 STATE: EMERGENCY (Green LED ON)");
}

// ===========================
// CAMERA INITIALIZATION
// ===========================
void initCamera() {
  Serial.println("📷 Initializing Camera...");
  
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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  // Check PSRAM
  if (psramFound()) {
    Serial.println("   ✓ PSRAM found");
    config.frame_size = FRAMESIZE_VGA;   // 640x480
    config.jpeg_quality = 10;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.grab_mode = CAMERA_GRAB_LATEST;
  } else {
    Serial.println("   ⚠️  PSRAM not found - using DRAM");
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
    config.fb_location = CAMERA_FB_IN_DRAM;
  }
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed: 0x%x\n", err);
    return;
  }
  
  // ===================================
  // === FIX 3: STABILIZE SENSOR SETTINGS ===
  // Toned down from '2' to '1' to reduce noise
  // ===================================
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 1);      // Was 2
  s->set_contrast(s, 1);        // Was 2
  s->set_saturation(s, 1);      // Was 2
  s->set_sharpness(s, 1);       // Was 2
  s->set_denoise(s, 1);         
  s->set_gainceiling(s, (gainceiling_t)3); 
  s->set_whitebal(s, 1);        
  s->set_awb_gain(s, 1);        
  
  Serial.println("   ✓ Camera optimized for mobile screen detection\n");
}

// ===========================
// WIFI SETUP
// ===========================
void setupWifi() {
  Serial.print("📡 Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✓ WiFi Connected!");
    Serial.print("   IP Address: ");
    Serial.println(WiFi.localIP());
    Serial.print("   Signal: ");
    Serial.print(WiFi.RSSI());
    Serial.println(" dBm\n");
  } else {
    Serial.println("\n❌ WiFi Connection Failed!");
  }
}

// ===========================
// AWS IoT CONNECTION
// ===========================
void connectAWS() {
  net.setCACert(AWS_CERT_CA);
  net.setCertificate(AWS_CERT_CRT);
  net.setPrivateKey(AWS_CERT_PRIVATE);
  
  client.setServer(AWS_IOT_ENDPOINT, 8883);
  client.setCallback(mqttCallback);
  client.setKeepAlive(60);
  
  Serial.print("🔐 Connecting to AWS IoT...");
  
  int attempts = 0;
  while (!client.connected() && attempts < 5) {
    if (client.connect(AWS_IOT_THING_NAME)) {
      Serial.println(" ✓ Connected!");
      
      if (client.subscribe(AWS_IOT_COMMAND_TOPIC)) {
        Serial.print("   ✓ Subscribed to: ");
        Serial.println(AWS_IOT_COMMAND_TOPIC);
      }
    } else {
      Serial.print(".");
      delay(1000);
      attempts++;
    }
  }
  Serial.println();
}

// ===========================
// MQTT CALLBACK
// ===========================
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.println("\n📬 MQTT Message Received:");
  Serial.print("   Topic: ");
  Serial.println(topic);
  
  String message;
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("   Payload: ");
  Serial.println(message);
  
  // Parse JSON
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, message);
  
  if (error) {
    Serial.println("❌ Failed to parse JSON");
    return;
  }
  
  String command = doc["command"] | "";
  
  // ===================================
  // === FIX 4: REMOVED LOGIC CONFLICT ===
  // The emergency state is now ONLY set by the HTTP response,
  // not by this MQTT message. This just logs the receipt.
  // ===================================
  if (command == "EMERGENCY_CLEAR") {
    Serial.println("   ✓ Emergency command received (already handled by HTTP response)");
    // setEmergencyState(); // <-- REMOVED to prevent double-trigger
  }
}

// ===========================
// UPLOAD IMAGE TO AWS
// ===========================
void uploadImage(camera_fb_t * fb) {
  // Check WiFi and reconnect if needed
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("❌ WiFi disconnected - skipping upload");
    // Don't try to reconnect here, just wait for next loop
    return;
  }
  
  Serial.println("📤 Uploading to AWS Lambda...");

  HTTPClient http;
  
  // Use regular HTTP (not HTTPS) - API Gateway public endpoints are HTTP
  http.begin(API_GATEWAY_URL);

  http.addHeader("Content-Type", "image/jpeg");
  http.setTimeout(30000);
  http.setConnectTimeout(10000);
  
  // Retry logic for HTTP upload
  int maxRetries = 3;
  int httpResponseCode = -1;
  
  for (int retry = 0; retry < maxRetries; retry++) {
    if (retry > 0) {
      Serial.print("   ⚠️  Retry ");
      Serial.print(retry);
      Serial.println("/3...");
      delay(2000);
    }
    
    // Send the request
    httpResponseCode = http.POST(fb->buf, fb->len);
    
    if (httpResponseCode == 200) {
      Serial.print("   ✓ HTTP Response: ");
      Serial.println(httpResponseCode);
      
      String response = http.getString();
      Serial.println("   Response:");
      
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, response);
      
      if (!error) {
        String status = doc["status"] | "unknown";
        
        if (status == "emergency_detected") {
          Serial.println("   🚨 EMERGENCY VEHICLE DETECTED!");
          String vehicleType = doc["vehicle_type"] | "Unknown";
          float confidence = doc["confidence"] | 0.0;
          
          Serial.print("   Vehicle: ");
          Serial.println(vehicleType);
          Serial.print("   Confidence: ");
          Serial.print(confidence);
          Serial.println("%");
          
          // This is now the ONLY place the state is set
          setEmergencyState(); 
        } else {
          Serial.println("   ✓ Normal - No emergency");
          
          if (doc["detected_objects"].is<JsonArray>()) {
            Serial.print("   Objects: ");
            JsonArray objects = doc["detected_objects"].as<JsonArray>();
            for (size_t i = 0; i < objects.size() && i < 5; i++) {
              if (i > 0) Serial.print(", ");
              Serial.print(objects[i].as<const char*>());
            }
            Serial.println();
          }
        }
      } else {
         Serial.println("   ❌ Failed to parse JSON response!");
         Serial.println(response);
      }
      http.end();
      break; // Success, exit retry loop
      
    } else if (httpResponseCode > 0) {
      // Server responded with an error (e.g., 400, 500)
      Serial.print("   ⚠️  HTTP Error: ");
      Serial.println(httpResponseCode);
      String response = http.getString();
      Serial.print("   Error Body: ");
      Serial.println(response);
      http.end();
      break; // Don't retry on server errors, it's a permanent fail
      
    } else {
      // Connection level error (e.g., -1, -3)
      Serial.print("   ❌ Connection Error: ");
      Serial.println(httpResponseCode);
      http.end();
      if (retry < maxRetries - 1) {
        Serial.println("   ⏳ Retrying...");
        // Re-initialize http client for next retry
        http.begin(API_GATEWAY_URL);
        http.addHeader("Content-Type", "image/jpeg");
        http.setTimeout(30000);
        http.setConnectTimeout(10000);
      }
    }
  }
  
  if (httpResponseCode < 0 && httpResponseCode != 200) {
      Serial.println("❌ Failed to upload image after all retries.");
  }

  http.end(); // Ensure client is always closed
  Serial.println();
}
