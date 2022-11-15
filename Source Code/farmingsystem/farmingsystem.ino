#include <Servo.h>
Servo myservo;  // create servo object to control a servo
int pos = 0;
#define servo_pin 9

// Thingspeak  
String statusChWriteKey = "LN2ADS20QCM8P8PE";  // Status Channel id: 385184
String canalID1 = "1694447"; //Actuator1
String canalID2 = "1694448"; //Actuator2

#include <SoftwareSerial.h>
SoftwareSerial EspSerial(6, 7); // Rx,  Tx
#define HARDWARE_RESET 8

// DS18B20
#include <OneWire.h>
#include <DallasTemperature.h>
#define ONE_WIRE_BUS 5 // DS18B20 on pin D5 
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
int airTemp = 0;

#define VCC2 2 //5V
#define GND2 3 //ground

//DHT
#include "DHT.h"
#include <stdlib.h>
int pinoDHT = 11;
int tipoDHT =  DHT11;
DHT dht(pinoDHT, tipoDHT); 
int airHum = 0;

// LDR (Light)
#define ldrPIN 10
int light = 0;

// Soil humidity
#define soilHumPIN A0
int soilHum = 0;

//Water level
#define trigpin 12  
#define echopin 13 

// Variables to be used with timers
long writeTimingSeconds = 17; // ==> Define Sample time in seconds to send data
long readTimingSeconds = 10; // ==> Define Sample time in seconds to receive data
long startWriteTiming = 0;
long elapsedWriteTime = 0;

long startReadTiming = 0;
long elapsedReadTime = 0;

// Variables to be used with Actuators
#define ACTUATOR1 A5 //fan
#define ACTUATOR2 A4 //waterpump

//LEDs
#define led1 A1
#define led2 A2
#define led3 A3

int duration, distance;

boolean fan = 0; 
boolean waterp = 0; 

int spare = 0;
boolean error;

void setup()
{
  Serial.begin(9600);
  
  pinMode(ACTUATOR1,OUTPUT);
  pinMode(ACTUATOR2,OUTPUT);
  pinMode(HARDWARE_RESET,OUTPUT);

  pinMode(trigpin, OUTPUT);
  pinMode(echopin, INPUT);
  
  digitalWrite(HARDWARE_RESET, HIGH);
  
  DS18B20.begin();
  dht.begin();
  
  myservo.attach(servo_pin); //servo
  
  pinMode(VCC2,OUTPUT);
  digitalWrite(VCC2, HIGH);
  pinMode(GND2,OUTPUT);
  digitalWrite(GND2, LOW);

  digitalWrite(ACTUATOR1, LOW); //o módulo relé é ativo em LOW
  digitalWrite(ACTUATOR2, LOW); //o módulo relé é ativo em LOW

  //LEDs
  pinMode(led1, OUTPUT); 
  pinMode(led2, OUTPUT);
  pinMode(led3, OUTPUT);

  digitalWrite(led1, LOW); 
  digitalWrite(led2, LOW);
  digitalWrite(led3, LOW);
  
  EspSerial.begin(9600); // Comunicacao com Modulo WiFi
  EspHardwareReset(); //Reset do Modulo WiFi
  startWriteTiming = millis(); // starting the "program clock" 
}

void loop()
{
  

  start: //label 
  error=0;
  
  elapsedWriteTime = millis()-startWriteTiming; 
  elapsedReadTime = millis()-startReadTiming; 
  
  if (elapsedWriteTime > (writeTimingSeconds*1000)) 
  {
    readSensors();
    writeThingSpeak();
    startWriteTiming = millis();   
  }

  if (elapsedReadTime > (readTimingSeconds*1000)) 
  {
    int command = readThingSpeak(canalID1); 
    if (command != 9) fan = command; 
    delay (5000);
    command = readThingSpeak(canalID2); 
    if (command != 9) waterp = command; 
    takeActions();
    startReadTiming = millis();   
  }

  if (airHum >= 64.0){   //độ ẩm trên 64 thì bật quạt
    digitalWrite(ACTUATOR1, HIGH);
    fan=1;
  }
  
  if (airHum < 64.0){
    digitalWrite(ACTUATOR1, LOW);
    fan=0;
  }
  
  if (light >= 0 && light <=0)
  {
    for (pos = 0; pos <= 180; pos += 1) 
    { // goes from 0 degrees to 180 degrees
      // in steps of 1 degree
      myservo.write(pos); // tell servo to go to position in variable 'pos'
      delay(15);          // waits 15ms for the servo to reach the position
    }
  }
  
  if(light >=1 && light <= 1)
  {
    for (pos = 180; pos >= 0; pos -= 1) 
    { // goes from 180 degrees to 0 degrees
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    }
  }

  if(soilHum < 800){
    digitalWrite(ACTUATOR2, LOW); 
    waterp=0;
  }

  if(soilHum > 800){
    digitalWrite(ACTUATOR2, HIGH); 
    waterp=1;
  }

  if(  (distance > 0) && (distance <= 1)   ) 
  {
    digitalWrite(led1, HIGH); 
    digitalWrite(led2, HIGH);
    digitalWrite(led3, HIGH);
  } else
  
  if(  (distance > 1) && (distance <= 3)  ) 
  {
  
    digitalWrite(led1, LOW); 
    digitalWrite(led2, HIGH);
    digitalWrite(led3, HIGH);
  
  } else
  
  if(  (distance > 3) && (distance <= 6)  ) 
  {
  
    digitalWrite(led1, LOW); 
    digitalWrite(led2, LOW);
    digitalWrite(led3, HIGH);
  } 
  
  if (error==1) //Resend if transmission is not completed 
  {       
    Serial.println(" <<<< ERROR >>>>");
    delay (2000);  
    goto start; //go to label "start"
  }
}

