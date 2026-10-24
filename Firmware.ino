/* =====================================================================
 *  Easy Car v1  -  ESP8266 (NodeMCU) vehicle automation firmware
 * ---------------------------------------------------------------------
 *  Created by : Aziz Ayoujil
 *  Email      : ayoujil.aziz0@gmail.com
 *  Repo       : https://github.com/Ayouil-Aziz/easy_car_v1
 *
 *  The NodeMCU drives a ULN2003 + 5V relay board to control a car's
 *  ignition, accessory/dashboard, door lock/unlock, windows and lights.
 *
 *  Two modes:
 *    - Manual : text / voice commands (EN + FR) from the Android app
 *    - Auto   : start/stop based on Wi-Fi proximity (RSSI)
 *
 *  OTA updates are available on port 81 at /update after the first
 *  USB upload.
 *
 *  WARNING: this controls real vehicle circuits. Test safely, on a car
 *  you own. Set your own Wi-Fi and OTA credentials below before flashing
 *  and never commit real secrets to a public repository.
 * ===================================================================== */

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

/* ----------------------- Configuration ------------------------------ */
// TODO: replace these with your own values before flashing.
const char* WIFI_SSID     = "EasyCar";          // Wi-Fi network to join
const char* WIFI_PASSWORD = "CHANGE_ME";        // Wi-Fi password
const char* MDNS_HOST     = "easycar";          // http://easycar.local
const char* OTA_PATH      = "/update";          // OTA update endpoint
const char* OTA_USERNAME  = "admin";            // CHANGE_ME
const char* OTA_PASSWORD  = "CHANGE_ME";        // CHANGE_ME

/* ----------------------- GPIO pin mapping --------------------------- */
// Outputs -> ULN2003 inputs -> relays
const int PIN_IGNITION = 12;  // D6  - engine start (momentary)
const int PIN_ACC      = 15;  // D8  - accessory / dashboard
const int PIN_LIGHTS   = 13;  // D7  - lights
const int PIN_UNLOCK   = 14;  // D5  - door unlock
const int PIN_LOCK     = 0;   // D3  - door lock
const int PIN_WINDOWS  = 2;   // D4  - power windows
// Input
const int PIN_BUTTON   = 4;   // D2  - physical start button (safety interlock)
// Sensor
const int PIN_NEUTRAL  = 5;   // D1  - hall effect sensor (gear-in-neutral safety)

/* ----------------------- Timing constants --------------------------- */
const unsigned long IGNITION_WARMUP_MS = 7000; // ACC on before cranking
const unsigned long CRANK_MS           = 1000; // starter engage time
const unsigned long PULSE_MS           = 500;  // lock/unlock pulse
const unsigned long WINDOW_TRAVEL_MS   = 2000; // window up/down time
const unsigned long WINDOW_FULL_MS     = 8000; // full window travel
const unsigned long CLIENT_TIMEOUT_MS  = 2000; // http client timeout

/* ----------------------- Globals ------------------------------------ */
ESP8266WebServer        httpServer(81);   // web UI + OTA
WiFiServer              commandServer(80);// raw command channel (app)
ESP8266HTTPUpdateServer httpUpdater;
WiFiClient              client;

String header;
String command = "";

/* Simple web form served on port 81 */
const char MAIN_page[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
  <head><meta name="viewport" content="width=device-width, initial-scale=1"></head>
  <body style="font-family:Helvetica;text-align:center;">
    <h2>Easy Car v1</h2>
    <h3>by Aziz Ayoujil</h3>
    <form action="/action_page">
      Command:<br>
      <input type="text" name="data" value="automode">
      <input type="submit" value="Send">
    </form>
  </body>
</html>
)=====";

/* ===================================================================== */
/*  Helpers                                                              */
/* ===================================================================== */

// Briefly pulse an output (used for lock/unlock/crank).
void pulse(int pin, unsigned long ms) {
  digitalWrite(pin, HIGH);
  delay(ms);
  digitalWrite(pin, LOW);
}

void rebootDevice() {
  delay(1000);
  ESP.reset();
}

// Neutral-safety interlock: returns true when the gear lever is in
// neutral. A digital hall sensor (e.g. A3144) pulls its output LOW when
// the magnet on the gear linkage is near it.
bool inNeutral() {
  return digitalRead(PIN_NEUTRAL) == LOW;
}

/* ===================================================================== */
/*  Car actions                                                          */
/* ===================================================================== */

// Start sequence: only if the physical button is pressed, the car is
// currently off (accessory line LOW), AND the gear is in neutral (hall
// sensor). Turns on ACC, waits, cranks the starter, then unlocks.
void startCar() {
  // Safety: only crank when the button is pressed, the car is off, AND
  // the gearbox is in neutral (hall sensor). Prevents starting in gear.
  if (digitalRead(PIN_BUTTON) == HIGH && digitalRead(PIN_ACC) == LOW && inNeutral()) {
    digitalWrite(PIN_ACC, HIGH);
    delay(IGNITION_WARMUP_MS);
    pulse(PIN_IGNITION, CRANK_MS);
    pulse(PIN_UNLOCK, PULSE_MS);
  }
}

