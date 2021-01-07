#include "FirebaseESP8266.h"  // for google firebase cloud
#include <ESP8266WiFi.h>    // for nodemcu
#include <Servo.h>          //servo lib
// below are library files for rfid module MFRC522
#include <deprecated.h>     
#include <MFRC522.h>  
#include <MFRC522Extended.h>
#include <require_cpp11.h>

#define FIREBASE_HOST "https://home-******.firebaseio.com/"     //cloud host url
#define FIREBASE_AUTH "**********************************"  //cloud authentication key for storing data, reading data from cloud etc
#define WIFI_SSID "WIFI HOSTNAME" // wifi id
#define WIFI_PASSWORD "WIFI PASSWORD" // wifi psswrd

FirebaseData firebaseData; //firebaseData object for class FirebaseData. Whenever we need we can use this object access the functions 
//of this class. You will understand ore as you read below

FirebaseJson json; //Here json object for FirebaseJson class. json is the data type we use for firebase. You can ssearch more on JSON online.

void printResult(FirebaseData &data); //function of firebase library to understand more go to this link https://github.com/mobizt/Firebase-ESP8266

Servo servo; //servo object created

MFRC522 mfrc522(D4, D3);  //D4 is the chip select pin for RFID module and D3 is the reset pin

//Garage NodeMCU pins
int garage_servoPin = D6;
int garage_relay_pin = D2;
int garage_vib_pin = D3;
int grg_command;
//Washroom system NodeMCU pins
int wash_pir_pin = D2;
int wash_relay_light = D3;
int wash_relay_fan = D4;
//Room NodeMCU pins
int room_pir_pin = D2;
int room_relay_light = D3;
int room_relay_fan = D4;
//Watering system NodeMCU pins
int moisture_sens = A0;
// Pins A,B and C are for 4051 MUX
int pin_A = D0;
int pin_B = D1;
int pin_C = D2;
int pot1_relay_pin = D3;
int pot2_relay_pin = D4;
int pot3_relay_pin = D5;
//Door system NodeMCU pins
int actuator_unlock_pin = D0;
int actuator_lock_pin = D1;
int door_vib_pin = D2;

struct moisture_data {  // structure to store incoming moisture data from user given from the app 
  int moisture1;
  int moisture2;
  int moisture3;
};

moisture_data data;  //data object created for moisture_data structure

String tagUID = "** ** ** **"; // RFID unique id which can be used for unlocking door. All other keys will result invalid. ex of an UID "32 C3 B9 42", id is always in HEX form

void setup()
{
  Serial.begin(9600);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD); //Init wifi connection with nodemcu
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED)  //If wifi not connected result loop being true hence will print "." until connection established 
  {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP()); // print IP addrs on serial monitor
  Serial.println();

  Firebase.begin(FIREBASE_HOST, FIREBASE_AUTH);   //Init firebase and for more information on below functions go to above given link
  Firebase.reconnectWiFi(true);            
  firebaseData.setBSSLBufferSize(1024, 1024);
  firebaseData.setResponseSize(1024);
  Firebase.setReadTimeout(firebaseData, 1000 * 60);
  Firebase.setwriteSizeLimit(firebaseData, "tiny");

  SPI.begin(); // begin spi connection for RFID module
  mfrc522.PCD_Init();  // Init rfid 

  pinMode(servoPin, OUTPUT);
  pinMode(garage_relay_pin, OUTPUT);
  pinMode(garage_vib_pin, OUTPUT);
  pinMode(wash_relay_light, OUTPUT);
  pinMode(wash_relay_fan, OUTPUT);
  pinMode(wash_pir_pin, INPUT);
  pinMode(room_relay_light, OUTPUT);
  pinMode(room_relay_fan, OUTPUT);
  pinMode(room_pir_pin, INPUT);
  pinMode(moisture_sens, INPUT);
  pinMode(pin_A, OUTPUT);
  pinMode(pin_B, OUTPUT);
  pinMode(pin_C, OUTPUT);
  pinMode(pot1_relay_pin, OUTPUT);
  pinMode(pot2_relay_pin, OUTPUT);
  pinMode(pot3_relay_pin, OUTPUT);
  pinMode(actuator_lock_pin, OUTPUT);
  pinMode(actuator_unlock_pin, OUTPUT);
  pinMode(door_vib_pin, INPUT);
  servo.attach(servoPin);
}


