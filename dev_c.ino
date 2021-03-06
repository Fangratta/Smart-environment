#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C  lcd(0x27,2,1,0,4,5,6,7);


unsigned long previousMillis = 0;
const long interval = 1000;  
int buttonState[5];
int lastButtonState[5];

//String cov[2]={"HIGH ","Normal"};

const char* ssid = "Fangratta"; //ssid wifi
const char* password = "fang0259"; //password wifi
#define LINE_TOKEN "FcehvJYvay2TwJL5mt0nVCWUKNKX0wZJHofUKaxgUux"
const char* mqtt_server = "192.168.166.86";
const String server= "192.168.166.86";
const String folder="1483";
int cnt;

float ph;
const int analogInPin = A0; 
int sensorValue = 0; 
unsigned long int avgValue; 
float b;
int buf[10],temp;

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
    http.begin(server, 80, "/"+folder+"/cmd3.php?"+val+"");
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

float fmap(float x, float x1, float x2, float y1, float y2)
{
  return (x - x1) * (y2 - y1) / (x2 - x1) + y1;
}


void setup() {
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
  client.setCallback(callback);
  pinMode(D5,OUTPUT);
}

void callback(String topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  // If a message is received on the topic esp8266/output, you check if the message is either on or off. 
  // Turns the output according to the message received
  if(topic=="esp8266/output"){
    Serial.print("Changing output to ");
    if(messageTemp == "on"){
      digitalWrite(D5, HIGH);
      Serial.print("on");
    }
    else if(messageTemp == "off"){
      digitalWrite(D5, LOW);
      Serial.print("off");
    }
  }
  Serial.println();
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (buttonState[1] != lastButtonState[1]) {
    if (buttonState[1] == 1) {
    Line_Notify(LINE_TOKEN,"Warning%20PH%20HIGH");//%20 = Space bar ????????????????????????????????????????????????????????? ????????????????????? %20 ???????????????????????????????????????????????????
    Serial.println("Line send1");
    }
    lastButtonState[1] = buttonState[1]; 
  }

  if (buttonState[2] != lastButtonState[2]) {
    if (buttonState[2] == 1) {
    Line_Notify(LINE_TOKEN,"Warning%20PH%20LOW");//%20 = Space bar ????????????????????????????????????????????????????????? ????????????????????? %20 ???????????????????????????????????????????????????
    Serial.println("Line send2");
    }
    lastButtonState[2] = buttonState[2]; 
  }
  

  unsigned long currentMillis = millis();
if (currentMillis - previousMillis >= interval) {
  previousMillis = currentMillis;

  for(int i=0;i<10;i++) 
 { 
  buf[i]=analogRead(analogInPin);
  delay(10);
 }
 for(int i=0;i<9;i++)
 {
  for(int j=i+1;j<10;j++)
  {
   if(buf[i]>buf[j])
   {
    temp=buf[i];
    buf[i]=buf[j];
    buf[j]=temp;
   }
  }
 }
 avgValue=0;
 for(int i=2;i<8;i++)
 avgValue+=buf[i];
 float pHVol=(float)avgValue*3.3/1024/6;
 float phValue = -5.70 * pHVol + 21.34;
 ph=phValue;
  
  client.publish("esp8266/vin", String(pHVol).c_str());
  client.publish("esp8266/ph", String(ph).c_str());

if(ph>10){
    buttonState[1]=0;
    lcd.setCursor(0,1);
    lcd.print("PH HIGH  ");
    client.publish("esp8266/state", String("PH HIGH").c_str());
    buttonState[1]=1;
  }else if(ph<4){
    buttonState[1]=1;
    lcd.setCursor(0,1);
    lcd.print("PH LOW   ");
    client.publish("esp8266/state", String("PH LOW").c_str());
    buttonState[2]=1;
  }else{
    lcd.setCursor(0,1);
    lcd.print("PH NORMAL");
    client.publish("esp8266/state", String("PH NORMAL").c_str());
    buttonState[1]=0;
    buttonState[2]=0;
  }

  cnt+=1;
  if(cnt>=10){
  connect_serv("ph="+String(ph)+"");
  cnt=0;
    }
}

lcd.setCursor(0,0);
lcd.print("pH:");
lcd.print(ph,1);
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
