
#include <SPI.h>
#include <SD.h>
#include <ModbusMaster.h>                   // Load the (modified) library for modbus communication command codes. Kindly install at our website.
#include "RTClib.h"
RTC_DS3231 rtc;
char daysOfTheWeek[7][12] = {"Ahad", "Senin", "Selasa", "Rabu", "Kamis", "Jum'at", "Sabtu"};
int jam, menit, detik;
int tanggal, bulan, tahun;
String hari;
#include <Wire.h> 
#include <Adafruit_INA219.h> 
// Anda perlu mendownload library ini diatas dan copy paste sesuai perintah 

const int INA_addr = 0x40;  // INA219 address
Adafruit_INA219 ina219(INA_addr);

float tegangan = 00;
float arus = 00; // diukur menggunakan MiliAmpere 
float daya = 00; 
        #define MAX485_DE      2                    // Define DE Pin to Arduino pin. Connect DE Pin of Max485 converter module to Pin 2 (default) Arduino board
        #define MAX485_RE      3     
int led = 13; 
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
        

void setup() 
{
        Serial.begin(9600);              /* To assign communication port to communicate with meter. with 2 stop bits (refer to manual)*/
        Serial3.begin(9600);
        pinMode(led, OUTPUT);
         if (! rtc.begin()) {
    Serial.println("Couldn't find RTC");
    Serial.flush();
    while (1) delay(10);
  }
 
        /* 1- PZEM-017 DC Energy Meter */
        
          setShunt(0x01);                          // Delete the "//" to set shunt rating (0x01) is the meter address by default
        // resetEnergy(0x01);                       // By delete the double slash symbol, the Energy value in the meter is reset. Can also be reset on the lcd Display      
        startMillisPZEM = millis();                 /* Start counting time for run code */
        node.begin(pzemSlaveAddr, Serial3);          /* Define and start the Modbus RTU communication. Communication to specific slave address and which Serial port */
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
 
  Serial.println("Buka file di kartu microSD");
 
  if (!SD.begin(53)) { //tergantung di pin chipselect yang digunakan
    Serial.println("Gagal baca microSD!");
    return;
  }
  Serial.println("Sukses baca kartu microSD!");
 ina219.begin();


  }

void loop() 
{  
  DateTime now = rtc.now();
  jam     = now.hour();
  menit   = now.minute();
  detik   = now.second();
  tanggal = now.day();
  bulan   = now.month();
  tahun   = now.year();
  hari    = daysOfTheWeek[now.dayOfTheWeek()];  
tegangan = ina219.getBusVoltage_V(); //command untuk pembacaan tegangan 
arus = ina219.getCurrent_mA(); //command untuk pembacaan arus 
daya = tegangan * (arus/1000); //rumus untuk mendapatkan nilai watt 

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
      //   Serial.println(String() + hari + ", " + tanggal + "-" + bulan + "-" + tahun);
      // Serial.println(String() + jam + ":" + menit + ":" + tahun);
      // Serial.println();
      // delay(1000);
            Serial.print(tanggal);
            Serial.print("/");
            Serial.print(bulan);
            Serial.print("/");
            Serial.print(tahun);
            Serial.print("/");
            Serial.print(jam);
            Serial.print(":");
            Serial.print(menit);
            Serial.print(":");
            Serial.print(detik);
            Serial.print(";");                                                                 /* Set cursor to first colum 0 and second row 1  */
            Serial.print(PZEMVoltage, 2);                                                                    /* Display Voltage on lcd Display with 1 decimal*/
            Serial.print(";");  
            Serial.print(PZEMCurrent, 2);  
           Serial.print(";");  
            Serial.print(PZEMPower, 2); 
            Serial.print(";");  
            Serial.print(PZEMEnergy, 2);                                                          /* Set the starting point again for next counting time */  
          Serial.print(";");
          Serial.print(tegangan);
          Serial.print(";");
          Serial.print(arus);
          Serial.print(";");
          Serial.print(daya);
          Serial.print(";");
          Serial.println();
          // jika file sudah berhasil dibuka maka tulis data dimulai
          myFile = SD.open("Baterai.csv", FILE_WRITE); //menulis File coba.csv
  // jika file sudah berhasil dibuka maka tulis data dimulai
  if (myFile) {
    Serial.println("mencoba nulis data di file PZM.txt");
            myFile.print(tanggal);
            myFile.print("/");
            myFile.print(bulan);
            myFile.print("/");
            myFile.print(tahun);
            myFile.print("/");
            myFile.print(jam);
            myFile.print(":");
            myFile.print(menit);
            myFile.print(":");
            myFile.print(detik);
            myFile.print(";");                                                                 /* Set cursor to first colum 0 and second row 1  */
            myFile.print(PZEMVoltage, 2);                                                                    /* Display Voltage on lcd Display with 1 decimal*/
            myFile.print(";");  
            myFile.print(PZEMCurrent, 2);  
            myFile.print(";");  
            myFile.print(PZEMPower, 2); 
            myFile.print(";");  
            myFile.print(PZEMEnergy, 2);                                                          /* Set the starting point again for next counting time */  
            myFile.print(";");
            myFile.print(tegangan);
            myFile.print(";");
            myFile.print(arus);
            myFile.print(";");
            myFile.print(daya);
            myFile.print(";");
            myFile.println();
    // tutup file
    myFile.close();
    Serial.println("SUKSES");
    digitalWrite(led, 1);
  } else {
    // jika gagal print error
    Serial.println("Gagal");
    digitalWrite(led, 0);
  }
//  delay(1000);
  delay(60000);
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
      
        Serial3.write(slaveAddr);                                                                       /* these whole process code sequence refer to manual*/
        Serial3.write(SlaveParameter);
        Serial3.write(highByte(registerAddress));
        Serial3.write(lowByte(registerAddress));
        Serial3.write(highByte(NewshuntAddr));
        Serial3.write(lowByte(NewshuntAddr));
        Serial3.write(lowByte(u16CRC));
        Serial3.write(highByte(u16CRC));
        delay(10);
        postTransmission();                                                                                /* trigger reception mode*/
        delay(100);
        while (Serial3.available())                                                                        /* while receiving signal from Serial3 from meter and converter */
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
      
        Serial3.write(OldslaveAddr);                                                                       /* these whole process code sequence refer to manual*/
        Serial3.write(SlaveParameter);
        Serial3.write(highByte(registerAddress));
        Serial3.write(lowByte(registerAddress));
        Serial3.write(highByte(NewslaveAddr));
        Serial3.write(lowByte(NewslaveAddr));
        Serial3.write(lowByte(u16CRC));
        Serial3.write(highByte(u16CRC));
        delay(10);
        postTransmission();                                                                                /* trigger reception mode*/
        delay(100);
        while (Serial3.available())                                                                        /* while receiving signal from Serial3 from meter and converter */
          {   
          }
}
