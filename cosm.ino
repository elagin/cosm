// BMP085_output
// by Filipe Vieira
// Simple example of library usage with almost every BMP085 and lib features being used.

#include <Wire.h>
#include <BMP085.h>

#include <SPI.h>
#include <Ethernet.h>

#include <OneWire.h>
#include <DallasTemperature.h>

#include <ztseg8b4.h>
#define I2C_ADDRESS 0x77 //77?

BMP085 dps = BMP085();
long Temperature = 0, Pressure = 0, Altitude = 0;

// termometer
#define ONE_WIRE_BUS 8 // termometer bus on maket
#define TEMPERATURE_PRECISION 10
#define MAX_DS1820_SENSORS 8

OneWire oneWire(ONE_WIRE_BUS);  // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature dallasSensors(&oneWire);    // Pass our oneWire reference to Dallas Temperature.

// Описание датчика
struct sensorData
{
  DeviceAddress   addr;       // Адрес датчика
  char            name[20];   // Имя для отображения
  float           temp;       // Текущее значение температуры
};

// Хранилище характеристик датчиков
sensorData sensorsParams[MAX_DS1820_SENSORS] = {
  0x28, 0x45, 0xAF, 0xC7, 0x02, 0x0, 0x0, 0x2C, "Remote", 0,
  0x28, 0x93, 0xBB, 0xC7, 0x02, 0x00, 0x00, 0x39, "rename me", 0,
  0x28, 0xB0, 0xDB, 0xC7, 0x02, 0x00, 0x00, 0xC7, "out", 0,
  0x28, 0x9B, 0xC5, 0xC7, 0x02, 0x00, 0x00, 0x57, "Ter cold", 0,
  0x28, 0xEE, 0xD6, 0xC7, 0x02, 0x00, 0x00, 0x16, "Ter warm", 0,
  0x28, 0x02, 0xDC, 0xC7, 0x02, 0x00, 0x00, 0xFF, "battery", 0, // passed
  0x28, 0xFA, 0xDF, 0xC7, 0x02, 0x00, 0x00, 0x62, "Stepan", 0
};

// Хранилище для обхода всех найденных датчиков
struct sensor
{
  byte            id;       // Идентификатор датчика из настрек
  DeviceAddress   addr;     // Адрес датчика
};

sensor sensors[MAX_DS1820_SENSORS];

short sensorCount = 0;

int error_pin = 7;
int ok_pin = 48;

#define USERAGENT      "Cosm Arduino Example (77684)" // user agent is the project name
#define APIKEY         "ErZDfeOMuzvp3HbdyRpWM5BKXEGSAKxFRW1rNjNrRloxRT0g" // your cosm api key
#define FEEDID         77684 // your feed ID

// assign a MAC address for the ethernet controller.
// fill in your address here:
byte mac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// fill in an available IP address on your network here,
// for manual configuration:
IPAddress ip(10,0,1,20);

// if you don't want to use DNS (and reduce your sketch size)
// use the numeric IP instead of the name for the server:
IPAddress server(64,94,18,121);      // numeric IP for api.cosm.com
//64.94.18.121
//char server[] = "api.cosm.com";   // name address for Cosm API