void loop() {

  //This section is for "Watering System Control"
  if (Firebase.getString(firebaseData, "/moisture1"))  //get moisture from path/moisture1 (user will enter the value through app eg i want 52% moisture in pot 1)
  {
    String moisture_data_1 = firebaseData.stringData(); // store above moisture data in var moisture_data_1 
    data.moisture1 = moisture_data_1.substring(2, 4);   // data recieved will be in format /"52/" format hence using substring to store just 52 to moisture1 in structure
    data.moisture1 = data.moisture1.toInt();   // type cast string to int
    //Serial.println(data.moisture2);
  }

  if (Firebase.getString(firebaseData, "/moisture2"))   // same as above just for pot 2
  {
    String moisture_data_2 = firebaseData.stringData();
    data.moisture2 = int(moisture_data_2.substring(2, 4);
    data.moisture2 = data.moisture2.toInt();
    //Serial.println(data.moisture2);
  }

  if (Firebase.getString(firebaseData, "/moisture3")) // ssame as bove just for pot 3
  {
    String moisture_data_3 = firebaseData.stringData();
    data.moisture3 = moisture_data_3.substring(2, 4);
    data.moisture3 = data.moisture3.toInt();
    //Serial.println(data.moisture3);
  }

  read_sens1();     // calling function read_sens1. To know its function go to bottom where all dunctions are given
  pot1_moisture = map(analogRead(A0), 850, 300, 0, 99);  // moisture sensor shows 850 when almost completely dry and 300 when almost completely wet
  // and so mapping total dry 850 to 0% and total wet 300 to 99% NOTE: We will not use 100 percent as it will add more lines of code while using substring and so only two digit nos.
  if (pot1_moisture < data.moisture1) { // pot1_moisture stores data recieving from moisture sensor in the pot 1 and if its below the value assigned by user 
    //then turn on relay to allow water to flow to pot
    digitalWrite(pot1_relay_pin, LOW);
  }
  else {
    digitalWrite(pot1_relay_pin, HIGH); // else turn off relay
  }

  read_sens2(); // refer bottom of the page for function's function  
  pot2_moisture = map(analogRead(A0), 850, 300, 0, 99); // everythings same as above just for pot 2
  if (pot2_moisture < data.moisture2) {
    digitalWrite(pot2_relay_pin, LOW);
  }
  else {
    digitalWrite(pot2_relay_pin, HIGH);
  }

  read_sens3();
  pot3_moisture = map(analogRead(A0), 850, 300, 0, 99); // everythings same as above just for pot 3
  if (pot3_moisture < data.moisture3) {
    digitalWrite(pot3_relay_pin, LOW);
  }
  else {
    digitalWrite(pot3_relay_pin, HIGH);
  }


  //This section is for "Garage Control"
  if (Firebase.getInt(firebaseData, "/Data/garage_control_mode")) { // get control mode from firebase from path /Data/garage_control_mode
    // 1 for unlocked mode and 0 for locked mode. will be set by user through app
    if (firebaseData.intData()) {   // if true (mode = 1) enter loop
      if (Firebase.getInt(firebaseData, "/Data/garage_door")) { // so get user input from path /Data/garage_door whther to open door or not. 1 to open and 0 to close
        grg_command = firebaseData.intData();  // store user command for open/close door to var grg_command
       // Serial.println(firebaseData.intData());
        if (grg_command == 1) { // if data recieved is 1 open door by activating servo
          servo.write(90);
         // Serial.println("Garage open");
          digitalWrite(garage_relay_pin, LOW)
        }
        else if (grg_command == 0) {  //else close door  
          servo.write(0);
         // Serial.println("Garage close");
          digitalWrite(garage_relay_pin, HIGH)
        }
      }
    }

    else {  // if data from path /Data/garage_control_mode is 0 ie locked mode and if
      int vib_stat = digitalRead(garage_vib_pin) // vibration sens reads high 
      delay(2000); // wait for 2 seconds to verify still vibrations are there implying someone is trying to open the door forcefully
      if (digitalRead(garage_vib_pin == vib_stat)) { 
        if (Firebase.setString(firebaseData, "/Data/garage_alert", "HIGH")) // send alert ie HIGH to firebase to path /Data/garage_alert which send alert message to user  
        }
      else {
        if (Firebase.setString(firebaseData, "/Data/garage_alert", "LOW")) //else if no vibrations then send LOW to path Data/garage_alert indicationg everything is safe
        }
    }
  }
  

  //This section for "Washroom Control"
  if (digitalRead(wash_pir_pin) == HIGH) {  //if pir sensor reads HIGH indicating someone is in the room
    digitalWrite(wash_relay_light, LOW);  // turn on lights and fan by activating respective relays
    digitalWrite(wash_relay_fan, LOW);
  }
  else {
    digitalWrite(wash_relay_light, HIGH); // else turn off the relays
    digitalWrite(wash_relay_fan, HIGH);
  }


  //This section is for "Room Control"
  if (digitalRead(room_pir_pin) == HIGH) {  // same as above if pir detects presence 
    digitalWrite(room_relay_light, LOW); // turn on relays
    digitalWrite(room_relay_fan, LOW);
  }
  else {
    digitalWrite(room_relay_light, HIGH); // else turn off relays
    digitalWrite(room_relay_fan, HIGH);
  }


  //This section is for "Door Lock System"

  if ( ! mfrc522.PICC_IsNewCardPresent()) { // check whether a new rfid card is present nearby if not thus condition becomes true(!mfrc522.PICC_IsNewCardPresent()) then 
    return;                                 // return until a new card is discovers
  }                                         // NOTE: !mfrc522.PICC_IsNewCardPresent() returns boolean, TRUE if is card is present else FALSE
                                            // to learn more on this visit https://github.com/miguelbalboa/rfid

  if ( ! mfrc522.PICC_ReadCardSerial()) { // so if a card or cards is/are present select one card 
    return;
  }

  String tag = "";  //create a tag variable to store the id of the card selected and ready to be read
  for (byte i = 0; i < mfrc522.uid.size; i++) // creating and array equal to the size of unique id of the card 
  {
    tag.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));  //keep on adding the byte to above tag variable to create uid of the rfid card
    tag.concat(String(mfrc522.uid.uidByte[i], HEX));  // convert the tag to HEX 
  }
  tag.toUpperCase(); // converts tag string to all uppercase letters 

  if (Firebase.getInt(firebaseData, "/Door_control_mode")) { //read user command from firebase from path /Door_control_mode whether house is in locked or unlocked mode 
    // 1 for unlocked mode that is owner is in the house and 0 for locked mode ie owner is not present at his home
    int door_mode = firebaseData.intData(); // store the door mode ie 1 or 0 to variable door_modee
  }
  if (door_mode == 1) {  // if door_mode is 1 ie user is at his home
    if (tag.substring(1) == tagUID) //if tagid matched the users UID defined at the line 61 veryfying its owner of the house
    {
      digitalWrite(actuator_unlock_pin, LOW); // unlock the door 
      digitalWrite(actuator_lock_pin, HIGH);
    }
    else        // else if some other id than user's
    {
      digitalWrite(actuator_unlock_pin, HIGH);  //do not unlock the door
      digitalWrite(actuator_lock_pin, LOW);
    }
  }
  else {     //but if mode is 0 that is user is not at home
    int door_vib_stat = digitalRead(door_vib_pin) // and vibration sensor detects vibration on the door indicating someone is forcefully trying to open the door
    delay(2000);
    if (digitalRead(door_vib_pin == door_vib_stat)) {    
      if (Firebase.setString(firebaseData, "/door_alert", "HIGH"))   // send and alert signal to user
      }
    else {
      if (Firebase.setString(firebaseData, "/door_alert", "LOW"))  // else if no vibration is happening s=do not send alert signal
      }
  }
}

// NodeMCU has only one analog pin but we need three analog pins for three moisture sensors in three pots 
//so 4501 mux is used here which 8 output lines and obviously 3 select lines (A, B, C)
// C  B  A
// 0  0  0  for output 1
// 0  0  1  for output 2
// 0  1  0  for output 3
// 0  1  1  for output 4......
//............................
// 1  1  1  for output 8 
void read_sens1() {  // to select first output line to which 1 pot moisture sensor is connected  we A, B and C to be 0, 0, 0 respectively
  digitalWrite(pin_A, LOW); // and so we make all three select pins LOW
  digitalWrite(pin_B, LOW);
  digitalWrite(pin_C, LOW);
}

void read_sens2() {  // for to read from moisture sensor in pot 2 we activate 2 line
  digitalWrite(pin_A, HIGH); // A = 1 and B = 0 and C = 0
  digitalWrite(pin_B, LOW);
  digitalWrite(pin_C, LOW);
}

void read_sens3() { // similarly for reading values from sensor in pot 3
  digitalWrite(pin_A, LOW); //we make A = 0, B = 1, C = 0
  digitalWrite(pin_B, HIGH);
  digitalWrite(pin_C, LOW);
}