/********* Read Sensors value *************/
void readSensors(void)
{
  digitalWrite(trigpin, HIGH);

  delayMicroseconds(1000);  
  digitalWrite(trigpin, LOW);

  duration = pulseIn(echopin,HIGH);
  distance = ( duration / 2) / 29.1;
  
  airHum = dht.readHumidity();

  DS18B20.requestTemperatures(); 
  airTemp = DS18B20.getTempCByIndex(0); // Sensor 0 will capture Soil Temp in Celcius
             
  light = digitalRead(ldrPIN);  
  soilHum = analogRead(soilHumPIN); 
}

/********* Take actions based on ThingSpeak Commands *************/
void takeActions(void)
{
  Serial.print("Fan: ");
  Serial.println(fan);
  Serial.print("Water Pump: ");
  Serial.println(waterp);
  if (fan == 1) digitalWrite(ACTUATOR1, HIGH);
  else digitalWrite(ACTUATOR1, LOW);
  if (waterp == 1) digitalWrite(ACTUATOR2, HIGH);
  else digitalWrite(ACTUATOR2, LOW); 
}

/********* Read Actuators command from ThingSpeak *************/
int readThingSpeak(String channelID)
{
  startThingSpeakCmd();
  int command;
  // preparacao da string GET
  String getStr = "GET /channels/";
  getStr += channelID;
  getStr +="/fields/1/last";
  getStr += "\r\n";

  String messageDown = sendThingSpeakGetCmd(getStr);
  if (messageDown[5] == 49)
  {
    command = messageDown[7]-48; 
    Serial.print("Command received: ");
    Serial.println(command);
  }
  else command = 9;
  return command;
}


/********* Conexao com TCP com Thingspeak *******/
void writeThingSpeak(void)
{

  startThingSpeakCmd();

  // preparacao da string GET
  String getStr = "GET /update?api_key=";
  getStr += statusChWriteKey;
  getStr +="&field1=";
  getStr += String(fan);
  getStr +="&field2=";
  getStr += String(waterp);
  getStr +="&field3=";
  getStr += String(distance);
  getStr +="&field4=";
  getStr += String(airHum);
  getStr +="&field5=";
  getStr += String(airTemp);
  getStr +="&field6=";
  getStr += String(soilHum);
  getStr +="&field7=";
  getStr += String(light);
  getStr +="&field8=";
  getStr += String(spare);
  getStr += "\r\n\r\n";

  sendThingSpeakGetCmd(getStr); 
}

/********* Reset ESP *************/
void EspHardwareReset(void)
{
  Serial.println("Reseting......."); 
  digitalWrite(HARDWARE_RESET, LOW); 
  delay(500);
  digitalWrite(HARDWARE_RESET, HIGH);
  delay(8000);//Tempo necessário para começar a ler 
  Serial.println("RESET"); 
}

/********* Start communication with ThingSpeak*************/
void startThingSpeakCmd(void)
{
  EspSerial.flush();//limpa o buffer antes de começar a gravar
  
  String cmd = "AT+CIPSTART=\"TCP\",\"";
  cmd += "184.106.153.149"; // Endereco IP de api.thingspeak.com
  cmd += "\",80";
  EspSerial.println(cmd);
  Serial.print("enviado ==> Start cmd: ");
  Serial.println(cmd);

  if(EspSerial.find("Error"))
  {
    Serial.println("AT+CIPSTART error");
    return;
  }
}

/********* send a GET cmd to ThingSpeak *************/
String sendThingSpeakGetCmd(String getStr)
{
  String cmd = "AT+CIPSEND=";
  cmd += String(getStr.length());
  EspSerial.println(cmd);
  Serial.print("enviado ==> lenght cmd: ");
  Serial.println(cmd);

  if(EspSerial.find((char *)">"))
  {
    EspSerial.print(getStr);
    Serial.print("enviado ==> getStr: ");
    Serial.println(getStr);
    delay(500);//tempo para processar o GET, sem este delay apresenta busy no próximo comando

    String messageBody = "";
    while (EspSerial.available()) 
    {
      String line = EspSerial.readStringUntil('\n');
      if (line.length() == 1) 
      { //actual content starts after empty line (that has length 1)
        messageBody = EspSerial.readStringUntil('\n');
      }
    }
    Serial.print("MessageBody received: ");
    Serial.println(messageBody);
    return messageBody;
  }
  else
  {
    EspSerial.println("AT+CIPCLOSE");     // alert user
    Serial.println("ESP8266 CIPSEND ERROR: RESENDING"); //Resend...
    spare = spare + 1;
    error=1;
    return "error";
  } 
}