// Stop sequence: only if the button is pressed and the car is on.
// Locks the doors, cuts the accessory line, then reboots the board.
void stopCar() {
  if (digitalRead(PIN_BUTTON) == HIGH && digitalRead(PIN_ACC) == HIGH) {
    pulse(PIN_LOCK, PULSE_MS);
    digitalWrite(PIN_ACC, LOW);
    rebootDevice();
  }
}

// Proximity mode: start/stop based on Wi-Fi signal strength (RSSI).
// Strong signal (near) -> start; weak signal (far) -> stop.
void autoMode() {
  long rssi = WiFi.RSSI();
  Serial.print("RSSI: ");
  Serial.println(rssi);
  delay(500);

  if (rssi <= -20 && rssi >= -70) {        // close
    startCar();
  } else if (rssi <= -80 && rssi >= -90) { // far
    stopCar();
  } else if (rssi >= 0) {                   // invalid / disconnected
    stopCar();
    delay(1000);
    rebootDevice();
  }
}

/* ===================================================================== */
/*  Command channel (Android app, port 80)                              */
/* ===================================================================== */

// Read one command line from the raw TCP client and strip the HTTP verb
// and trailing protocol text, leaving just the command word.
String readCommand() {
  while (!client.available()) delay(1);
  String request = client.readStringUntil('\r');
  request.remove(0, 5);                       // strip "GET /"
  request.remove(request.length() - 9, 9);    // strip " HTTP/1.1"
  return request;
}

void sendBackEcho(const String& echo) {
  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: text/html");
  client.println();
  client.println("<!DOCTYPE HTML><html>" + echo + "</html>");
  client.stop();
  delay(1);
}

// Parse and run a single manual command. Accepts English and French
// keywords (including a few voice-recognition fallbacks).
void handleCommand(const String& cmd) {
  if (cmd == "start" || cmd == "démarrer") {
    startCar();
  } else if (cmd == "stop" || cmd == "arrêter") {
    stopCar();
  } else if (cmd == "unlock" || cmd == "ouvrir" || cmd == "ouvrez") {
    pulse(PIN_UNLOCK, PULSE_MS);
  } else if (cmd == "lock" || cmd == "ferme") {
    pulse(PIN_LOCK, PULSE_MS);
  } else if (cmd == "lighton" || cmd == "light") {
    digitalWrite(PIN_LIGHTS, HIGH);
  } else if (cmd == "lightoff" || cmd == "light off") {
    digitalWrite(PIN_LIGHTS, LOW);
  } else if (cmd == "dashon" || cmd == "dashboard") {
    digitalWrite(PIN_ACC, HIGH);
  } else if (cmd == "dashoff" || cmd == "off") {
    digitalWrite(PIN_ACC, LOW);
  } else if (cmd == "windowup" || cmd == "window") {
    pulse(PIN_WINDOWS, WINDOW_TRAVEL_MS);
  } else if (cmd == "lockandwindows") {
    digitalWrite(PIN_ACC, HIGH);
    pulse(PIN_LOCK, PULSE_MS);
    pulse(PIN_WINDOWS, WINDOW_FULL_MS);
  }
}

// Listen for a manual command from the Android app and act on it.
void manualMode() {
  client = commandServer.available();
  if (!client) return;

  command = readCommand();
  handleCommand(command);
  sendBackEcho(command);
  command = "";
}

/* ===================================================================== */
/*  Web form handlers (port 81)                                         */
/* ===================================================================== */

void handleRoot() {
  httpServer.send(200, "text/html", FPSTR(MAIN_page));
}

void handleForm() {
  String data = httpServer.arg("data");
  httpServer.send(200, "text/html", "OK");
  if (data == "automode") {
    autoMode();
  } else if (data == "manual") {
    manualMode();
  }
}

/* ===================================================================== */
/*  Wi-Fi                                                                */
/* ===================================================================== */

void connectWiFi() {
  Serial.println("Connecting to Wi-Fi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println("\nWi-Fi connected. IP: " + WiFi.localIP().toString());
}

/* ===================================================================== */
/*  Setup / loop                                                         */
/* ===================================================================== */

void setup() {
  Serial.begin(115200);

  const int outputs[] = {PIN_IGNITION, PIN_ACC, PIN_LIGHTS,
                         PIN_UNLOCK, PIN_LOCK, PIN_WINDOWS};
  for (int pin : outputs) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);   // all circuits off at boot
  }
  pinMode(PIN_BUTTON, INPUT);
  pinMode(PIN_NEUTRAL, INPUT);   // hall effect neutral sensor

  WiFi.softAPdisconnect(true);
  connectWiFi();

  commandServer.begin();
  httpServer.on("/", handleRoot);
  httpServer.on("/action_page", handleForm);
  httpServer.begin();

  MDNS.begin(MDNS_HOST);
  httpUpdater.setup(&httpServer, OTA_PATH, OTA_USERNAME, OTA_PASSWORD);
  MDNS.addService("http", "tcp", 81);

  Serial.printf("OTA ready at http://%s.local:81%s\n", MDNS_HOST, OTA_PATH);
}

void loop() {
  httpServer.handleClient();
  handleForm();
  MDNS.update();
  delay(100);
}
