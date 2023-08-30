#include "Wire.h"
#define DS3231_I2C_ADDRESS 0x68
#include <SoftwareSerial.h>
        /* 1- PZEM-017 DC Energy Meter */
        //#include "RTClib.h"
#include <SPI.h>
#include <SD.h>
#include <ModbusMaster.h>                   // Load the (modified) library for modbus communication command codes. Kindly install at our website.
        #define MAX485_DE      2                    // Define DE Pin to Arduino pin. Connect DE Pin of Max485 converter module to Pin 2 (default) Arduino board
        #define MAX485_RE      3     
       
 
        File myFile;
        
        static uint8_t pzemSlaveAddr = 0x01;        // Declare the address of device (meter) in term of 8 bits. You can change to 0x02 etc if you have more than 1 meter.
        static uint16_t NewshuntAddr = 0x0000;      // Declare your external shunt value. Default is 100A, replace to "0x0001" if using 50A shunt, 0x0002 is for 200A, 0x0003 is for 300A
                                                    // By default manufacturer may already set, however, to set manually kindly delete the "//" for line "// setShunt(0x01);" in void setup
        ModbusMaster node;                          /* activate modbus master codes*/  
        float PZEMVoltage =0;                       /* Declare value for DC voltage */
        float PZEMCurrent =0;                       /* Declare value for DC current*/
        float PZEMPower =0;                         /* Declare value for DC Power */
        float PZEMEnergy=0;                         /* Declare value for DC Energy */
        unsigned long startMillisPZEM;              /* start counting time for lcd Display */
        unsigned long currentMillisPZEM;            /* current counting time for lcd Display */
        const unsigned long periodPZEM = 1000;      // refresh every X seconds (in seconds) in LED Display. Default 1000 = 1 second 
        int page = 1;    
        SoftwareSerial mySerial(5, 6);
// Convert normal decimal numbers to binary coded decimal
byte decToBcd(byte val){
  return( (val/10*16) + (val%10) );
}
// Convert binary coded decimal to normal decimal numbers
byte bcdToDec(byte val){
  return( (val/16*10) + (val%16) );
}

void setup() 
{ 
  Wire.begin();
        Serial.begin(9600);              /* To assign communication port to communicate with meter. with 2 stop bits (refer to manual)*/
        mySerial.begin(9600);
        /* 1- PZEM-017 DC Energy Meter */
        
         setShunt(0x01);                          // Delete the "//" to set shunt rating (0x01) is the meter address by default
        // resetEnergy(0x01);                       // By delete the double slash symbol, the Energy value in the meter is reset. Can also be reset on the lcd Display      
        startMillisPZEM = millis();                 /* Start counting time for run code */
        node.begin(pzemSlaveAddr, mySerial);          /* Define and start the Modbus RTU communication. Communication to specific slave address and which Serial port */
        pinMode(MAX485_RE, OUTPUT);                 /* Define RE Pin as Signal Output for RS485 converter. Output pin means Arduino command the pin signal to go high or low so that signal is received by the converter*/
        pinMode(MAX485_DE, OUTPUT);                 /* Define DE Pin as Signal Output for RS485 converter. Output pin means Arduino command the pin signal to go high or low so that signal is received by the converter*/
        digitalWrite(MAX485_RE, 0);                 /* Arduino create output signal for pin RE as LOW (no output)*/
        digitalWrite(MAX485_DE, 0);                 /* Arduino create output signal for pin DE as LOW (no output)*/
                                                    // both pins no output means the converter is in communication signal receiving mode
        node.preTransmission(preTransmission);      // Callbacks allow us to configure the RS485 transceiver correctly
        node.postTransmission(postTransmission);
        changeAddress(0XF8, 0x01);                  // By delete the double slash symbol, the meter address will be set as 0x01. 
                                                    // By default I allow this code to run every program startup. Will not have effect if you only have 1 meter
                           
                             /* after everything done, wait for 1 second */
  setDS3231time(30,42,16,5,13,10,16);
  Serial.println("Buka file di kartu microSD");
 
  if (!SD.begin(4)) { //tergantung di pin chipselect yang digunakan
    Serial.println("Gagal baca microSD!");
    return;
  }
  Serial.println("Sukses baca kartu microSD!");
 
  }