void setup(void) {
  pinMode(10, OUTPUT);
  pinMode(4, OUTPUT);
  digitalWrite(10, HIGH);
  digitalWrite(4, HIGH);

  pinMode(error_pin, OUTPUT);
  digitalWrite(error_pin, HIGH);

  pinMode(ok_pin, OUTPUT);
  digitalWrite(ok_pin, HIGH);

  Serial.begin(9600);
  Wire.begin();
  delay(1000);
  digitalWrite(ok_pin, LOW);
  digitalWrite(error_pin, LOW);

  ztseg8b4.SetAddress(ZTSEG8B4_ADDR);
  Wire.requestFrom(ZTSEG8B4_ADDR, 15);    // request 6 bytes from slave device #2

  while(Wire.available())    // slave may send less than requested
  { 
    char c = Wire.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }
  ztseg8b4.DisplayDec(0);   

  Serial.println("Attempting to get an IP address using DHCP:");
  if (!Ethernet.begin(mac))
  {
    digitalWrite(error_pin, HIGH);
    delay(200);
    digitalWrite(error_pin, LOW);    
    delay(200);
    digitalWrite(error_pin, HIGH);    
    // if DHCP fails, start with a hard-coded address:
    Serial.println("failed to get an IP address using DHCP, trying manually");
    Ethernet.begin(mac, ip);
  }
  Serial.print("My address:");
  Serial.println(Ethernet.localIP());

  dps.init();   
  //  showall();
//   Serial.println("Registers dump");
  dumpRegisters();
//  Serial.println("Calibration data");
  dps.dumpCalData();

  t_init();

  digitalWrite(4, HIGH);
  delay(300);
}

bool compareAddres(DeviceAddress deviceAddress, DeviceAddress deviceAddress2)
{
  int count = 0;
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] == deviceAddress2[i])
      count ++;
  }
  return count==8;
}

void t_init()
{
  Wire.begin();
  dallasSensors.begin();
  oneWire.reset_search();
  // Loop through each device, print out address

  // Grab a count of devices on the wire
  sensorCount = dallasSensors.getDeviceCount();
  Serial.print("Found ");
  Serial.print(sensorCount);
  Serial.println(" devices.");
  delay(50);

  for(int i=0; i < sensorCount; i++)
  {
    Serial.println(i);
    DeviceAddress addr_tmp;
    if(oneWire.search(addr_tmp))
//    if(oneWire.search(sensors[i].addr)) // Найден следующий датчик
    {
      Serial.println("new sensor found");
      dallasSensors.setResolution(addr_tmp, TEMPERATURE_PRECISION); // Устанавливаем точность измерения
//      for(uint8_t j = 0; j < sizeof(sensorsParams); j++)
      for(uint8_t j = 0; j < 7; j++)      
      {
        if(compareAddres(sensorsParams[j].addr, addr_tmp)) // Ищем датчик по адресу в списке настроек
        {
          Serial.println("limk sensor ");
          sensors[i].id = j;                           // Связваем подключенный датчик с настройками
          //                    dataout.addData(sensorsParams[j].pachubeId); // указываем id для Pachube с которыми будем работать
        }
      }
//      sensorCount++;
    }
    else
    {
      break; // Датчики закончились
    }

  }
  /*    
   for(int i=0;i<numberOfDevices; i++)
   {
   // Search the wire for address
   if(sensors.getAddress(tempDeviceAddress, i))
   {
   Serial.print("Found device ");
   Serial.print(i, DEC);
   Serial.print(" with address: ");
   //		printAddress(tempDeviceAddress);
   Serial.println();
  /*
   Serial.print("Setting resolution to ");
   Serial.println(TEMPERATURE_PRECISION, DEC);
   */
  // set the resolution to TEMPERATURE_PRECISION bit (Each Dallas/Maxim device is capable of several different resolutions)
  //      sensors.setResolution(tempDeviceAddress, TEMPERATURE_PRECISION);
  /*
      Serial.print("Resolution actually set to: ");
   Serial.print(sensors.getResolution(tempDeviceAddress), DEC); 
   Serial.println();*/
  /*    }
   else
   {
   Serial.print("Found ghost device at ");
   Serial.print(i, DEC);
   Serial.print(" Check power and cabling");
   }
   }
   */
}

String floatToStr(float val)
{
  String res;
  char bufFloat[40];
  dtostrf(val, 2, 2, bufFloat);
  res += bufFloat;
  return res;
}

