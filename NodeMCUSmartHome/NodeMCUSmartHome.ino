#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>

// Replace with your WiFi credentials
#define WIFI_SSID "*********"
#define WIFI_PASSWORD "*********"

// Replace with your Firebase project API Key
#define API_KEY "*********"

// Replace with your user email and password
#define USER_EMAIL "*********"
#define USER_PASSWORD "*********"

// Replace with your Firebase Realtime Database URL
#define DATABASE_URL "*********"

SoftwareSerial mySerial(D5, D6);  // RX, TX


// Define Firebase Data object
FirebaseData fbdo;

// Define Firebase authentication
FirebaseAuth auth;

// Define Firebase configuration
FirebaseConfig config;

// BEDROOM VARIABLES
bool bedroomLights;
bool bedroomFan;
int humidity;
int temperature;
int bedroomLED = D2;

// GARDEN VARIABLES
bool gardenLights;

// KITCHEN VARIABLES
bool alarmStatus;
bool flameStatus;
bool gasStatus;
bool kitchenLights;
int kitchenLED = D1;

// LIVING ROOM VARIABLES
bool doorLock;
bool livingLights;
bool windowState;
int livingLED = D3;

bool pirState;
bool ledStatus;
bool doorLockStatus;
bool livingRoomLightStatus;

void setup() {
  Serial.begin(9600);    // Initialize hardware serial for debugging
  mySerial.begin(9600);  // Initialize software serial with baud rate 115200 (Read from Arduino)

  pinMode(kitchenLED, OUTPUT);
  pinMode(bedroomLED, OUTPUT);
  pinMode(livingLED, OUTPUT);

  //Wifi Connection Setup
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  // Initialize Firebase
  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.database_url = DATABASE_URL;

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}


int firebaseCounter = 0;

void loop() {
  if (firebaseCounter % 2 == 0) {
    if (Firebase.ready()) {

      // READING BEDROOM VALUES -----------------------------------
      Firebase.RTDB.getBool(&fbdo, "/Bedroom/BedroomLights");
      bedroomLights = fbdo.boolData();

      Firebase.RTDB.getBool(&fbdo, "/Bedroom/Fan");
      bedroomFan = fbdo.boolData();

      // READING GARDEN VALUES
      Firebase.RTDB.getBool(&fbdo, "/Garden/GardenLights");
      gardenLights = fbdo.boolData();

      // READING KITCHEN VALUES
      Firebase.RTDB.getBool(&fbdo, "/Kitchen/KitchenLights");
      kitchenLights = fbdo.boolData();

      // READING LIVING ROOM VALUES
      Firebase.RTDB.getBool(&fbdo, "/LivingRoom/LivingLights");
      livingLights = fbdo.boolData();

      Firebase.RTDB.getBool(&fbdo, "/LivingRoom/DoorLock");
      doorLock = fbdo.boolData();

      Firebase.RTDB.getBool(&fbdo, "/LivingRoom/Window");
      windowState = fbdo.boolData();

      // READING SETTINGS VALUES
      Firebase.RTDB.getBool(&fbdo, "/Settings/AlarmStatus");
      alarmStatus = fbdo.boolData();

      Firebase.RTDB.getBool(&fbdo, "/Settings/pirState");
      pirState = fbdo.boolData();

      // END OF READING VALUES-------------------------------------
    }
  }

  // Increment the counter
  firebaseCounter++;
  if (firebaseCounter >= 2) {
    firebaseCounter = 0;
  }

  //  EVALUATE LIVING INFORMATION
  if (livingLights) {
    digitalWrite(livingLED, HIGH);
  } else {
    digitalWrite(livingLED, LOW);
  }

  delay(100);


  mySerial.print("Door: ");
  mySerial.print(doorLock);

  mySerial.print("Window: ");
  mySerial.print(windowState);

  mySerial.print("Fan: ");
  mySerial.print(bedroomFan);

  mySerial.print("PIR: ");
  mySerial.print(pirState);

  mySerial.print("Alarm: ");
  mySerial.print(alarmStatus);

  mySerial.print("Garden: ");
  mySerial.println(gardenLights);



  if (bedroomLights) {
    digitalWrite(bedroomLED, HIGH);
  } else {
    digitalWrite(bedroomLED, LOW);
  }

  // EVALUATE KITCHEN INFORMATION
  if (kitchenLights) {
    digitalWrite(kitchenLED, HIGH);
  } else {
    digitalWrite(kitchenLED, LOW);
  }


  // SERIAL COMMUNICATION BETWEEN ARDUINO AND NODEMCU-----------------
  if (mySerial.available() > 0) {
    // Read data from serial until newline character
    String data = mySerial.readStringUntil('\n');


    if (data.length() > 0) {
      // Parse the received data
      int temperatureIndex = data.indexOf("Temperature: ");
      int humidityIndex = data.indexOf("Humidity: ");
      int flameIndex = data.indexOf("Flame State: ");
      int gasIndex = data.indexOf("Gas State: ");


      // Extract temperature, humidity, and flame state
      int temperature = data.substring(temperatureIndex + 13, humidityIndex).toInt();
      int humidity = data.substring(humidityIndex + 9, flameIndex).toInt();
      int flameStatusInt = data.substring(flameIndex + 13).toInt();
      int gasStatusInt = data.substring(gasIndex + 11).toInt();


      // Convert flame state string to boolean
      bool flameState = (flameStatusInt == 1);
      bool gasState = (gasStatusInt == 1);


      // Print the received data
      Serial.print("Received: ");
      Serial.print("Temperature: ");
      Serial.print(temperature);
      Serial.print("°C, Humidity: ");
      Serial.print(humidity);
      Serial.print("%, Flame State: ");
      Serial.print(flameState);
      Serial.print(" Gas State: ");
      Serial.println(gasState);

      // You can proceed to use the received data here
      // For example, sending it to Firebase
      Firebase.RTDB.setInt(&fbdo, "/Bedroom/Humidity", humidity);
      Firebase.RTDB.setInt(&fbdo, "/Bedroom/Temperature", temperature);
      Firebase.RTDB.setBool(&fbdo, "/Kitchen/FlameStatus", flameState);
      Firebase.RTDB.setBool(&fbdo, "/Kitchen/GasStatus", gasState);
    }
  }
}
