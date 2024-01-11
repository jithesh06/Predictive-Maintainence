#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include "addons/TokenHelper.h"
// Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

#include <Filters.h>

// Insert your network credentials
#define WIFI_SSID "Vivo V29"
#define WIFI_PASSWORD "123456789"

// Insert Firebase project API Key
#define API_KEY "AIzaSyBH9icBuimosf0z8CYLVqPidjVFlTcEdpY"

// Insert RTDB URL
#define DATABASE_URL "https://predictive-maintainence-1841d-default-rtdb.firebaseio.com/"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int count = 0;
bool signupOK = false;

float testFrequency = 50; // test signal frequency (Hz)
int res=0;
float windowLength = 40.0/testFrequency; //how long to average the signal, for statistist
int sensor=0; //Sensor analog input, here it's AD
int therm=0;
int oil = 0;
int voltz = 0;
float intercept=-0.04; // to be adjusted based on calibration testing
const int sensorIn = 32;      // pin where the OUT pin from sensor is connected on Arduino
int mVperAmp = 185;           // this the 5A version of the ACS712 -use 100 for 20A Module and 66 for 30A Module
int Watt = 0;
double Voltage = 0;
double VRMS = 0;
double AmpsRMS = 0;
int oils;
int r;

float slope= 0.0405; // to be adjusted based on calibration testing

float current_volts; // Voltage
int temp=0;
unsigned long printPeriod = 1000; //Refresh rate

unsigned long previousMillis = 0;

void setup() {
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  } else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 100 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();

    sensor=analogRead(32); // read the analog in value:
    therm = analogRead(33);
    oils = analogRead(34);
    voltz = analogRead(35); 
 
    Voltage = getVPP();
    VRMS = (Voltage/2.0) *0.707;   //root 2 is 0.707
    AmpsRMS = ((VRMS * 1000)/mVperAmp)-0.6; //0.3 is the error I got for my sensor
 
    Serial.print(AmpsRMS);
    Serial.print(" Amps RMS  ---  ");
    
    Watt = (AmpsRMS*240/1.2);
    Serial.print(Watt);
    Serial.println(" Watts");
    
    Serial.println(therm); //Displays the value
    temp = map(therm, 4095, 100, 29, 70);
    Serial.println(temp); //Displays the value

    //oils = analogRead(oil);
    Serial.println(oils);

    if(oils <= 100){
      r = 0;
    }

    else if(oils >=120 && oils <=200){
      r = 1;
    }

    else if(oils >=250){
      r = 2;
    }

    // Upload the analog sensor values to the Firebase Realtime Database
    if (Firebase.RTDB.setInt(&fbdo, "CURRENT", AmpsRMS) &&
        Firebase.RTDB.setInt(&fbdo, "VOLTAGE", voltz) &&
        Firebase.RTDB.setInt(&fbdo, "OIL VISCOSITY", r) &&
        Firebase.RTDB.setInt(&fbdo, "TEMPERATURE", temp)) {
      Serial.println("Data uploaded successfully");
    } else {
      Serial.println("Failed to upload data");
      Serial.println("REASON: " + fbdo.errorReason());
    }

    count++;
  }
}

float getVPP()
{
  float result;
  int readValue;                // value read from the sensor
  int maxValue = 0;             // store max value here
  int minValue = 4096;          // store min value here ESP32 ADC resolution
  
   uint32_t start_time = millis();
   while((millis()-start_time) < 1000) //sample for 1 Sec
   {
       readValue = analogRead(sensorIn);
       // see if you have a new maxValue
       if (readValue > maxValue) 
       {
           //record the maximum sensor value/
           maxValue = readValue;
       }
       if (readValue < minValue) 
       {
           //record the minimum sensor value/
           minValue = readValue;
       }
   }
   
   // Subtract min from max
   result = ((maxValue - minValue) * 3.3)/4096.0; //ESP32 ADC resolution 4096
      
   return result;
 }
