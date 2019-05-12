// Version v0.1 - SPECSensor NO2 data logger
// This code works with sensor firmware 15SEP17 and Flip&Click SAM3X
// In conjunction with electronza.com and seetheair.wordpress.com

// include the SD library:
#include <SPI.h>
#include <SD.h>

#define LED_debug true
#define LED_A 38
#define LED_B 37
#define LED_C 39
#define LED_D 40

// #define SERIAL1_RX_BUFFER_SIZE 256
// #define SERIAL1_TX_BUFFER_SIZE 256

// SD card

// set up variables using the SD utility library functions:
Sd2Card card;
SdVolume volume;
SdFile root;

// change this to match your SD shield or module;
// Flip & Click with SD click
// in mikroBUS socket D
// CS is PB23 = Arduino 78
// See https://www.arduino.cc/en/Hacking/PinMappingSAM3X
const int chipSelect = 78;

// Sensor values
  // The format of the output is: SN[XXXXXXXXXXXX], PPB [0 : 999999], TEMP [-99:99],
  // RH[0:99], RawSensor[ADCCount], TempDigital, RHDigital, Day[0:99], Hour [0:23]
  // Note that on Arduino Due integer variable (int)  stores a 32-bit (4-byte) value. 
  // This yields a range of -2,147,483,648 to 2,147,483,647 
  // (minimum value of -2^31 and a maximum value of (2^31) - 1). 
  // On 8 bit boards some readings have to be recorded as floats

String SensorSerialNo; 
int NO2;
int Temperature;
int RH;
int RawSensor;
int TempDigital;
int RHDigital;
int Days;
int Hours;
int Minutes;
int Seconds;

const int send_delay = 10000;
unsigned int count = 1;

#define command_delay 500
#define start_delay 2500
String dataString = "";
String responseString = "";
boolean dataStringComplete = 0;
char inChar;

void setup() {

  if (LED_debug){
    pinMode(LED_A, OUTPUT);
    pinMode(LED_B, OUTPUT);
    pinMode(LED_C, OUTPUT);
    pinMode(LED_D, OUTPUT);
  }
  
  Serial.begin(9600);
  // Wait for serial port
  while(!Serial);

// SPI: first begin, then set parameters
  SPI.begin();

  Serial.print("Initializing SD card...");
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while(1);
  }
  // Flash the led if card is initialized
  //digitalWrite(LED_PIN,HIGH);
  //delay(500);
  //digitalWrite(LED_PIN,LOW);
  Serial.println("SD card initialized.");

  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  // digitalWrite(LED_PIN,HIGH); //LED will stay on while we access the SD card

 File dataFile = SD.open("DataLOG.csv", FILE_WRITE);

  Serial.print("Writing to file: " );
  Serial.println("DataLOG.csv");
  dataFile.println("");
  dataFile.println("Count,GAS(ppb),GAS_ADC,Temperature,Humidity,Days,Hours,Minutes,Seconds");
  dataFile.close();
  
  Serial.println("SPEC DGS-NO2 sensor init...");
  Serial.println("");
  Serial1.begin(9600);
  // Normally, data is returned within one second
  Serial1.setTimeout(1500);
  // reserve 80 bytes for the dataString
  dataString.reserve(80);
  responseString.reserve(150);
  
  // Wait for sensor 
  delay(500);
  flush_serial1();
  Serial.println("Resetting sensor");
  SPEC_reset();
  
  // Print firmware version
  SPEC_show_firmware(); 

  // EEPROM dump
  SPEC_dump_EEPROM();
  Serial.println(" ");
  Serial.println("STARTING MEASUREMENTS");
  Serial.println(" ");
}  



void loop() {
  // Do a readout every 10 seconds
  SPEC_Data_read();
  SPEC_parse_data();
  SPEC_print_data();
  SD_logData();
  delay(send_delay);
}

/* ********************************************************************************
 * This function triggers one measurement and receives the data from the sensor
 **********************************************************************************/
void SPEC_Data_read(){
  if (LED_debug){
    digitalWrite(LED_A, HIGH);
  }
  // First, we do some initialization
  // dataStringComplete is set as "false", as we don't have any valid data received
  dataStringComplete = 0;
  // Clear the data string
  dataString = "";
  // Now we trigger a measurement
  //Serial1.print(" ");
  Serial1.print("\r");
  // We wait for the sensor to respond
  dataString = Serial1.readStringUntil('\n');
  //Serial.println(dataString);
  if (LED_debug){
    digitalWrite(LED_A, LOW);
  }
}

/* ********************************************************************************
 * This function takes the received string and upodates sensor data
 **********************************************************************************/
