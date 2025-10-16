#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>
// -------- SENSOR ULTRASSÔNICO --------
const int trigPin = 32;
const int echoPin = 33;
#define SOUND_SPEED 0.034
#define CM_TO_INCH 0.393701
long duration;
float distanceCm;
float distanceInch;
// -------- MOTOR CONTROL --------
struct MOTOR_PINS {
  int pinEn;
  int pinIN1;
  int pinIN2;
};
std::vector<MOTOR_PINS> motorPins = {
  {22, 26, 27},  // RIGHT_MOTOR
  {23, 14, 12}   // LEFT_MOTOR
};
#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0
#define RIGHT_MOTOR 0
#define LEFT_MOTOR 1
#define FORWARD 1
#define BACKWARD -1
const int PWMFreq = 1000;
const int PWMResolution = 8;
const int PWMSpeedChannel = 4;
void rotateMotor(int motorNumber, int motorDirection) {
  digitalWrite(motorPins[motorNumber].pinIN1, motorDirection == FORWARD ? HIGH : LOW);
  digitalWrite(motorPins[motorNumber].pinIN2, motorDirection == BACKWARD ? HIGH : LOW);
  if (motorDirection == STOP) {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);  }}
void moveCar(int inputValue) {
  switch (inputValue) {
    case UP: rotateMotor(RIGHT_MOTOR, FORWARD); rotateMotor(LEFT_MOTOR, FORWARD); break;
    case DOWN: rotateMotor(RIGHT_MOTOR, BACKWARD); rotateMotor(LEFT_MOTOR, BACKWARD); break;
    case LEFT: rotateMotor(RIGHT_MOTOR, FORWARD); rotateMotor(LEFT_MOTOR, BACKWARD); break;
    case RIGHT: rotateMotor(RIGHT_MOTOR, BACKWARD); rotateMotor(LEFT_MOTOR, FORWARD); break;
    case STOP:
    default:
      rotateMotor(RIGHT_MOTOR, STOP); rotateMotor(LEFT_MOTOR, STOP); break;
  }}

// -------- PWM LED (SEPARADO DO MOTOR) --------
const int ledPWMChannel = 5;
const int ledPin = 25;  // Pino separado para LED PWM
const int ledFreq = 5000;
const int ledResolution = 8;
// -------- WiFi + WebSocket --------
const char* ssid = "MyWiFiCar";
const char* password = "12345678";
AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");
const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
// ... [HTML completo do controle remoto aqui, idêntico ao seu código original] ...
)HTMLHOMEPAGE";
void handleRoot(AsyncWebServerRequest *request) {
  request->send_P(200, "text/html", htmlHomePage);
}
void handleNotFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "File Not Found");}
void onCarInputWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_DATA) {
    AwsFrameInfo info = (AwsFrameInfo)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT){
      std::string myData((char*)data, len);
      std::istringstream ss(myData);
      std::string key, value;
      std::getline(ss, key, ',');
      std::getline(ss, value, ',');
      int valueInt = atoi(value.c_str());
      if (key == "MoveCar") moveCar(valueInt);
      else if (key == "Speed") {
        ledcWrite(PWMSpeedChannel, valueInt);  // Motor speed
        ledcWrite(ledPWMChannel, valueInt);    // LED brightness   }    }
  } else if (type == WS_EVT_DISCONNECT) {
    moveCar(STOP);  }}
void setUpPinModes() {
  ledcSetup(PWMSpeedChannel, PWMFreq, PWMResolution);
  ledcSetup(ledPWMChannel, ledFreq, ledResolution);
  ledcAttachPin(motorPins[0].pinEn, PWMSpeedChannel);
  ledcAttachPin(motorPins[1].pinEn, PWMSpeedChannel);
  ledcAttachPin(ledPin, ledPWMChannel);
  for (auto m : motorPins) {
    pinMode(m.pinIN1, OUTPUT);
    pinMode(m.pinIN2, OUTPUT);  }
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  moveCar(STOP);}
void setup() {
  Serial.begin(115200);
  setUpPinModes();
  WiFi.softAP(ssid, password);
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);
  server.begin();}
void loop() {
  wsCarInput.cleanupClients();
  // SENSOR ULTRASSÔNICO
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distanceCm = duration * SOUND_SPEED / 2;
  distanceInch = distanceCm * CM_TO_INCH;
  Serial.print("Distância (cm): ");
  Serial.println(distanceCm);
  delay(1000);  // Pode ajustar para leitura mais frequente
}