void setDS3231time(byte second, byte minute, byte hour, byte dayOfWeek, byte
dayOfMonth, byte month, byte year){
  // sets time and date data to DS3231
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0x0E); // select register
  Wire.write(0b00011100); // write register bitmap, bit 7 is /EOS
  Wire.write(decToBcd(second)); // set seconds
  Wire.write(decToBcd(minute)); // set minutes
  Wire.write(decToBcd(hour)); // set hours
  Wire.write(decToBcd(dayOfWeek)); // set day of week (1=Sunday, 7=Saturday)
  Wire.write(decToBcd(dayOfMonth)); // set date (1 to 31)
  Wire.write(decToBcd(month)); // set month
  Wire.write(decToBcd(year)); // set year (0 to 99)
  Wire.endTransmission();
}
void readDS3231time(byte *second,
byte *minute,
byte *hour,
byte *dayOfWeek,
byte *dayOfMonth,
byte *month,
byte *year){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);
  Wire.write(0); // set DS3231 register pointer to 00h
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);
  // request seven bytes of data from DS3231 starting from register 00h
  *second = bcdToDec(Wire.read() & 0x7f);
  *minute = bcdToDec(Wire.read());
  *hour = bcdToDec(Wire.read() & 0x3f);
  *dayOfWeek = bcdToDec(Wire.read());
  *dayOfMonth = bcdToDec(Wire.read());
  *month = bcdToDec(Wire.read());
  *year = bcdToDec(Wire.read());
}
void displayTime(){
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,
  &year);
  // send it to the serial monitor
  Serial.print(hour, DEC);
  // convert the byte variable to a decimal number when displayed
  Serial.print(":");
  if (minute<10){
    Serial.print("0");
  }
  Serial.print(minute, DEC);
  Serial.print(":");
  if (second<10){
    Serial.print("0");
  }
  Serial.print(second, DEC);
  Serial.print(" ");
  Serial.print(dayOfMonth, DEC);
  Serial.print("/");
  Serial.print(month, DEC);
  Serial.print("/");
  Serial.print(year, DEC);
  
}
void loop() 
{    
        currentMillisPZEM = millis();                                                                     /* count time for program run every second (by default)*/
        if (currentMillisPZEM - startMillisPZEM >= periodPZEM)                                            /* for every x seconds, run the codes below*/
        {    
          uint8_t result;                                                                                 /* Declare variable "result" as 8 bits */   
          result = node.readInputRegisters(0x0000, 6);                                                    /* read the 9 registers (information) of the PZEM-014 / 016 starting 0x0000 (voltage information) kindly refer to manual)*/
          if (result == node.ku8MBSuccess)                                                                /* If there is a response */
            {
              uint32_t tempdouble = 0x00000000;                                                           /* Declare variable "tempdouble" as 32 bits with initial value is 0 */ 
              PZEMVoltage = node.getResponseBuffer(0x0000) / 100.0;                                       /* get the 16bit value for the voltage value, divide it by 100 (as per manual) */
                                                                                                          // 0x0000 to 0x0008 are the register address of the measurement value
              PZEMCurrent = node.getResponseBuffer(0x0001) / 100.0;                                       /* get the 16bit value for the current value, divide it by 100 (as per manual) */
              
              tempdouble =  (node.getResponseBuffer(0x0003) << 16) + node.getResponseBuffer(0x0002);      /* get the power value. Power value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
              PZEMPower = tempdouble / 10.0;                                                              /* Divide the value by 10 to get actual power value (as per manual) */
              
              tempdouble =  (node.getResponseBuffer(0x0005) << 16) + node.getResponseBuffer(0x0004);      /* get the energy value. Energy value is consists of 2 parts (2 digits of 16 bits in front and 2 digits of 16 bits at the back) and combine them to an unsigned 32bit */
              PZEMEnergy = tempdouble;                                                                    
              
              if (pzemSlaveAddr==2)                                                                       /* just for checking purpose to see whether can read modbus*/
                {
                }
            } 
              else
                {
                }
              startMillisPZEM = currentMillisPZEM ;                                                       /* Set the starting point again for next counting time */
        }   
  byte second, minute, hour, dayOfWeek, dayOfMonth, month, year;
  // retrieve data from DS3231
  readDS3231time(&second, &minute, &hour, &dayOfWeek, &dayOfMonth, &month,&year);
        displayTime(); // display the real-time clock data on the Serial Monitor,
            Serial.println();
            Serial.print("tegangan:");                                                                      /* Set cursor to first colum 0 and second row 1  */
            Serial.print(PZEMVoltage, 2);                                                                    /* Display Voltage on lcd Display with 1 decimal*/
            Serial.println ("V         ");
            Serial.print("Arus:");  
            Serial.print(PZEMCurrent, 2);  
           Serial.println("A          ");
           Serial.print("Daya:");  
            Serial.print(PZEMPower, 2);
            Serial.println("W          "); 
            Serial.print("energi:");  
            Serial.print(PZEMEnergy, 2);  
            Serial.println("Wh        ");
                                                                    /* Set the starting point again for next counting time */  
          delay(1000);
          // jika file sudah berhasil dibuka maka tulis data dimulai
          myFile = SD.open("PZM.txt", FILE_WRITE); //menulis File coba.csv
  // jika file sudah berhasil dibuka maka tulis data dimulai
  if (myFile) {
    Serial.println("mencoba nulis data di file PZM.txt");
    delay(2000);
    myFile.print(dayOfMonth, DEC);
  myFile.print("/");
  myFile.print(month, DEC);
  myFile.print("/");
  myFile.print(year, DEC );
  myFile.print("/");
    myFile.print(hour, DEC);
    myFile.print(":");
    if (minute<10){
    myFile.print("0");
  }
    myFile.print(minute, DEC);
    myFile.print(":");
    if (second<10){
    myFile.print("0");
  }
    myFile.print(second, DEC);
    myFile.print("/");
    myFile.print(PZEMVoltage);
    myFile.print(",");
    myFile.print(PZEMCurrent);
    myFile.print(",");
    myFile.print(PZEMPower);
    myFile.print(",");
    myFile.println(PZEMEnergy);
    // tutup file
    myFile.close();
    Serial.println("SUKSES");
  } else {
    // jika gagal print error
    Serial.println("Gagal");
  }
}

