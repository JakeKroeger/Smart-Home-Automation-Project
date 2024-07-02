#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <Adafruit_Fingerprint.h>
#include <SoftwareSerial.h>
#include <Servo.h>

Servo servoMotor;  // Create a servo object

SoftwareSerial mySerial(10, 11);  // RX, TX pins

Adafruit_Fingerprint finger = Adafruit_Fingerprint(&mySerial);

#define DHTPIN 5  // Change this to the pin you connected the DHT11 SIGNAL pin to.
#define DHTTYPE DHT11

DHT dht(DHTPIN, DHTTYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2);

// GARDEN VARIABLES
int GardenLed1 = 22;
int GardenLed2 = 23;
int rainSensor = A3;

// LIVING ROOM VARIABLES
int doorLock = 4;  //This relay module is switched HIGH and LOW are swtiched

// BEDROOM VARIABLES
int bedroomFan = 25;
bool fanState;
bool windowState = true;


// KITCHEN VARIABLES
int gasSensor = A2;
int buzzerPin = 7;

// OTHER VARIABLES
int pirPin = 8;
bool servoOpened = true;

// Serial Communication Variables------------------------------
String receivedMessage;
bool gardenLightStatus = true;
bool alarmStatus;
bool flameStatus;
bool gasStatus;
bool pirStatus = true;


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);   // Initialize hardware serial for debugging
  Serial1.begin(9600);  // Initialize hardware Serial1 with baud rate 9600

  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(rainSensor, INPUT);

  pinMode(GardenLed1, OUTPUT);
  pinMode(GardenLed2, OUTPUT);
  pinMode(doorLock, OUTPUT);
  pinMode(bedroomFan, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(pirPin, INPUT);
  servoMotor.attach(3);
  servoMotor.write(90);

  lcd.begin(16, 2);  // Adjust the dimensions (16, 2) based on your LCD's specifications.
  lcd.backlight();   // Turn on the backlight.
  dht.begin();

  //FUNCTION TO CHECK FINGERPRINT SENSOR
  finger.begin(57600);
  delay(5);

  if (finger.verifyPassword()) {
    Serial.println("Found fingerprint sensor!");
  } else {
    Serial.println("Did not find fingerprint sensor :(");
  }
}

// FUNCTION TO CHECK IF FINGER IS VALID
uint8_t getFingerprintID() {
  uint8_t p = finger.getImage();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image taken");
      break;
    case FINGERPRINT_NOFINGER:
      Serial.println("No finger detected");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_IMAGEFAIL:
      Serial.println("Imaging error");
      return p;
    default:
      Serial.println("Unknown error");
      return p;

      delay(100);
  }

  // OK success!

  p = finger.image2Tz();
  switch (p) {
    case FINGERPRINT_OK:
      Serial.println("Image converted");
      break;
    case FINGERPRINT_IMAGEMESS:
      Serial.println("Image too messy");
      return p;
    case FINGERPRINT_PACKETRECIEVEERR:
      Serial.println("Communication error");
      return p;
    case FINGERPRINT_FEATUREFAIL:
      Serial.println("Could not find fingerprint features");
      return p;
    case FINGERPRINT_INVALIDIMAGE:
      Serial.println("Could not find fingerprint features");
      return p;
    default:
      Serial.println("Unknown error");
      return p;
  }

  // OK converted!
  p = finger.fingerSearch();
  if (p == FINGERPRINT_OK) {
    Serial.println("Found a print match!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Acces Granted");
    digitalWrite(doorLock, HIGH);
    delay(5000);
    digitalWrite(doorLock, LOW);
  } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
    Serial.println("Communication error");
    return p;
  } else if (p == FINGERPRINT_NOTFOUND) {
    Serial.println("Did not find a match");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Access Denied");
    digitalWrite(doorLock, LOW);
    delay(4000);
    return p;
  } else {
    Serial.println("Unknown error");
    return p;
  }

  // found a match!
  Serial.print("Found ID #");
  Serial.print(finger.fingerID);
  Serial.print(" with confidence of ");
  Serial.println(finger.confidence);

  return finger.fingerID;
}

