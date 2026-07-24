// EchoPath ESP32 Firmware
// Dual Communication: USB Serial + WiFi SoftAP / WebSocket (Port 81)
// Ultrasonic + Buzzer Controller
//
// Reads distance from an HC-SR04 ultrasonic sensor and streams it over BOTH:
//   1. USB Serial as "DIST:<value>"
//   2. WiFi WebSocket (ws://192.168.4.1:81) as "DIST:<value>"
//
// Listens for direction commands (LEFT / RIGHT / CENTER / NONE) from either interface
// and drives a passive piezo buzzer with a non-blocking state machine.
//
// ✅ Compatible with BOTH ESP32 Arduino Core 2.x AND Core 3.x.
// ✅ Dual Communication: USB Serial + WebSocket (SoftAP: "EchoPath_AP")
//
// Wiring:
//   HC-SR04 TRIG → GPIO 5
//   HC-SR04 ECHO → GPIO 18
//   Passive Buzzer → GPIO 4  (use a transistor or direct 3.3V drive)

#include <WiFi.h>
#include <WebSocketsServer.h>

// ----------------------------
// Type Declarations (Placed before Arduino auto-prototype generation)
// ----------------------------
typedef enum {
  PATTERN_NONE,
  PATTERN_LEFT,
  PATTERN_RIGHT,
  PATTERN_CENTER
} Pattern;

#define TRIG_PIN    5
#define ECHO_PIN    18
#define BUZZER_PIN  4
#define LEDC_CHAN   0       // used only on Core 2.x
#define LEDC_BITS   8

// ----------------------------
// Wireless Configuration (SoftAP + WebSockets)
// ----------------------------
const char* AP_SSID = "EchoPath_AP";
const char* AP_PASS = "12345678";       // Minimum 8 characters for WPA2
WebSocketsServer webSocket = WebSocketsServer(81);

// ----------------------------
// Core version shim
// ----------------------------
inline void buzzerTone(uint32_t freq) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(BUZZER_PIN, freq);
#else
  ledcWriteTone(LEDC_CHAN, freq);
#endif
}

inline void buzzerOff() {
  buzzerTone(0);
}

// ----------------------------
// Non-blocking Buzzer Pattern Engine
// ----------------------------
Pattern currentPattern = PATTERN_NONE;
int patternStep = 0;
unsigned long stepStartTime = 0;

void startPattern(Pattern p) {
  currentPattern = p;
  patternStep = 0;
  stepStartTime = millis();
}

void updateBuzzer() {
  if (currentPattern == PATTERN_NONE) {
    buzzerOff();
    return;
  }

  unsigned long elapsed = millis() - stepStartTime;

  if (currentPattern == PATTERN_LEFT) {
    // LEFT = two short beeps (1500 Hz): 200ms tone, 150ms silent, 200ms tone
    if (patternStep == 0) {
      buzzerTone(1500);
      if (elapsed >= 200) { patternStep = 1; stepStartTime = millis(); }
    } else if (patternStep == 1) {
      buzzerOff();
      if (elapsed >= 150) { patternStep = 2; stepStartTime = millis(); }
    } else if (patternStep == 2) {
      buzzerTone(1500);
      if (elapsed >= 200) { patternStep = 3; stepStartTime = millis(); }
    } else {
      buzzerOff();
      currentPattern = PATTERN_NONE;
    }
  } else if (currentPattern == PATTERN_RIGHT) {
    // RIGHT = one long beep (2500 Hz): 500ms tone
    if (patternStep == 0) {
      buzzerTone(2500);
      if (elapsed >= 500) { patternStep = 1; stepStartTime = millis(); }
    } else {
      buzzerOff();
      currentPattern = PATTERN_NONE;
    }
  } else if (currentPattern == PATTERN_CENTER) {
    // CENTER = three rapid beeps (2000 Hz): 120ms tone, 80ms silent (x3)
    if (patternStep == 0 || patternStep == 2 || patternStep == 4) {
      buzzerTone(2000);
      if (elapsed >= 120) { patternStep++; stepStartTime = millis(); }
    } else if (patternStep == 1 || patternStep == 3) {
      buzzerOff();
      if (elapsed >= 80) { patternStep++; stepStartTime = millis(); }
    } else {
      buzzerOff();
      currentPattern = PATTERN_NONE;
    }
  }
}

// ----------------------------
// WebSocket Event Handler
// ----------------------------
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
  if (type == WStype_TEXT) {
    String command = String((char*)payload);
    command.trim();

    // Echo ACK over WebSocket
    String ack = "CMD:" + command;
    webSocket.sendTXT(num, ack);

    // Also output to Serial for monitoring
    Serial.print("WS_CMD:");
    Serial.println(command);

    if      (command == "LEFT")   startPattern(PATTERN_LEFT);
    else if (command == "RIGHT")  startPattern(PATTERN_RIGHT);
    else if (command == "CENTER") startPattern(PATTERN_CENTER);
    else                          startPattern(PATTERN_NONE);
  }
}

// ----------------------------
// Setup
// ----------------------------
void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttach(BUZZER_PIN, 2000, LEDC_BITS);
#else
  ledcSetup(LEDC_CHAN, 2000, LEDC_BITS);
  ledcAttachPin(BUZZER_PIN, LEDC_CHAN);
#endif

  // ── Wireless SoftAP Setup ─────────────────────────────────────────────
  WiFi.softAP(AP_SSID, AP_PASS);
  IPAddress apIP = WiFi.softAPIP();
  Serial.print("EchoPath SoftAP Active — IP: ");
  Serial.println(apIP);

  // ── WebSocket Server Setup (Port 81) ──────────────────────────────────
  webSocket.begin();
  webSocket.onEvent(webSocketEvent);

  Serial.println("EchoPath Ready (Serial + WebSocket Port 81)");
}

// ----------------------------
// Ultrasonic
// ----------------------------
int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH, 30000);  // 30 ms timeout ~ 510 cm max
  if (duration == 0) return -1;
  return (int)(duration * 0.034 / 2);
}

// ----------------------------
// Main Loop
// ----------------------------
unsigned long lastDistPoll = 0;

void loop() {
  unsigned long now = millis();

  // ── Service WebSocket events ──────────────────────────────────────────
  webSocket.loop();

  // ── Stream ultrasonic distance (~10 Hz non-blocking) ────────────────────
  if (now - lastDistPoll >= 100) {
    lastDistPoll = now;
    int dist = getDistance();

    // Stream over USB Serial
    Serial.print("DIST:");
    Serial.println(dist);

    // Broadcast over WebSocket (port 81)
    String distStr = "DIST:" + String(dist);
    webSocket.broadcastTXT(distStr);
  }

  // ── Receive USB Serial command ─────────────────────────────────────────
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();

    // Echo back over Serial
    Serial.print("CMD:");
    Serial.println(command);

    if      (command == "LEFT")   startPattern(PATTERN_LEFT);
    else if (command == "RIGHT")  startPattern(PATTERN_RIGHT);
    else if (command == "CENTER") startPattern(PATTERN_CENTER);
    else                          startPattern(PATTERN_NONE);
  }

  // ── Non-blocking Buzzer Pattern Engine ──────────────────────────────────
  updateBuzzer();
}
