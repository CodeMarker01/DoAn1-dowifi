/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/
*********/

// Import required libraries
#include <Arduino.h>

// Web Server
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "FS.h"
#define QUOTE(...) #__VA_ARGS__

//Cam bien DHT11
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include "Wire.h"
#include "SPI.h"

// Do nhip tim va oxy MAX30102
#include "MAX30105.h"
#include "spo2_algorithm.h"
MAX30105 particleSensor;

#define MAX_BRIGHTNESS 255

#if defined(__AVR_ATmega328P__) || defined(__AVR_ATmega168__)
//Arduino Uno doesn't have enough SRAM to store 100 samples of IR led data and red led data in 32-bit format
//To solve this problem, 16-bit MSB of the sampled data will be truncated. Samples become 16-bit data.
uint16_t irBuffer[100];  //infrared LED sensor data
uint16_t redBuffer[100]; //red LED sensor data
#else
uint32_t irBuffer[100];  //infrared LED sensor data
uint32_t redBuffer[100]; //red LED sensor data
#endif

int32_t bufferLength;  //data length
int32_t spo2;          //SPO2 value
int8_t validSPO2;      //indicator to show if the SPO2 calculation is valid
int32_t heartRate;     //heart rate value
int8_t validHeartRate; //indicator to show if the heart rate calculation is valid

byte pulseLED = D4;         //Must be on PWM pin
byte readLED = BUILTIN_LED; //Blinks with each data read
unsigned int j = 0;
unsigned int i = 0;
unsigned int k = 25;
unsigned int l = 0;
unsigned int m = 0;
unsigned long Lred;
unsigned long currentTime = millis();
unsigned long previousTimeShut = 0;
unsigned long previousTimeUp = 0;
unsigned long interval = 10000;
unsigned int dem = 0;
unsigned int demWifiConnect = 0;
bool coWiFi = true;

// Oled 1.3"
#include <SH1106.h>

// lay thoi gian thuc
#include <WiFiUdp.h>
#include <NTPClient.h>
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "1.vn.pool.ntp.org", 7 * 3600);
String WeekDays[7] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
String months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
String HMS;
String currentDate;

// Create AsyncWebServer object on port 80
ESP8266WebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillisDHT = 0; // will store last time DHT was updated
unsigned long previousMillisRTC = 0; // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long intervalDHT = 10000;
const long intervalRTC = 1000;
uint32_t wifiIP;

//Oled section
SH1106 display(0x3C, D2, D1);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
    <script src="https://use.fontawesome.com/a836d2f02c.js"></script>
    <script src="https://code.highcharts.com/highcharts.js"></script>
    <link rel="stylesheet" href="styles.css">
    <title>Document</title>
</head>
<body>
  <style>
    html {
  font-family: Arial;
  display: inline-block;
  margin: 0px auto;
  text-align: center;
  }
    h2 {  
      font-size: 3rem;
    }
    p {
      font-size: 3rem;
    }
    .units {
      font-size: 1.2rem;
    }
    .dht-labels {
      font-size: 1.5rem;
      vertical-align: middle;
      padding-bottom: 15px;
    }
    .scanWifi{
      color: #1e836c
      width: 1 rem;
      height: 1 rem;
    }
  </style>
  <h2>ESP8266 Smart Watch Server</h2>
  <a href='/scanwifi'> 
        <button class="scanWifi"> 
            Scan Wifi
        </button> 
    </a> 
  <a href='/sleepMax'> 
      <button class="scanWifi"> 
          Sensor Sleep
      </button> 
  </a> 
  <a href='/wakeUpMax'> 
        <button class="scanWifi"> 
            Wake Up Sensor
        </button> 
    </a> 
  <p>
    <i class="fas fa-thermometer-half" style="color:#059e8a;"></i> 
    <span class="dht-labels">Temperature</span> 
    <span id="temperature">%TEMPERATURE%</span>
    <sup class="units">&deg;C</sup>
  </p>
  <p>
    <i class="fas fa-tint" style="color:#00add6;"></i> 
    <span class="dht-labels">Humidity</span>
    <span id="humidity">%HUMIDITY%</span>
  </p>
  <p>
    <i class="fas fa-heartbeat" style="color:#00add6;"></i> 
    <span class="dht-labels">Heart Rate</span>
    <span id="HR">%heartRate%</span>
  </p>
  <p>
    <i class="fas fa-fingerprint" style="color:#00add6;"></i> 
    <span class="dht-labels">SpO2</span>
    <span id="SPO2">%spo2%</span>
  </p>
  <br>
  <h2><i class="fas fa-calendar-alt style="color:#00add6;"></i> <span>Calendar</span></h2>
  <p>
    <span id="timehms">%HMS%</span>
    </br>
    <span id="timedmy" class="dht-labels">%currentDate%</span>
  </p>
  