void loop() {
  // put your main code here, to run repeatedly:
  int ldrValue = analogRead(A0);
  int flameValue = analogRead(A1);
  int gasState = analogRead(gasSensor);
  int rainState = analogRead(rainSensor);
  int pirState = digitalRead(pirPin);
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  //Displaying Temperature on LCD---------------------------
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature);
  lcd.print("C");



  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.print(humidity);
  lcd.print("%");
  delay(100);


  // -------------------------Send data to NodeMCU------------------------
  int temperatureInt = (int)temperature;
  int humidityInt = (int)humidity;


  // Send temperature, humidity, and flame state as integers over Serial1
  Serial1.print("Temperature: ");
  Serial1.print(temperatureInt);

  Serial1.print("Humidity: ");
  Serial1.print(humidityInt);

  Serial1.print("Flame State: ");
  Serial1.print(flameStatus);

  Serial1.print("Gas State: ");
  Serial1.print(gasStatus);

  Serial1.print("Window: ");
  Serial1.println(windowState);



  // //Checking Serial Communication messages-------------------

  if (Serial1.available() > 0) {
    String message = Serial1.readStringUntil('\n');

    if (message.length() > 0) {
      int doorIndex = message.indexOf("Door: ");
      int windowIndex = message.indexOf("Window: ");
      int fanIndex = message.indexOf("Fan: ");
      int pirIndex = message.indexOf("PIR: ");
      int alarmIndex = message.indexOf("Alarm: ");
      int gardenIndex = message.indexOf("Garden: ");

      bool doorStatus = ((message.substring(doorIndex + 6, windowIndex).toInt()) == 1);
      doorStatus == true ? digitalWrite(doorLock, LOW) : digitalWrite(doorLock, HIGH);

      windowState = ((message.substring(windowIndex + 8, fanIndex).toInt()) == 1);

      fanState = ((message.substring(fanIndex + 5, pirIndex).toInt()) == 1);

      pirStatus = ((message.substring(pirIndex + 5, alarmIndex).toInt()) == 1);

      alarmStatus = ((message.substring(alarmIndex + 7, gardenIndex).toInt()) == 1);

      gardenLightStatus = ((message.substring(gardenIndex + 8).toInt()) == 1);

      Serial.print("DOOR is :  ");
      Serial.print(doorStatus);
      Serial.print(" Window is : ");
      Serial.print(windowState);
      Serial.print(" Fan: ");
      Serial.print(fanState);
      Serial.print(" PIR: ");
      Serial.print(pirStatus);
      Serial.print(" AlarmStatus: ");
      Serial.print(alarmStatus);
      Serial.print(" GardenLights: ");
      Serial.println(gardenLightStatus);
    }
  }


  // ---------------FINGERPRINT READING FINGER--------------------
  // Perform fingerprint scanning
  getFingerprintID();
  delay(50);



  //LDR FUNCTION--------------------------------------
  if (ldrValue < 300 && gardenLightStatus == true) {
    digitalWrite(GardenLed1, HIGH);
    digitalWrite(GardenLed2, HIGH);
  } else {
    digitalWrite(GardenLed1, LOW);
    digitalWrite(GardenLed2, LOW);
  }


  //Rain Function---------------------------------------
  if (rainState > 400) {
    Serial.println("Rain detected");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Rain Detected!");
    servoMotor.write(0);
    delay(2000);
  } else {
    if (windowState == true) {
      servoMotor.write(90);
    } else {
      servoMotor.write(0);
    }
    delay(150);
  }

  //Temperature Function-------------------------------
  if (temperature > 25) {
    digitalWrite(bedroomFan, HIGH);
  } else {
    if (fanState == true) {
      digitalWrite(bedroomFan, HIGH);
    } else
      digitalWrite(bedroomFan, LOW);
  }

  //Flame function--------------------------------------
  if (flameValue <= 400) {
    //Buzzer ON
    flameStatus = true;
    Serial.println(flameStatus);
    if (alarmStatus == true)
      digitalWrite(7, HIGH);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Fire Alert!!");
    delay(500);


  } else {
    //Buzzer off
    flameStatus = false;
    digitalWrite(7, LOW);
    lcd.clear();
  }


  //Motion Function--------------------------------------
  if (pirState == HIGH && pirStatus == true) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Motion Alert!!");
    delay(1500);

  } else {

    lcd.clear();
  }



  //GAS Function--------------------------------------
  Serial.println(gasState);
  if (gasState > 700) {
    gasStatus = true;
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Gas Alert!");  // Print your gas alert message
    delay(1000);
  } else {
    gasStatus = false;
    lcd.clear();
  }
}