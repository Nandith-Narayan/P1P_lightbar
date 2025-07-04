#include <Secrets.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <MQTT.h>
#include <Arduino_JSON.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 12

#define IDLE 0
#define DOWNLOADING_GCODE 1
#define PREPARING 2
#define PRINTING 3
#define PAUSED 4
#define DONE_PRINTING 5

WiFiClientSecure wifiClient;
MQTTClient mqtt(8192, 128);
Adafruit_NeoPixel leds(50, LED_PIN, NEO_GRB + NEO_KHZ800);

double nozzleTemp = 0;
double nozzleTempTarget = 0;
double bedTemp = 0;
double bedTempTarget = 0;
int progressPercent = 0;
int printStage = 0;
int printSubStage = 0;
double gcodeDownloadProgress = 0;
String printStatus = "";
int printerState = 0;

int currentState = IDLE;

unsigned long bt, at;
unsigned long previousTime, currentTime;

unsigned long start, t;

void setup() {
  // put your setup code here, to run once:
  start = millis();
  previousTime = millis();
  Serial.begin(115200);
  leds.begin();
  leds.clear(); 
  leds.show(); 
  connect();
  requestAllData();
}

void loop() {
  // put your main code here, to run repeatedly:
  //Serial.println(mqtt.connected());
  mqtt.loop();
  if(!mqtt.connected() || WiFi.status() != WL_CONNECTED){
    Serial.println("Attempting to reconnect.");
    connect();
  }

  

  switch(currentState){
    case IDLE:
      displaySolidColor(leds.Color(0, 64, 0));
      break;
    case DOWNLOADING_GCODE:
      displayProgressBar((int)gcodeDownloadProgress,leds.Color(64, 64, 0));
      break;
    case PREPARING:
      displayMovingPixel(leds.Color(0, 0, 64));
      break;
    case PRINTING:
      displayProgressBar(progressPercent,leds.Color(64, 64, 64));
      break;
    case PAUSED:
      displaySolidColor(leds.Color(64, 0, 0));
      break;
    case DONE_PRINTING:
      displayBreathingEffect();
      break;
  }
  

}

int movingEffectVal = 0, movingEffectDir = 1;
void displayMovingPixel(uint32_t color){
  currentTime = millis();
  if(currentTime-previousTime > 20){
    previousTime = millis();
    movingEffectVal += movingEffectDir;
    if(movingEffectVal<0){
      movingEffectVal = 0;
      movingEffectDir = 1;
    }
    if(movingEffectVal>49){
      movingEffectVal = 49;
      movingEffectDir = -1;
    }
  }
  leds.clear(); 
  leds.setPixelColor(movingEffectVal, color);
  leds.show(); 
}

int breathingEffectVal = 64, breathingEffectDir = -1;
void displayBreathingEffect(){
  currentTime = millis();
  if(currentTime-previousTime > 50){
    previousTime = millis();
    breathingEffectVal += breathingEffectDir;
    if(breathingEffectVal<0){
      breathingEffectVal = 0;
      breathingEffectDir = 1;
    }
    if(breathingEffectVal>64){
      breathingEffectVal = 64;
      breathingEffectDir = -1;
    }
  }
  
  leds.clear(); 
  for(int i=0;i<50;i++){
    leds.setPixelColor(i, leds.Color(0, breathingEffectVal, 0));
  }
  leds.show(); 
}

void displayProgressBar(int percentage, uint32_t color){
  
  int val = constrain(percentage/2, 0, 49);
  leds.clear(); 
  for(int i=0;i<=val;i++){
    leds.setPixelColor(i, color);
  }
  leds.show(); 
}

void displaySolidColor(uint32_t color){
  leds.clear(); 
  for(int i=0;i<50;i++){
    leds.setPixelColor(i, color);
  }
  leds.show(); 
}

void requestAllData(){
  String topic = "device/";
  topic += P1P_SERIAL;
  topic += "/request";
  const char* payload =
  "{"
    "\"pushing\": {"
      "\"command\": \"pushall\","
      "\"push_target\": 1,"
      "\"sequence_id\": 1,"
      "\"version\": 1"
    "}"
  "}";
  mqtt.publish(topic, payload);
}

void connect(){
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("Connecting to WiFI");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println("\nConnected to WiFi!");
  
  mqtt.begin(P1P_IP, P1P_MQTT_PORT, wifiClient);
  mqtt.onMessage(processResponse);
  mqtt.setTimeout(2000);
  wifiClient.setInsecure();
  int failCount = 0;
  while(!mqtt.connect("ESP32_Lightbar", P1P_MQTT_USERNAME, P1P_MQTT_PASS)){
    failCount++;
    Serial.printf("Failed to connect to P1P %d times.\n", failCount);
    if(failCount > 2){
      delay(5000);
    }
    if(failCount > 20){
      delay(10000);
    }
    delay(100);
  }

  String topic = "device/";
  topic += P1P_SERIAL;
  topic += "/report";
  mqtt.subscribe(topic);
}