</body>
<script>

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("timehms").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/hms", true);
  xhttp.send();
}, 500 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("timedmy").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/dmy", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("HR").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/hr", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("SPO2").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/spo2", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("temperature").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("humidity").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;

var chartT = new Highcharts.Chart({
  chart:{ renderTo : 'chart-temperature' },
  title: { text: 'BME280 Temperature' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    },
    series: { color: '#059e8a' }
  },
  xAxis: { type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Temperature (Celsius)' }
    //title: { text: 'Temperature (Fahrenheit)' }
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartT.series[0].data.length > 40) {
        chartT.series[0].addPoint([x, y], true, true, true);
      } else {
        chartT.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/temperature", true);
  xhttp.send();
}, 10000 ) ;

var chartH = new Highcharts.Chart({
  chart:{ renderTo:'chart-humidity' },
  title: { text: 'BME280 Humidity' },
  series: [{
    showInLegend: false,
    data: []
  }],
  plotOptions: {
    line: { animation: false,
      dataLabels: { enabled: true }
    }
  },
  xAxis: {
    type: 'datetime',
    dateTimeLabelFormats: { second: '%H:%M:%S' }
  },
  yAxis: {
    title: { text: 'Humidity (%)' }
  },
  credits: { enabled: false }
});
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var x = (new Date()).getTime(),
          y = parseFloat(this.responseText);
      //console.log(this.responseText);
      if(chartH.series[0].data.length > 40) {
        chartH.series[0].addPoint([x, y], true, true, true);
      } else {
        chartH.series[0].addPoint([x, y], true, false, true);
      }
    }
  };
  xhttp.open("GET", "/humidity", true);
  xhttp.send();
}, 10000 ) ;

</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String &var)
{
  if (var == "HMS")
  {
    return String(HMS);
  }
  if (var == "currentDate")
  {
    return String(currentDate);
  }

  return String();
}