void SPEC_parse_data(){
  // Parses the received dataString
  // Data string is comma separated
  // The format of the output is: SN[XXXXXXXXXXXX], PPB [0 : 999999], TEMP [-99:99],
  // RH[0:99], RawSensor[ADCCount], TempDigital, RHDigital, Day[0:99], Hour [0:23],
  // Minute[0:59], Second[0 : 59]\r\n
  // Take a look also at
  // https://stackoverflow.com/questions/11068450/arduino-c-language-parsing-string-with-delimiter-input-through-serial-interfa
  // We look first for the SN
  int idx1 = dataString.indexOf(',');
  SensorSerialNo = dataString.substring(0, idx1);
  int idx2 = dataString.indexOf(',', idx1 + 1);
  // Hint: after comma there's a space - it should be ignored
  String S_gas = dataString.substring(idx1 + 2, idx2);
  NO2 = S_gas.toInt();
  int idx3 = dataString.indexOf(',', idx2 + 1);
  String S_temp = dataString.substring(idx2 + 2, idx3);
  Temperature = S_temp.toInt();
  int idx4 = dataString.indexOf(',', idx3 + 1);
  String S_humi = dataString.substring(idx3 + 2, idx4);
  RH = S_humi.toInt();
  int idx5 = dataString.indexOf(',', idx4 + 1);
  String S_raw_gas = dataString.substring(idx4 + 2, idx5);
  RawSensor = S_raw_gas.toInt();
  int idx6 = dataString.indexOf(',', idx5 + 1);
  String S_Tdigital = dataString.substring(idx5 + 2, idx6);
  TempDigital = S_Tdigital.toInt();
  int idx7 = dataString.indexOf(',', idx6 + 1);
  String S_RHdigital = dataString.substring(idx6 + 2, idx7);
  RHDigital = S_RHdigital.toInt();
  int idx8 = dataString.indexOf(',', idx7 + 1);
  String S_Days = dataString.substring(idx7 + 2, idx8);
  Days = S_Days.toInt();
  int idx9 = dataString.indexOf(',', idx8 + 1);
  String S_Hours = dataString.substring(idx8 + 2, idx9);
  Hours = S_Hours.toInt();
  int idx10 = dataString.indexOf(',', idx9 + 1);
  String S_Minutes = dataString.substring(idx9 + 2, idx10);
  Minutes = S_Minutes.toInt();
  int idx11 = dataString.indexOf('\r');
  String S_Seconds = dataString.substring(idx10 + 2, idx11);
  Seconds = S_Seconds.toInt();
}


/* ********************************************************************************
 * This function prints the sensor data
 **********************************************************************************/
void SPEC_print_data(){
  Serial.println("********************************************************************");
  Serial.print ("Sensor Serial No. is ");
  Serial.println (SensorSerialNo);
  Serial.print ("Count Number: ");
  Serial.println (count);
  Serial.print ("NO2 level is ");
  Serial.print (NO2);
  Serial.println (" ppb");
  Serial.print ("Temperature is ");
  Serial.print (Temperature, DEC);
  Serial.println (" deg C");
  Serial.print ("Humidity is ");
  Serial.print (RH, DEC);
  Serial.println ("% RH");
  Serial.print ("Sensor is online since: ");
  Serial.print (Days, DEC);
  Serial.print (" days, ");
  Serial.print (Hours, DEC);
  Serial.print (" hours, ");
  Serial.print (Minutes, DEC);
  Serial.print (" minutes, ");
  Serial.print (Seconds, DEC);
  Serial.println (" seconds");
  Serial.println ("Raw Sensor Data");    
  Serial.print ("Raw gas level: ");
  Serial.println (RawSensor);
  Serial.print ("Temperature digital: ");
  Serial.println (TempDigital);
  Serial.print ("Humidity digital: ");
  Serial.println (RHDigital);
  Serial.println ("");
}


/* ********************************************************************************
 * EEPROM dump
 **********************************************************************************/
void SPEC_dump_EEPROM(){
  Serial1.print("e");
  dataString = Serial1.readStringUntil('\n');
  // You can uncomment this line if you wish
  //Serial.println(dataString);
  for (int i=0; i<20; i++){ 
    responseString = Serial1.readStringUntil('\n');
    Serial.println(responseString);
  }   
}  

void SPEC_reset(){
  Serial1.print("r");
  delay(1000);
}  

void SPEC_show_firmware(){
  Serial1.print("f");
  responseString = Serial1.readStringUntil('\n');
  Serial.print("Sensor firmware version is ");
  Serial.println(responseString);
  Serial.println("");
  delay(400);
}  

void flush_serial1(){
  // Do we have data in the serial buffer?
  // If so, flush it
  if (Serial1.available() > 0){
    Serial.println ("Flushing serial buffer...");
    while(1){
      inChar = (char)Serial1.read();
      delay(10);
      Serial.print(inChar);
      if (inChar == '\n') break; 
    }
    Serial.println (" ");
    Serial.println ("Buffer flushed!");
  }
}

void SD_logData(){
   String logString = "";
   logString += String(count);
   logString += ',';
   logString += String(NO2);
   logString += ',';
   logString += String(RawSensor);
   logString += ',';
   logString += String(Temperature);
   logString += ',';
   logString += String(RH);
   logString += ',';
   logString += String(Days);
   logString += ',';
   logString += String(Hours);
   logString += ',';
   logString += String(Minutes);
   logString += ',';
   logString += String(Seconds);
   count++;
   // write data to sd card
   if (LED_debug){
     digitalWrite(LED_D, HIGH);
   }
   
 File dataFile = SD.open("DataLOG.csv", FILE_WRITE);
   if (dataFile) {
    dataFile.println(logString);
    dataFile.close();
    // print to the serial port too:
    //Serial.println(logString);
   }
   if (LED_debug){
    digitalWrite(LED_D, LOW);
   }
}
