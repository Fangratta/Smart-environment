 #include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <ESP8266HTTPClient.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7);
#include <NewPing.h>
#define TRIGGER_PIN1  D3
#define ECHO_PIN1     D7
#define MAX_DISTANCE 200
NewPing sonar1(TRIGGER_PIN1, ECHO_PIN1, MAX_DISTANCE);
unsigned int uS1;
long distance[3];
long cm[3];

unsigned long previousMillis = 0;
const long interval = 1000;  

int buttonState[5];
int lastButtonState[5];

float cm_map;
int cnt;

const char* ssid = "Fangratta"; //ssid wifi
const char* password = "fang0259"; //password wifi
#define LINE_TOKEN "FcehvJYvay2TwJL5mt0nVCWUKNKX0wZJHofUKaxgUux"
const char* mqtt_server = "192.168.11.86";
const String server="192.168.11.86";
const String folder="1483";

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
    http.begin(server, 80, "/"+folder+"/cmd.php?"+val+"");
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


void setup()
{
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
  pinMode(D5,OUTPUT);
  pinMode(D6,OUTPUT);
}

void loop(){
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
delay(50); 
 uS1 = sonar1.ping();
 distance[1]=uS1 / US_ROUNDTRIP_CM;
 if(distance[1]>=5){
      cm[1]=distance[1];
 }else{
      cm[1]=0;
 }

 if (buttonState[1] != lastButtonState[1]) {
    if (buttonState[1] == 1) {
    Line_Notify(LINE_TOKEN,"Warning%20high%20water");//%20 = Space bar ห้ามเว้นวรรคเด็ดขาด ต้องใช้ %20 เท่านั้นถ้าจะเว้น
    Serial.println("Line send");
    }
    lastButtonState[1] = buttonState[1]; 
  }

 
  unsigned long currentMillis = millis();
if (currentMillis - previousMillis >= interval) {
  previousMillis = currentMillis;
   cm_map=map(cm[1],30,0,0,30); //30-0 cm to 0-30
   client.publish("esp8266/cm", String(cm[1]).c_str());
    //Serial.println("cm published");
    if(cm[1]>10 && cm[1]<=20){
    client.publish("esp8266/led1", String("Level 1").c_str());
  }else if(cm[1]>5 && cm[1]<=10){
    client.publish("esp8266/led1", String("Level 2").c_str());
  }

  cnt+=1;
  if(cnt>=10){
  connect_serv("cm="+String(cm[1])+"");
  cnt=0;
  }
  }

  lcd.setCursor(0,0);
  lcd.print("CM:");
  lcd.print(cm[1]);
  lcd.print("   ");
  /*
  lcd.print("[");
  lcd.print(cm_map);
  lcd.print("]  ");*/
  if(cm[1]>10 && cm[1]<=20){
    buttonState[1]=0;
    lcd.setCursor(0,1);
    lcd.print("Level 1");
    digitalWrite(D5,HIGH);
    digitalWrite(D6,LOW);
  }else if(cm[1]>5 && cm[1]<=10){
    buttonState[1]=1;
    digitalWrite(D6,HIGH);
    digitalWrite(D5,LOW);
    lcd.setCursor(0,1);
    lcd.print("Level 2");
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