void setup()
{
  // Serial port for debugging purposes
  Serial.begin(115200);
  pinMode(pulseLED, OUTPUT);
  pinMode(readLED, OUTPUT);

  //esp tu phat wifi (Soft-AP)
  WiFi.disconnect();
  delay(100);

  server.on("/scanwifi", [] {
    if (server.hasArg("ssid") && server.hasArg("sspass"))
    {
      String tmp = server.arg("ssid") + "\n" + server.arg("sspass");
      SPIFFS.begin();
      File f = SPIFFS.open("wifi", "w");
      if (f)
      {
        f.write((const uint8_t *)tmp.c_str(), tmp.length());
      }
      f.close();
      server.send(200, "text/plain; charset=utf-8", "Đã thiết lập xong");
      ESP.reset();
    }
    else
    {
      server.send(200, "text/html", QUOTE(<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>Document</title>
  </head>
  <body>
    <form action="./scanwifi">
      <label for="">Tên wifi: </label>
      <input type="text" name="ssid" id="wfname" />
      <br /><br />
      <label for="">Mật khẩu wifi: </label>
      <input type="password" name="sspass" />
      <br /><br />
      <input type="submit" value="Xong" />
    </form>

    <div id="listWifi"></div>
    <script>
      window.onload = getData;
      function getData() {
        var xhttp = new XMLHttpRequest();
        xhttp.onreadystatechange = function()
        {
          if (this.readyState == 4 && this.status == 200)
          {
            var resText = xhttp.responseText.split("|");
            var tmp = "";
            for (i = 0; i < resText.length; i++)
            {
              tmp =
                  tmp +
                  "<div id = " +
                  i +
                  ' onclick="setUsr(this)">' +
                  resText[i] +
                  "</div>";
            }
            document.getElementById("listWifi").innerHTML = tmp;
            getData();
          }
        };
        xhttp.open("GET", "/listWifi", true);
        xhttp.send();
      }
      function setUsr(divObj) {
        document.getElementById("wfname").value = divObj.innerHTML;
      }
    </script>
  </body>
</html>
));
    }
  });
  server.on("/listWifi", [] {
    String list;
    int n = WiFi.scanNetworks();
    if (n == 0)
    {
      list = "no networks found";
    }
    else
    {
      for (int i = 0; i < n; ++i)
      {
        list += String(WiFi.SSID(i)) + "|";
      }
    }
    server.send(200, "text/html", list);
  });

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("dang quang", "19216812");

  // todo do wifi
  String id = "";
  String pass = "";
  SPIFFS.begin();
  File f = SPIFFS.open("wifi", "r");
  if (f)
  {
    id = f.readStringUntil('\n');
    pass = f.readString();
  }
  f.close();

  if (id != "" && pass != "")
  {
    Serial.println(String("Đang kết nối: ") + id + " | " + pass);
    WiFi.begin(id, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
      delay(100);
      Serial.print(".");
      server.handleClient();
      demWifiConnect++;
      if (demWifiConnect == 10)
      {
        demWifiConnect = 0;
        coWiFi = false;
        break;
      }
    }

    Serial.println(String("Kết nối thành công"));

    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.print(String("Chưa có dữ liệu wifi"));
  }

  // Print ESP8266 Local IP Address
  wifiIP = ((uint32_t)WiFi.localIP());
  Serial.print(wifiIP);
  Serial.print(", IP: ");
  Serial.println(WiFi.localIP());

  // Route for root / web page
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
  //   request->send_P(200, "text/html", index_html, processor);
  // });
  // server.on("/", HTTP_GET, [] { server.send(200, "text/html", String(processor).c_str()); });
  server.on("/", HTTP_GET, [] { server.send(200, "text/html", index_html); });

  //Gio phut giay
  server.on("/hms", HTTP_GET, [] {
    server.send(200, "text/plain", String(HMS).c_str());
  });
  //Ngay thang nam
  server.on("/dmy", HTTP_GET, [] {
    server.send(200, "text/plain", String(currentDate).c_str());
  });

  //Nhip tim HR
  server.on("/hr", HTTP_GET, [] {
    server.send(200, "text/plain", String(heartRate).c_str());
  });
  //SpO2
  server.on("/spo2", HTTP_GET, [] {
    server.send(200, "text/plain", String(spo2).c_str());
  });

  // todo Sleep and Wake Up Max30102
  // server.on("/sleepMax", HTTP_GET, [] {
  //   particleSensor.shutDown();
  //   Serial.println("Tat sensor");
  // });
  // server.on("/wakeUpMax", HTTP_GET, [] {
  //   particleSensor.wakeUp();
  //   Serial.println("Bat sensor");
  // });

  // Start server
  server.begin();

  // todo Oled 1.3"
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
  display.display();
  display.clear();
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 20, "IP: ");
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 35, WiFi.localIP().toString());

  //todo MAX30102
  // !Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    Serial.println(F("MAX30105 was not found. Please check wiring/power."));
    while (1)
      ;
  }

  byte ledBrightness = 60; //Options: 0=Off to 255=50mA
  byte sampleAverage = 4;  //Options: 1, 2, 4, 8, 16, 32
  byte ledMode = 2;        //Options: 1 = Red only, 2 = Red + IR, 3 = Red + IR + Green
  byte sampleRate = 100;   //Options: 50, 100, 200, 400, 800, 1000, 1600, 3200
  int pulseWidth = 411;    //Options: 69, 118, 215, 411
  int adcRange = 4096;     //Options: 2048, 4096, 8192, 16384

  particleSensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange); //Configure sensor with these settings
}

