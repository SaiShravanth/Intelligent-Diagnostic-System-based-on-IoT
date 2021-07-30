#include <ESP8266WiFi.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <WiFiClientSecure.h>

#include <Wire.h>
#include <Adafruit_MLX90614.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "MAX30100_PulseOximeter.h"

// Adafruit IO
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME    "Shravanth_Sai"
#define AIO_KEY         "aio_hTpI72JmQkHmJOrFJSI5gXQhs2cw"

#define SCREEN_WIDTH 128    // OLED display width, in pixels
#define SCREEN_HEIGHT 64    // OLED display height, in pixels
#define OLED_RESET -1       // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

WiFiClient client1; //--> Create a WiFiClientSecure object.

// Setup the MQTT client class by passing in the WiFi client and MQTT server and login details.
Adafruit_MQTT_Client mqtt(&client1, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);
Adafruit_MQTT_Publish heart_rate = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/heart_rate");
Adafruit_MQTT_Publish spo2_level = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/spo2_level");
Adafruit_MQTT_Publish body_temp = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/body_temp");

//----------------------------------------SSID and Password of your WiFi router.
const char* ssid = "Shravanth"; //--> Your wifi name or SSID.
const char* password = "shravanth2508"; //--> Your wifi password.
//----------------------------------------

//----------------------------------------Host & httpsPort
const char* host = "script.google.com";
const int httpsPort = 443;
//----------------------------------------
WiFiClientSecure client;

String GAS_ID = "AKfycbz44SXgv_f644JB73jLt3ROpTzK9_kNOIqRT4FxrONNOjgHnw4OJNG9Z3JMQOYtMgE3"; //--> spreadsheet script ID

Adafruit_MLX90614 mlx = Adafruit_MLX90614();

int count = 0; 
#define REPORTING_PERIOD_MS     1000
PulseOximeter pox;
uint32_t tsLastReport = 0;

int h;
int x;
int ecg;

void onBeatDetected()
{
    Serial.println("Beat!");
}
 

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password); //--> Connect to your WiFi router
  Serial.println("");

  //----------------------------------------Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(250);
    delay(250);
    //----------------------------------------
  }
  //----------------------------------------If successfully connected to the wifi router, the IP Address that will be visited is displayed in the serial monitor
  Serial.println("");
  Serial.print("Successfully connected to : ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println();
  //----------------------------------------
  client.setInsecure();
  // connect to adafruit io
  connect();
  

  pinMode(D3,INPUT); // L0+
  pinMode(D4,INPUT); // L0-
  
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C); //initialize with the I2C addr 0x3C (128x64)

  Serial.println("Adafruit MLX90614 test");  
  mlx.begin();

  Serial.print("Initializing pulse oximeter..");
 
  // Initialize the PulseOximeter instance
  // Failures are generally due to an improper I2C wiring, missing power supply
  // or wrong target chip
  if (!pox.begin()) 
    {
    Serial.println("FAILED");
  for(;;);
    }
    else 
    {
    Serial.println("SUCCESS");
    }
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
 
  // Register a callback for the beat detection
  pox.setOnBeatDetectedCallback(onBeatDetected);

  display.clearDisplay();
  display.setCursor(1,30);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Place your finger");
  display.setCursor(1,10);
  display.setTextSize(1);
  display.display();
 
}

// connect to adafruit io via MQTT
void connect() {
  Serial.print(F("Connecting to Adafruit IO... "));
  int8_t ret;
  while ((ret = mqtt.connect()) != 0) {
    switch (ret) {
      case 1: Serial.println(F("Wrong protocol")); break;
      case 2: Serial.println(F("ID rejected")); break;
      case 3: Serial.println(F("Server unavail")); break;
      case 4: Serial.println(F("Bad user/pass")); break;
      case 5: Serial.println(F("Not authed")); break;
      case 6: Serial.println(F("Failed to subscribe")); break;
      default: Serial.println(F("Connection failed")); break;
    }

    if(ret >= 0)
      mqtt.disconnect();

    Serial.println(F("Retrying connection..."));
    delay(10000);
  }
  Serial.println(F("Adafruit IO Connected!"));
}

void loop() {

 while(1){
    if(count==20)
        break;
    pox.update();
    // Asynchronously dump heart rate and oxidation levels to the serial
    // For both, a value of 0 means "invalid"
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        h=pox.getHeartRate();
        x=pox.getSpO2();
        Serial.print("Heart rate:");
        Serial.print(h);
        Serial.print("bpm / SpO2:");
        Serial.print(x);
        Serial.println("%");
        tsLastReport = millis();
        count = count +1;
        Serial.println(count);
    }
 }

 Serial.println("while loop complete");
   // ping adafruit io a few times to make sure we remain connected
  if(! mqtt.ping(3)) {
    // reconnect to adafruit io
    if(! mqtt.connected())
      connect();
  }
    
