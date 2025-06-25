#include <SoftwareSerial.h>
#include <Wire.h>

#if defined(ESP32)
#include <WiFi.h>
#include <FirebaseESP32.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <FirebaseESP8266.h>
#endif

#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>

#define WIFI_SSID "Teranalysis"
#define WIFI_PASSWORD "admin123"
#define API_KEY "AIzaSyCjaSBOuK3NfSTf7rpA9cU1c1i01fDu88U"
#define DATABASE_URL "soil-quality-5c266-default-rtdb.asia-southeast1.firebasedatabase.app"
#define USER_EMAIL "mgsuico.mentorspire@gmail.com"
#define USER_PASSWORD "soilquality@2024"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

bool signupOK = false;

#define MOIST_PIN A0
#define RO D2
#define DI D3
#define DE D7
#define RE D8

#define LED_PIN D5
#define SWITCH_PIN D4

const byte nitro[] = {0x01,0x03, 0x00, 0x1e, 0x00, 0x01, 0xe4, 0x0c};
const byte phos[] = {0x01,0x03, 0x00, 0x1f, 0x00, 0x01, 0xb5, 0xcc};
const byte pota[] = {0x01,0x03, 0x00, 0x20, 0x00, 0x01, 0x85, 0xc0};

byte values[11];

SoftwareSerial mod(RO, DI);

const int AirValue = 645, WaterValue = 370; //Calibration get the output value if the sesnor is in dry air and when the sensor is in water. Change the value.
int soilMoistureValue = 0, soilmoisturepercent=0;
String isDoneState = "";

bool currentStatus = false; // To store the current status of ON/OFF
bool lastButtonState = HIGH; // To store the previous state of the button
unsigned long lastDebounceTime = 0; // To handle button debounce
unsigned long debounceDelay = 50; // Debounce time in milliseconds

void setup() {
  Serial.begin(9600); 

  mod.begin(4800);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);
  
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  if (Firebase.signUp(&config, &auth, "", "")) {
    signupOK = true;
  }

  config.token_status_callback = tokenStatusCallback;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
  Firebase.setDoubleDigits(5);

  pinMode(RE, OUTPUT);
  pinMode(DE, OUTPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(SWITCH_PIN, INPUT);
  
  delay(500);
}

void loop() {

  byte val1,val2,val3;
  val1 = nitrogen();
  delay(250);
  val2 = phosphorous();
  delay(250);
  val3 = potassium();
  delay(250);

  // Print values to the serial monitor
  Serial.print("Nitrogen: ");
  Serial.print(val1);
  Serial.println(" mg/kg");
  Serial.print("Phosphorous: ");
  Serial.print(val2);
  Serial.println(" mg/kg");
  Serial.print("Potassium: ");
  Serial.print(val3);
  Serial.println(" mg/kg");

  soilMoistureValue = analogRead(MOIST_PIN);
  Serial.println(soilMoistureValue);
  soilmoisturepercent = map(soilMoistureValue, AirValue, WaterValue, 0, 100);
  soilmoisturepercent = constrain(soilmoisturepercent, 0, 100);

  if(soilmoisturepercent >= 100)
  {
    Serial.println("100 %");
  }
  else if(soilmoisturepercent <=0)
  {
    Serial.println("0 %");
  }
  else if(soilmoisturepercent >0 && soilmoisturepercent < 100)
  {
    Serial.print(soilmoisturepercent);
    Serial.println("%");
  }
  
  if (Firebase.ready()) {
    Firebase.setFloat(fbdo, F("/Nitrogen"), val1);
    Firebase.setFloat(fbdo, F("/Phosphorus"), val2);
    Firebase.setFloat(fbdo, F("/Potassium"), val3);
    Firebase.setInt(fbdo, "/Moisture", soilmoisturepercent);

    isDoneState = Firebase.getString(fbdo, "/isDone") ? fbdo.to<const char *>() : "";

    handleSwitch();

    if (isDoneState == "YES" && currentStatus) {
      for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(500);
        digitalWrite(LED_PIN, LOW);
        delay(500);
      }
    }
  }

  delay(2000);
}

byte nitrogen(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(nitro,sizeof(nitro))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
 
byte phosphorous(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(phos,sizeof(phos))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}
 
byte potassium(){
  digitalWrite(DE,HIGH);
  digitalWrite(RE,HIGH);
  delay(10);
  if(mod.write(pota,sizeof(pota))==8){
    digitalWrite(DE,LOW);
    digitalWrite(RE,LOW);
    for(byte i=0;i<7;i++){
    //Serial.print(mod.read(),HEX);
    values[i] = mod.read();
    Serial.print(values[i],HEX);
    }
    Serial.println();
  }
  return values[4];
}

void handleSwitch() {
  int buttonState = digitalRead(SWITCH_PIN);

  if (buttonState == LOW && lastButtonState == HIGH && (millis() - lastDebounceTime) > debounceDelay) {
    lastDebounceTime = millis();

    currentStatus = !currentStatus;
    
    if (currentStatus) {
      Firebase.setString(fbdo, "/Status", "ON");
      digitalWrite(LED_PIN, HIGH);
      Serial.println("Status set to ON");
    } else {
      Firebase.setString(fbdo, "/Status", "OFF");
      Firebase.setString(fbdo, "/isDone", "NO");
      digitalWrite(LED_PIN, LOW);
      Serial.println("Status set to OFF");
    }
  }

  lastButtonState = buttonState;
}