void processResponse(String &topic, String &payload) {
  bt = millis();
  JSONVar json = JSON.parse(payload);
  if(json.hasOwnProperty("print")){
    String txt = JSON.stringify(json["print"]);
    JSONVar innerJson = JSON.parse(txt);
    if(innerJson.hasOwnProperty("bed_temper")){
      bedTemp = (double)innerJson["bed_temper"];
    }
    if(innerJson.hasOwnProperty("bed_target_temper")){
      bedTempTarget = (double)innerJson["bed_target_temper"];
    }
    if(innerJson.hasOwnProperty("nozzle_temper")){
      nozzleTemp = (double)innerJson["nozzle_temper"];
    }
    if(innerJson.hasOwnProperty("nozzle_target_temper")){
      nozzleTempTarget = (double)innerJson["nozzle_target_temper"];
    }
    if(innerJson.hasOwnProperty("mc_print_stage")){
      printStage = atoi(innerJson["mc_print_stage"]);
    }
    if(innerJson.hasOwnProperty("mc_print_sub_stage")){
      printSubStage = (int)innerJson["mc_print_sub_stage"];
    }
    if(innerJson.hasOwnProperty("mc_percent")){
      progressPercent = (int)innerJson["mc_percent"];
    }
    if(innerJson.hasOwnProperty("gcode_file_prepare_percent")){
      gcodeDownloadProgress = atof(innerJson["gcode_file_prepare_percent"]);
    }
    if(innerJson.hasOwnProperty("print_type")){
      printStatus = (String)innerJson["print_type"];
    }
    if(innerJson.hasOwnProperty("stg_cur")){
      printerState = (int)innerJson["stg_cur"];
    }
      
  }

  at = millis();
  Serial.printf("Parsed JSON in %dms.\n", at-bt);
  currentState = determineState();
  printData();
}

int determineState(){
  if(printStage == 1 && printStatus.compareTo("idle") == 0 && nozzleTemp < 100){
    return IDLE;
  }
  if(printStage == 1 && printStatus.compareTo("idle") == 0 && nozzleTemp >= 100){
    return DONE_PRINTING;
  }
  if(printStage == 3){
    return PAUSED;
  }
  if(printStage == 1 && printStatus.compareTo("cloud") == 0){
    return DOWNLOADING_GCODE;
  }
  if(printStage == 2 && printerState != 0 && printStatus.compareTo("cloud") == 0){
    return PREPARING;
  }
  if(printStage == 2 && printerState == 0 && printStatus.compareTo("cloud") == 0){
    return PRINTING;
  }
  return IDLE;
}


int determineStateDebug(){
  t = millis() - start;
  if(t < 3000){
    return IDLE;
  }
  if(t < 5000){
    return DOWNLOADING_GCODE;
  }
  if(t < 10000){
    return PREPARING;
  }
  if(t < 20000){
    return PRINTING;
  }
  if(t < 30000){
    return DONE_PRINTING;
  }
  
  return PAUSED;
  
  
}

void printData(){
  Serial.println("---------------------------------------------");
  Serial.printf("Print progress: %d\%\n", progressPercent);
  Serial.printf("Nozzle Temperature: %.2lf/%.2lf\n",nozzleTemp, nozzleTempTarget);
  Serial.printf("Bed Temperature: %.2lf/%.2lf\n",bedTemp, bedTempTarget);
  Serial.printf("Print Stage: %d-%d\n", printStage, printSubStage);
  Serial.printf("G-code download progress: %.2lf\n", gcodeDownloadProgress);
  Serial.printf("Status: %s\n", printStatus);
  Serial.printf("Printer state: %d\n", printerState);
  printState();
  Serial.println("---------------------------------------------");
}

void printState(){

  Serial.print("STATE: ");
  switch(currentState){
    case IDLE:
      Serial.println("IDLE");
      break;
    case DOWNLOADING_GCODE:
      Serial.println("DOWNLOADING_GCODE");
      break;
    case PREPARING:
      Serial.println("PREPARING");
      break;
    case PRINTING:
      Serial.println("PRINTING");
      break;
    case PAUSED:
      Serial.println("PAUSED");
      break;
    case DONE_PRINTING:
      Serial.println("DONE_PRINTING");
      break;
  }
}