if((digitalRead(D3)==1)||(digitalRead(D4)==1))
{
  //Serial.println("!");
  display.setCursor(1,50);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Connect ECG Properly");
  display.display();
  delay(50);
}
else
{
  ecg = analogRead(A0);
  //Serial.println(ecg);
}
    
if (count == 20) {
    if((pox.getSpO2()>70) && (pox.getHeartRate()>55) && (pox.getSpO2()< 100) && (pox.getHeartRate() < 130)){

        float  temp_ambC = mlx.readAmbientTempC();
        float  t = mlx.readObjectTempC();
        float  temp_ambF = mlx.readAmbientTempF();
        float  temp_objF = mlx.readObjectTempF();

        Serial.print("Ambient = "); Serial.print(temp_ambC); 
        Serial.print("*C\tObject = "); Serial.print(t); Serial.println("*C");
        Serial.print("Ambient = "); Serial.print( temp_ambF); 
        Serial.print("*F\tObject = "); Serial.print(temp_objF); Serial.println("*F");
        Serial.println();

         //publishing data to adafruit
      
      if (! heart_rate.publish(h)) {                     //Publish to Adafruit
          Serial.println(F("Failed"));
          } 
      if (! spo2_level.publish(x)) {                     //Publish to Adafruit
          Serial.println(F("Failed"));
          }
      if (! body_temp.publish(t)) {                     //Publish to Adafruit
          Serial.println(F("Failed"));
          }
      else {
          Serial.println(F("Sent!"));
          }
  
      sendData(h, x, t); //--> Calls the sendData Subroutine
  
  delay(10); 
  display.clearDisplay();
  
  display.setCursor(1,10);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Temperature:");
  display.setCursor(80,10);
  display.setTextSize(1);
  display.print(t);
  display.print((char)247);
  display.print("C");
  display.setCursor(1,20);
  display.setTextSize(1);
  display.print("HeartRate:");
  display.setCursor(80,20);
  display.setTextSize(1);
  display.print(int(pox.getHeartRate()));// to display real time values on oled use: display.print(pox.getHeartRate());
  display.print(" bpm");
  display.setCursor(1,30);
  display.setTextSize(1);
  display.print("SpO2:");
  display.setCursor(80,30);
  display.setTextSize(1);
  display.print(pox.getSpO2());//to display real time values on oled use: display.print(pox.getSpO2());
  display.print(" %");
  display.setCursor(1,40);  
  display.setTextSize(1);
  display.println("ECG:");
  display.setCursor(80,40);
  display.setTextSize(1);
  display.print(ecg);
  
  display.display();
  delay(500);
    }
  else 
    { 
  display.clearDisplay();
  display.setCursor(1,20);  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.println("Please Wait!!");
  display.display();
  delay(500);
    }
    
    count =0;
  }

}

// Subroutine for sending data to Google Sheets
void sendData(int h, int x, float t) {
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  
  //----------------------------------------Connect to Google host
  if (!client.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  //----------------------------------------

  //----------------------------------------Processing data and sending data
  String string_hrate =  String(h); 
  String string_spo2 =  String(x, DEC);  
  String string_temp =  String(t, DEC);
  String url = "/macros/s/" + GAS_ID + "/exec?hrate=" + string_hrate + "&spo2=" + string_spo2 + "&temp=" + string_temp ;
  Serial.print("requesting URL: ");
  Serial.println(url);

  client.print(String("GET ") + url + " HTTP/1.1\r\n" +
         "Host: " + host + "\r\n" +
         "User-Agent: BuildFailureDetectorESP8266\r\n" +
         "Connection: close\r\n\r\n");

  Serial.println("request sent");
  //----------------------------------------

  //----------------------------------------Checking whether the data was sent successfully or not
  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = client.readStringUntil('\n');
  if (line.startsWith("{\"state\":\"success\"")) {
    Serial.println("esp8266/Arduino CI successfull!");
  } else {
    Serial.println("esp8266/Arduino CI has failed");
  }
  Serial.print("reply was : ");
  Serial.println(line);
  Serial.println("closing connection");
  Serial.println("==========");
  Serial.println();
  //----------------------------------------
} 