void get_dalas_temp()
{
  dallasSensors.requestTemperatures();
  // Loop through each device, print out temperature data
  for(int i=0;i<sensorCount; i++)
  {
//    Serial.println(i);
    byte sensor = sensors[i].id; // Выбираем датчик
    sensorsParams[sensor].temp = dallasSensors.getTempC(sensorsParams[sensor].addr); // Сохраняем показание.  
    /*    // It responds almost immediately. Let's print out the data
     float tempC = sensors.getTempC(tempDeviceAddress);
     Serial.print("tempC[");
     Serial.print(i);
     Serial.print("] : ");
     Serial.println(tempC);      
     //      if(tempC < 79 && tempC > -100)
     {
     if(0==i)
     {
     return tempC;
     }
     //        normalValues(tempC, min_dalas_temperature, dalas_temperature, max_dalas_temperature);                  
     //        Serial.println("Temp OK");
     }
     } 
     */
    //else ghost device! Check your power requirements and cabling
  }
}

void loop(void)
{
  dps.getTemperature(&Temperature); 
  float temp = Temperature / 10.0;  
  Serial.print("Temperature(C):");
  Serial.println(temp);  

  dps.getPressure(&Pressure);
  float press_mm = Pressure / 133.3;
  Serial.print("  Pressure(mm):");
  Serial.println(press_mm);

  get_dalas_temp();
    
//  if(press_mm > 0)
  {
    String dataString = "pressure,";
    dataString += floatToStr(press_mm);
    dataString += "\nbaro_termo,";
    dataString += floatToStr(temp);
    //    if(tempC < 79 && tempC > -100)

    String dalasData;
    for(int i=0;i<sensorCount; i++)
    {
      byte sensor = sensors[i].id; // Выбираем датчик
//      sensorsParams[sensor].temp = dallasSensors.getTempC(sensorsParams[sensor].addr); // Сохраняем показание.
      if(sensorsParams[sensor].temp != 85)
      {
        dataString += "\n";
        dataString += sensorsParams[sensor].name;
        dataString += ",";
        dataString += floatToStr(sensorsParams[sensor].temp);
        String dasplaySensor = "out";
        
//        Display(uint8_t addr,uint8_t *buf)
        uint8_t buf = 1234;
//        ztseg8b4.Display(&buf);
/*       
        float test = 0.0;
        for(int ss=0; ss < 22; ss++)
        {
            ztseg8b4.Display(buf);
            delay(500);
            test = test + 0.1;
        }*/
        if(dasplaySensor.equals(sensorsParams[sensor].name))
        {
          ztseg8b4.DisplayDec(sensorsParams[sensor].temp);
        }

      }
    }
//    if(85 != outTemp)
/*    
    {
      digitalWrite(error_pin, LOW );
      dataString += "\nout,";
      dataString += floatToStr(outTemp);
      ztseg8b4.DisplayDec(outTemp);      
    }
    else
    {
      digitalWrite(error_pin, HIGH);
      ztseg8b4.DisplayDec(0.1);
    }
*/
    Serial.println(dataString);
    sendData(dataString);
  }

  int led_on = true;
  for(int i=60*10; i > 0; i--)
  {
    digitalWrite(ok_pin, led_on);
    delay(1000);
    Serial.println(i);
    led_on = !led_on;
  }

}