void loop()
{
  server.handleClient(); //duy tri su song cho cai server

  unsigned long currentMillis = millis();
  display.display();
  display.clear();
  bufferLength = 100; //buffer length of 100 stores 4 seconds of samples running at 25sps
                      //read the first 100 samples, and determine the signal range
                      // if (coWiFi == true)
                      // {
  timeClient.update();

  // Cap nhap thoi gian
  if (currentMillis - previousMillisRTC >= intervalRTC)
  {
    previousMillisRTC = currentMillis;
    HMS = timeClient.getFormattedTime();
    Serial.println(HMS);

    // Get a time structure
    unsigned long epochTime = timeClient.getEpochTime();

    struct tm *ptm = gmtime((time_t *)&epochTime);

    int monthDay = ptm->tm_mday;

    int currentMonth = ptm->tm_mon + 1;

    String currentMonthName = months[currentMonth - 1];

    int currentYear = ptm->tm_year + 1900;

    //Print complete date:
    currentDate = String(WeekDays[timeClient.getDay()]) + "-" + String(monthDay) + "-" + String(currentMonth) + "-" + String(currentYear);
    String currentDateOled = String(currentYear) + "-" + String(currentMonth) + "-" + String(monthDay);
    String currentWeekdayOled = String(WeekDays[timeClient.getDay()]);
    Serial.print("Current date : ");
    Serial.println(currentDate);

    Serial.println("");
  }

  if (j < 100)
  {
    if (i < bufferLength)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check();                   //Check the sensor for new data

      redBuffer[i] = particleSensor.getRed();
      irBuffer[i] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      Serial.print(F("red="));
      Serial.print(redBuffer[i], DEC);
      Serial.print(F(", ir="));
      Serial.println(irBuffer[i], DEC);
      i++;
    }
    if (j == 99)
    { //calculate heart rate and SpO2 after first 100 samples (first 4 seconds of samples)
      maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    }
    j++;
  }

  //Continuously taking samples from MAX30102.  Heart rate and SpO2 are calculated every 1 second
  // while (1)
  else if (j >= 100)
  {
    //dumping the first 25 sets of samples in the memory and shift the last 75 sets of samples to the top
    // for (byte i = 25; i < 100; i++)
    if (k < 100)
    {
      redBuffer[k - 25] = redBuffer[k];
      irBuffer[k - 25] = irBuffer[k];
      k++;
    }

    //take 25 sets of samples before calculating the heart rate.
    // for (byte i = 75; i < 100; i++)
    else if (k < 125)
    {
      while (particleSensor.available() == false) //do we have new data?
        particleSensor.check();                   //Check the sensor for new data

      digitalWrite(readLED, !digitalRead(readLED)); //Blink onboard LED with every data read

      redBuffer[k - 25] = particleSensor.getRed();
      irBuffer[k - 25] = particleSensor.getIR();
      particleSensor.nextSample(); //We're finished with this sample so move to next sample

      //send samples and calculation result to terminal program through UART
      // Serial.print(F("red="));
      // Serial.print(redBuffer[i], DEC);
      // Serial.print(F(", ir="));
      // Serial.print(irBuffer[i], DEC);

      // Serial.print(F(", HR="));
      // Serial.print(heartRate, DEC);

      // Serial.print(F(", HRvalid="));
      // Serial.print(validHeartRate, DEC);

      // Serial.print(F(", SPO2="));
      // Serial.print(spo2, DEC);

      // Serial.print(F(", SPO2Valid="));
      // Serial.println(validSPO2, DEC);
    }

    //After gathering 25 new samples recalculate HR and SP02
    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate, &validHeartRate);
    k++;
    j++;
  }

  else if (j >= 200)
  {
    j = 100;
    k = 25;
  }

  // hien thi oled
  // todo display.drawString(x (hang ngang), y(hang doc), "Temp: ");
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  // display.drawString(0, 0, "Temp: ");
  display.drawString(0, 0, "HR: ");
  display.setFont(ArialMT_Plain_10);
  // display.drawString(40, 0, String(t));
  display.drawString(40, 0, String(heartRate));
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(100, 0, "SpO2: ");
  display.setFont(ArialMT_Plain_10);
  // display.drawString(128, 0, String(h));
  display.drawString(128, 0, String(spo2));
  display.setTextAlignment(TEXT_ALIGN_CENTER);
  display.setFont(ArialMT_Plain_16);
  display.drawString(64, 20, HMS);
  display.setFont(ArialMT_Plain_10);
  display.drawString(64, 35, currentDate);
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(ArialMT_Plain_10);
  display.drawString(0, 45, "IP: ");
  display.setFont(ArialMT_Plain_10);
  display.drawString(30, 45, WiFi.localIP().toString());
}