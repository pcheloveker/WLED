#include "wled.h"

// AAA Fire Effect: two red fronts moving from opposite ends towards each other
static uint16_t mode_aaa_fire(void) {
  static uint16_t leftPos = 0;  // Static variable to store position
  static uint16_t rightPos = 0;
  static uint32_t lastReset = 0;  // Track last reset time
  
  // Check if we need to reset (collision detected)
  if ((uint32_t)(leftPos + 10) >= (uint32_t)(SEGLEN - rightPos)) {
    if (strip.now - lastReset > 500) {  // 500ms pause
      leftPos = 0;
      rightPos = 0;
      lastReset = strip.now;
    }
  }
  
  SEGMENT.fadeToBlackBy(25);  // Smooth fading

  // Left front (10 LEDs)
  for (uint16_t i = leftPos; i < leftPos + 10; i++) {
    if (i < SEGLEN) SEGMENT.setPixelColor(i, RED);  // Red color
  }

  // Right front (10 LEDs from the end)
  for (int16_t i = (int16_t)SEGLEN - rightPos - 1; i >= (int16_t)SEGLEN - rightPos - 10; i--) {
    if (i >= 0) SEGMENT.setPixelColor((uint16_t)i, RED);
  }

  leftPos++;
  rightPos++;

  return 10;  // Delay between frames (ms)
}

static const char _data_FX_MODE_AAA_FIRE[] PROGMEM = "AAA Fire@!;Speed;Intensity;Custom1;Custom2;";

/////////////////////
//  UserMod Class  //
/////////////////////

class AaaFireEffectUsermod : public Usermod {
 private:
  bool enabled = true;
  bool buttonPressed = false;
  uint32_t lastButtonPress = 0;
  uint32_t effectStartTime = 0;
  uint8_t previousEffect = 0;
  bool effectActive = false;
  uint32_t effectDuration = 20000; // 20 seconds in milliseconds
  uint8_t aaaFireEffectId = 255; // Will be set to actual ID after adding effect
  
  // Button pin configuration
  int buttonPin = 0; // GPIO0, can be changed
  
 public:
  void setup() override {
    // Add the AAA Fire effect and get its actual ID
    aaaFireEffectId = strip.addEffect(255, &mode_aaa_fire, _data_FX_MODE_AAA_FIRE);
    
    // Debug: print the actual effect ID
    DEBUG_PRINT(F("AAA Fire effect ID: "));
    DEBUG_PRINTLN(aaaFireEffectId);
    
    // Setup button pin
    pinMode(buttonPin, INPUT_PULLUP);
  }
  
  void loop() override {
    if (!enabled) return;
    
    // Check button state
    bool currentButtonState = !digitalRead(buttonPin); // Inverted because of pullup
    
    // Detect button press (rising edge)
    if (currentButtonState && !buttonPressed) {
      buttonPressed = true;
      lastButtonPress = millis();
      
      // Start AAA Fire effect
      startAaaFireEffect();
    }
    
    // Detect button release (falling edge)
    if (!currentButtonState && buttonPressed) {
      buttonPressed = false;
    }
    
    // Check if effect should end
    if (effectActive && (millis() - effectStartTime > effectDuration)) {
      stopAaaFireEffect();
    }
  }
  
  void startAaaFireEffect() {
    if (effectActive) return; // Already active
    
    // Store current effect
    previousEffect = strip.getSegment(0).mode;
    
    // Activate AAA Fire effect using the correct ID
    strip.getSegment(0).setMode(aaaFireEffectId);
    
    effectStartTime = millis();
    effectActive = true;
    
    // Log the activation
    DEBUG_PRINTLN(F("AAA Fire effect activated"));
  }
  
  void stopAaaFireEffect() {
    if (!effectActive) return;
    
    // Restore previous effect
    strip.getSegment(0).setMode(previousEffect);
    
    effectActive = false;
    
    // Log the deactivation
    DEBUG_PRINTLN(F("AAA Fire effect deactivated"));
  }
  
  // Handle API requests
  void connected() override {
    // Add API endpoints when WiFi is connected
    server.on(F("/aaa_fire"), HTTP_POST, [this](AsyncWebServerRequest *request) {
      startAaaFireEffect();
      request->send(200, "application/json", "{\"status\":\"AAA Fire effect activated\"}");
    });
    
    server.on(F("/aaa_fire"), HTTP_DELETE, [this](AsyncWebServerRequest *request) {
      stopAaaFireEffect();
      request->send(200, "application/json", "{\"status\":\"AAA Fire effect deactivated\"}");
    });
    
    server.on(F("/aaa_fire"), HTTP_GET, [this](AsyncWebServerRequest *request) {
      char response[200];
      snprintf(response, sizeof(response), 
              "{\"active\":%s,\"duration\":%u,\"remaining\":%u}",
              effectActive ? "true" : "false",
              (unsigned int)effectDuration,
              effectActive ? (unsigned int)(effectDuration - (millis() - effectStartTime)) : 0);
      request->send(200, "application/json", response);
    });
  }
  
  uint16_t getId() override { return USERMOD_ID_UNSPECIFIED; }
};

static AaaFireEffectUsermod aaa_fire_effect;
REGISTER_USERMOD(aaa_fire_effect);