// this method makes a HTTP connection to the server:
void sendData(String thisData)
{
  digitalWrite(ok_pin, HIGH);
  Serial.println("sendData");
  // if there's a successful connection:

  digitalWrite(4, HIGH);
  EthernetClient client;
  if (client.connect(server, 80))
  {
    //    Serial.println("connecting...");
    // send the HTTP PUT request:
    client.print("PUT /v2/feeds/");
    client.print(FEEDID);
    client.println(".csv HTTP/1.1");
    client.println("Host: api.cosm.com");
    client.print("X-ApiKey: ");
    client.println(APIKEY);
    client.print("User-Agent: ");
    client.println(USERAGENT);
    client.print("Content-Length: ");
    client.println(thisData.length());

    // last pieces of the HTTP PUT request:
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();

    // here's the actual content of the PUT request:
    client.println(thisData);
    digitalWrite(ok_pin, LOW);

    char displayBuf[200];
    delay(100); // Что бы сервер успел сгенерить ответ
    char data[2048];
    int i = 0;
    int dataSize = client.available();
    Serial.print("dataSize: ");
    Serial.println(dataSize);
    while(i < dataSize)
    {
      data[i] = client.read();
      i++;
    }
    data[i]=0;
    char szKey[] = "HTTP/1.1 200 OK";
    if(strncmp (szKey,data, 15)==0)
    {
      Serial.println("OK!");
    }
    else
    {
      sprintf(displayBuf, data);
      Serial.println(displayBuf);
    }
  } 
  else
  {
    // if you couldn't make a connection:
    Serial.println("connection failed");
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
  // note the time that the connection was made or attempted:
  //  Serial.println("sendData <<<");  
}

void showall(void)
{ 
  /*  
   Serial.println("Current BMP085 settings");
   Serial.println("==========================================================");
   Serial.print("device address                  = 0x");
   Serial.println(dps.getDevAddr(), HEX);
   Serial.print("Mode                            = ");
   switch (dps.getMode()) {
   case MODE_ULTRA_LOW_POWER: 
   Serial.println("MODE_ULTRA_LOW_POWER");
   break;
   case MODE_STANDARD: 
   Serial.println("MODE_STANDARD");
   break;    
   case MODE_HIGHRES: 
   Serial.println("MODE_HIGHRES");
   break;    
   case MODE_ULTRA_HIGHRES:     
   Serial.println("MODE_ULTRA_HIGHRES");
   break; 
   }  
   */
  delay(200);
}

void dumpRegisters()
{
  byte ValidRegisterAddr[]={
    0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,0xF6,0xF7,0xF8,0xF9  };
  byte _b, i, totregisters = sizeof(ValidRegisterAddr);
  /*  Serial.println("---dump start---");
   Serial.println("Register address|Register data");
   Serial.println("Reg.address(hex,dec) Reg.data(bin,hex,dec)");*/
  for (i=0;i<totregisters;i++){    
    /*    Serial.print("0x");
     Serial.print(ValidRegisterAddr[i], HEX);
     Serial.print(",");
     Serial.print(ValidRegisterAddr[i], DEC);
     Serial.print(",");*/
    dps.readmem(ValidRegisterAddr[i], 1, &_b);
    /*    Serial.print("b");
     print_bits(_b);
     Serial.print(",0x");
     Serial.print(_b,HEX);
     Serial.print(",");
     Serial.println(_b,DEC);  */
  }
  //  Serial.println("---dump end---");
}

void print_bits(byte val){
  int i;
  for(i=7; i>=0; i--) 
    Serial.print(val >> i & 1, BIN);
}
/* void print_unit16(uint16_t val){
 int i;
 for(i=15; i>=0; i--) 
 Serial.print(val >> i & 1, BIN);
 } 
 */
/*
bool sendToServer(String thisData)
{
  char displayBuf[200];
  digitalWrite(4, HIGH);
  char buf[2048];
  EthernetClient client;
  client.connect(server, 80);
  if(client.connected())
  {
    Serial.println("connected");
    // send the HTTP PUT request:
    client.print("PUT /v2/feeds/");
    client.print(FEEDID);
    client.println(".csv HTTP/1.1");
    client.println("Host: api.cosm.com");
    client.print("X-ApiKey: ");
    client.println(APIKEY);
    client.print("User-Agent: ");
    client.println(USERAGENT);
    client.print("Content-Length: ");
    client.println(thisData.length());

    // last pieces of the HTTP PUT request:
    client.println("Content-Type: text/csv");
    client.println("Connection: close");
    client.println();

    // here's the actual content of the PUT request:
    client.println(thisData);        
  }
  else
  {
    Serial.println("Not connected");
  }
  client.stop();
}
*/



