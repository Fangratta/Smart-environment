#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C  lcd(0x27,2,1,0,4,5,6,7);

#define USE_AVG
const int sharpLEDPin = D3;   // Arduino digital pin 7 connect to sensor LED.
const int sharpVoPin = A0;   // Arduino analog pin 5 connect to sensor Vo.
// For averaging last N raw voltage readings.
#ifdef USE_AVG
#define N 100
static unsigned long VoRawTotal = 0;
static int VoRawCount = 0;
#endif // USE_AVG
// Set the typical output voltage in Volts when there is zero dust. 
static float Voc = 0.1;
const float K = 0.5;
// Helper functions to print a data value to the serial monitor.
void printValue(String text, unsigned int value, bool isLast = false) {
  Serial.print(text);
  Serial.print("=");
  Serial.print(value);
  if (!isLast) {
    Serial.print(", ");
  }
}
void printFValue(String text, float value, String units, bool isLast = false) {
  Serial.print(text);
  Serial.print("=");
  Serial.print(value);
  Serial.print(units);
  if (!isLast) {
    Serial.print(", ");
  }
}

unsigned long previousMillis = 0;
const long interval = 1000;  
int buttonState[5];
int lastButtonState[5];
float dustDensity;
int co;
String cov[2]={"HIGH","Normal"};

const char* ssid = "Fangratta"; //ssid wifi
const char* password = "fang0259"; //password wifi
#define LINE_TOKEN "FcehvJYvay2TwJL5mt0nVCWUKNKX0wZJHofUKaxgUux"
const char* mqtt_server = "192.168.166.86";
const String server= "192.168.166.86";
const String folder="1483";
int cnt;

HTTPClient http;
WiFiClient espClient;
PubSubClient client(espClient);

void Line_Notify(String token,String message) {
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  if (!client.connect("notify-api.line.me", 443)) {
    Serial.println("connection failed");
    return;   
  }
  String req = "";
  req += "POST /api/notify HTTP/1.1\r\n";
  req += "Host: notify-api.line.me\r\n";
  req += "Authorization: Bearer " + String(token) + "\r\n";
  req += "Cache-Control: no-cache\r\n";
  req += "User-Agent: ESP8266\r\n";
  req += "Content-Type: application/x-www-form-urlencoded\r\n";
  req += "Content-Length: " + String(String("message=" + message).length()) + "\r\n";
  req += "\r\n";
  req += "message=" + message;
  // Serial.println(req);
  client.print(req);
  delay(20);
  while(client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r") {
      break;
    }
    //Serial.println(line);
  }
}

void connect_serv(String val){
    if((WiFi.status() == WL_CONNECTED)) {
    http.begin(server, 80, "/"+folder+"/cmd2.php?"+val+"");
        int httpCode = http.GET();
        if(httpCode) {
            Serial.printf("[HTTP] GET... code: %d\n", httpCode);

            // file found at server
            if(httpCode == 200) {
              String payload = http.getString();
               // http.writeToStream(&USE_SERIAL);
                Serial1.println("1");
            }else{
                Serial1.println("0");
            }
        } else {
            Serial.print("[HTTP] GET... failed, no connection or no HTTP server\n");
        }
    } 
}

void setup() {
  pinMode(sharpLEDPin, OUTPUT);
  Serial.begin(9600);
  Serial.begin(9600);
  Serial1.begin(9600);
  lcd.begin (16,2); 
  lcd.setBacklightPin(3,POSITIVE);
  lcd.setBacklight(HIGH);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
  client.setServer(mqtt_server, 1883);
  pinMode(D6,OUTPUT);
  pinMode(D7,OUTPUT);
  pinMode(D5,INPUT);
  digitalWrite(D5,HIGH);
  digitalWrite(D6,HIGH);
}

void loop() {  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (buttonState[1] != lastButtonState[1]) {
    if (buttonState[1] == 1) {
    Line_Notify(LINE_TOKEN,"????????? PM 2.5???????????????????????????????????????????????????!!%20%20");//%20 = Space bar ????????????????????????????????????????????????????????? ????????????????????? %20 ???????????????????????????????????????????????????
    Serial.println("Line send");
    }
    lastButtonState[1] = buttonState[1]; 
  }

  if (buttonState[2] != lastButtonState[2]) {
    if (buttonState[2] == 1) {
    Line_Notify(LINE_TOKEN,"????????? CO ???????????????????????????????????????????????????!! %20high%20CO");//%20 = Space bar ????????????????????????????????????????????????????????? ????????????????????? %20 ???????????????????????????????????????????????????
    Serial.println("Line send");
    }
    lastButtonState[2] = buttonState[2]; 
  }
  
  // Turn on the dust sensor LED by setting digital pin LOW.
  digitalWrite(sharpLEDPin, LOW);
  // Wait 0.28ms before taking a reading of the output voltage as per spec.
  delayMicroseconds(280);
  // Record the output voltage. This operation takes around 100 microseconds.
  int VoRaw = analogRead(sharpVoPin);
  // Turn the dust sensor LED off by setting digital pin HIGH.
  digitalWrite(sharpLEDPin, HIGH);
  // Wait for remainder of the 10ms cycle = 10000 - 280 - 100 microseconds.
  delayMicroseconds(9620);
  float Vo = VoRaw;
  #ifdef USE_AVG
  VoRawTotal += VoRaw;
  VoRawCount++;
  if ( VoRawCount >= N ) {
    Vo = 1.0 * VoRawTotal / N;
    VoRawCount = 0;
    VoRawTotal = 0;
  } else {
    return;
  }
  #endif // USE_AVG

  // Compute the output voltage in Volts.
  Vo = Vo / 730.0 * 5;
  //printFValue("Vo", Vo*1000.0, "mV");

  // Convert to Dust Density in units of ug/m3.
  float dV = Vo - Voc;
  if ( dV < 0 ) {
    dV = 0;
    Voc = Vo;
  }
  dustDensity = dV / K * 100.0;
 // printFValue("DustDensity", dustDensity, "ug/m3", true);
 // Serial.println("");

  unsigned long currentMillis = millis();
if (currentMillis - previousMillis >= interval) {
  previousMillis = currentMillis;
  client.publish("esp8266/dust", String(dustDensity).c_str());
  co=digitalRead(D5);
    
    if(co==LOW){
    client.publish("esp8266/co", String("CO HIGH").c_str());
    digitalWrite(D7,LOW);
    buttonState[2]=1;
  }else{
    client.publish("esp8266/co", String("CO Normal").c_str());
    digitalWrite(D7,HIGH);
    buttonState[2]=0;
  }

  if(dustDensity>200){
    digitalWrite(D6,LOW);
    buttonState[1]=1;
  }else{
    digitalWrite(D6,HIGH);
    buttonState[1]=0;
  }
  
  cnt+=1;
  if(cnt>=10){
  connect_serv("dust="+String(dustDensity)+"");
  cnt=0;
    }

  lcd.setCursor(0,0);
  lcd.print("PM2:");
  lcd.print(dustDensity);
  lcd.print("  ");

  lcd.setCursor(0,1);
  lcd.print("CO:");
  lcd.print(cov[co]);
  lcd.print("   ");
  
  }

  
  
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str())) {
      Serial.println("connected");  
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more outputs)
      client.subscribe("esp8266/output");  
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}