void preTransmission()                                                                                    /* transmission program when triggered*/
{
        /* 1- PZEM-017 DC Energy Meter */
        digitalWrite(MAX485_RE, 1);                                                                       /* put RE Pin to high*/
        digitalWrite(MAX485_DE, 1);                                                                       /* put DE Pin to high*/
        delay(1);                                                                                         // When both RE and DE Pin are high, converter is allow to transmit communication
}

void postTransmission()                                                                                   /* Reception program when triggered*/
{
        
        /* 1- PZEM-017 DC Energy Meter */
        
        delay(3);                                                                                         // When both RE and DE Pin are low, converter is allow to receive communication
        digitalWrite(MAX485_RE, 0);                                                                       /* put RE Pin to low*/
        digitalWrite(MAX485_DE, 0);                                                                       /* put DE Pin to low*/
}



void setShunt(uint8_t slaveAddr)                                                                          //Change the slave address of a node
{

        /* 1- PZEM-017 DC Energy Meter */
        
        static uint8_t SlaveParameter = 0x06;                                                             /* Write command code to PZEM */
        static uint16_t registerAddress = 0x0003;                                                         /* change shunt register address command code */
        
        uint16_t u16CRC = 0xFFFF;                                                                         /* declare CRC check 16 bits*/
        u16CRC = crc16_update(u16CRC, slaveAddr);                                                         // Calculate the crc16 over the 6bytes to be send
        u16CRC = crc16_update(u16CRC, SlaveParameter);
        u16CRC = crc16_update(u16CRC, highByte(registerAddress));
        u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
        u16CRC = crc16_update(u16CRC, highByte(NewshuntAddr));
        u16CRC = crc16_update(u16CRC, lowByte(NewshuntAddr));
      
        preTransmission();                                                                                 /* trigger transmission mode*/
      
        mySerial.write(slaveAddr);                                                                       /* these whole process code sequence refer to manual*/
        mySerial.write(SlaveParameter);
        mySerial.write(highByte(registerAddress));
        mySerial.write(lowByte(registerAddress));
        mySerial.write(highByte(NewshuntAddr));
        mySerial.write(lowByte(NewshuntAddr));
        mySerial.write(lowByte(u16CRC));
        mySerial.write(highByte(u16CRC));
        delay(10);
        postTransmission();                                                                                /* trigger reception mode*/
        delay(100);
        while (mySerial.available())                                                                        /* while receiving signal from mySerial from meter and converter */
          {   
          }
}

void changeAddress(uint8_t OldslaveAddr, uint8_t NewslaveAddr)                                            //Change the slave address of a node
{

        /* 1- PZEM-017 DC Energy Meter */
        
        static uint8_t SlaveParameter = 0x06;                                                             /* Write command code to PZEM */
        static uint16_t registerAddress = 0x0002;                                                         /* Modbus RTU device address command code */
        uint16_t u16CRC = 0xFFFF;                                                                         /* declare CRC check 16 bits*/
        u16CRC = crc16_update(u16CRC, OldslaveAddr);                                                      // Calculate the crc16 over the 6bytes to be send
        u16CRC = crc16_update(u16CRC, SlaveParameter);
        u16CRC = crc16_update(u16CRC, highByte(registerAddress));
        u16CRC = crc16_update(u16CRC, lowByte(registerAddress));
        u16CRC = crc16_update(u16CRC, highByte(NewslaveAddr));
        u16CRC = crc16_update(u16CRC, lowByte(NewslaveAddr));
      
        preTransmission();                                                                                 /* trigger transmission mode*/
      
        mySerial.write(OldslaveAddr);                                                                       /* these whole process code sequence refer to manual*/
        mySerial.write(SlaveParameter);
        mySerial.write(highByte(registerAddress));
        mySerial.write(lowByte(registerAddress));
        mySerial.write(highByte(NewslaveAddr));
        mySerial.write(lowByte(NewslaveAddr));
        mySerial.write(lowByte(u16CRC));
        mySerial.write(highByte(u16CRC));
        delay(10);
        postTransmission();                                                                                /* trigger reception mode*/
        delay(100);
        while (mySerial.available())                                                                        /* while receiving signal from mySerial from meter and converter */
          {   
          }
}
