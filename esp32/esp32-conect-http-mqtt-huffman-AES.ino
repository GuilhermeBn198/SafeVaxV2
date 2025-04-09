#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <LiquidCrystal.h>
#include <AESLib.h>
#include <base64.h>
#include <map>
#include <queue>

// ======== HUFFMAN IMPLEMENTATION ========
struct HuffmanNode {
  char ch; int freq;
  HuffmanNode *left, *right;
  HuffmanNode(char c, int f): ch(c), freq(f), left(NULL), right(NULL) {}
};
struct CompareNode {
  bool operator()(HuffmanNode* a, HuffmanNode* b) { return a->freq > b->freq; }
};
HuffmanNode* huffmanTree;
std::map<char,String> huffmanCodes;

HuffmanNode* buildHuffmanTree(const String &data) {
  std::map<char,int> freq;
  for(char c: data) freq[c]++;
  std::priority_queue<HuffmanNode*,std::vector<HuffmanNode*>,CompareNode> pq;
  for(auto &p: freq) pq.push(new HuffmanNode(p.first,p.second));
  while(pq.size()>1) {
    HuffmanNode* l = pq.top(); pq.pop();
    HuffmanNode* r = pq.top(); pq.pop();
    HuffmanNode* m = new HuffmanNode('\0',l->freq+r->freq);
    m->left=l; m->right=r;
    pq.push(m);
  }
  return pq.top();
}
void buildHuffmanCodes(HuffmanNode* node, String code="") {
  if(!node) return;
  if(!node->left && !node->right) huffmanCodes[node->ch]=code;
  buildHuffmanCodes(node->left, code+"0");
  buildHuffmanCodes(node->right, code+"1");
}
String huffmanCompress(const String &s) {
  String out="";
  for(char c: s) out+=huffmanCodes[c];
  return out;
}
String huffmanDecompress(const String &bits, HuffmanNode* root) {
  String out=""; HuffmanNode* cur=root;
  for(char b: bits) {
    cur = (b=='0'?cur->left:cur->right);
    if(!cur->left && !cur->right) { out+=cur->ch; cur=root; }
  }
  return out;
}

// ======== AES‑CBC IMPLEMENTATION ========
static const uint8_t AES_KEY[32] = {
  0x60,0x3d,0xeb,0x10,0x15,0xca,0x71,0xbe,
  0x2b,0x73,0xae,0xf0,0x85,0x7d,0x77,0x81,
  0x1f,0x35,0x2c,0x07,0x3b,0x61,0x08,0xd7,
  0x2d,0x98,0x10,0xa3,0x09,0x14,0xdf,0xf4
};
AESLib aesLib;
void generateIV(uint8_t *iv) {
  for(int i=0;i<16;i++) iv[i]=random(0,256);
}
String aes_encrypt(const String &plain) {
  int len = plain.length();
  int padded = ((len+15)/16)*16;
  uint8_t iv[16]; generateIV(iv);
  uint8_t cipher[padded];
  aesLib.encryptCBC((uint8_t*)plain.c_str(), len, cipher, AES_KEY, 256, iv);
  uint8_t msg[16+padded]; memcpy(msg,iv,16); memcpy(msg+16,cipher,padded);
  int b64len = base64_enc_len(16+padded);
  char b64[b64len+1]; base64_encode(b64,(char*)msg,16+padded);
  return String(b64);
}
String aes_decrypt(const String &b64) {
  int bl = b64.length();
  int dl = base64_dec_len(b64.c_str(),bl);
  uint8_t buf[dl]; base64_decode((char*)buf,b64.c_str(),bl);
  uint8_t iv[16]; memcpy(iv,buf,16);
  int clen = dl-16; uint8_t plain[clen];
  aesLib.decryptCBC(buf+16,clen,plain,AES_KEY,256,iv);
  return String((char*)plain);
}

// ======== HARDWARE CONFIG ========
#define DHT_PIN_IN   4
#define DHT_PIN_EXT  14
#define DHT_TYPE     DHT11
DHT dht_in(DHT_PIN_IN,DHT_TYPE), dht_ext(DHT_PIN_EXT,DHT_TYPE);

#define TRIG_PIN     5
#define ECHO_PIN     25

#define LED_OK_PIN    2
#define LED_ALERT_PIN 15

#define SDA_PIN      21
#define SCL_PIN      22
Adafruit_PN532 nfc(SDA_PIN,SCL_PIN);

#define LCD_RS 26
#define LCD_E  27
#define LCD_D4 33
#define LCD_D5 32
#define LCD_D6 35
#define LCD_D7 34
LiquidCrystal lcd(LCD_RS,LCD_E,LCD_D4,LCD_D5,LCD_D6,LCD_D7);

// TIMING
const unsigned long PERIODIC_SEND_INTERVAL=60000;
const unsigned long GREEN_BLINK_INTERVAL=5000;
const unsigned long RED_BLINK_INTERVAL=1000;
const unsigned long TEMP_ALERT_DURATION=10000;
const unsigned long UNAUTH_WINDOW_BEFORE=60000;
const float distancia_limite=10.0;

// MQTT / WIFI
const char* ssid="Starlink_CIT";
const char* password="Ufrr@2024Cit";
const char* mqtt_server="cd8839ea5ec5423da3aaa6691e5183a5.s1.eu.hivemq.cloud";
const int   mqtt_port=8883;
const char* mqtt_user="hivemq.webclient.1734636778463";
const char* mqtt_pass="EU<pO3F7x?S%wLk4#5ib";

String centro="centroDeVacinaXYZ", container="containerXYZ";
String mqtt_topic="/" + centro + "/" + container;
const char* control_topic="esp32/control";

WiFiClientSecure espClient;
PubSubClient client(espClient);

// STATE
unsigned long lastSend=0, lastGreen=0, lastRed=0;
String usuario="desconhecido";
bool doorOpen=false, alarmState=false;
unsigned long lastRFID=0, tempAboveStart=0;
#define TEMP_HISTORY_SIZE 10
float tempHist[TEMP_HISTORY_SIZE]={0};
int tempIdx=0; bool avgAlert=false;

// FOR HUFFMAN SAMPLE
String sampleAlphabet = "{\"temperatura_interna\": 0.00,\"umidade_interna\": 0.00,\"temperatura_externa\": 0.00,\"umidade_externa\": 0.00,\"estado_porta\": \"Fechada\",\"usuario\": \"desconhecido\",\"alarm_state\": \"OK\",\"motivo\": \"periodic\"}";

// ======== SETUP ========
void setup() {
  Serial.begin(115200);
  randomSeed(micros());
  pinMode(LED_OK_PIN,OUTPUT); pinMode(LED_ALERT_PIN,OUTPUT);
  pinMode(TRIG_PIN,OUTPUT); pinMode(ECHO_PIN,INPUT);
  dht_in.begin(); dht_ext.begin();
  lcd.begin(16,4); lcd.print("Iniciando...");
  nfc.begin(); if(nfc.getFirmwareVersion()) nfc.SAMConfig();
  WiFi.begin(ssid,password);
  while(WiFi.status()!=WL_CONNECTED){delay(500);Serial.print(".");}
  espClient.setInsecure();
  client.setServer(mqtt_server,mqtt_port);
  client.setCallback(callback);

  // build Huffman
  huffmanTree = buildHuffmanTree(sampleAlphabet);
  buildHuffmanCodes(huffmanTree);
}

// ======== LOOP ========
void loop() {
  if(!client.connected()) {
    while(!client.connected()){
      client.connect("ESP32",mqtt_user,mqtt_pass);
      delay(5000);
    }
    client.subscribe(control_topic);
  }
  client.loop();

  // periodic send
  unsigned long now=millis();
  if(now-lastSend>=PERIODIC_SEND_INTERVAL){
    lastSend=now;
    sendData("periodic");
  }
  // LEDs
  if(!alarmState && now-lastGreen>=GREEN_BLINK_INTERVAL){
    lastGreen=now; blink(LED_OK_PIN,100);
  }
  if(alarmState && now-lastRed>=RED_BLINK_INTERVAL){
    lastRed=now; blink(LED_ALERT_PIN,100);
  }
}

// ======== FUNCTIONS ========
void blink(uint8_t pin,int d){ digitalWrite(pin,HIGH); delay(d); digitalWrite(pin,LOW); }

float measureDistance(){
  digitalWrite(TRIG_PIN,LOW); delayMicroseconds(2);
  digitalWrite(TRIG_PIN,HIGH); delayMicroseconds(10);
  digitalWrite(TRIG_PIN,LOW);
  long dur=pulseIn(ECHO_PIN,HIGH);
  return (dur*0.034)/2.0;
}

void updateTempHist(){
  float t=dht_in.readTemperature();
  tempHist[tempIdx]=t; tempIdx=(tempIdx+1)%TEMP_HISTORY_SIZE;
  if(TEMP_HISTORY_SIZE>=10){
    float s5=tempHist[tempIdx],s10=s5;
    for(int i=1;i<5;i++) s5+=tempHist[(tempIdx-i+TEMP_HISTORY_SIZE)%TEMP_HISTORY_SIZE];
    for(int i=1;i<10;i++) s10+=tempHist[(tempIdx-i+TEMP_HISTORY_SIZE)%TEMP_HISTORY_SIZE];
    if(s5/5.0 > s10/10.0 && !avgAlert){
      avgAlert=true; sendData("avg_temp_alert");
    }
    else if(s5/5.0 <= s10/10.0 && avgAlert){
      avgAlert=false; sendData("avg_temp_stable");
    }
  }
}

void checkEvents(){
  float t=dht_in.readTemperature();
  if(t>THRESHOLD_TEMP){
    if(!tempAboveStart) tempAboveStart=millis();
    else if(millis()-tempAboveStart>=TEMP_ALERT_DURATION){
      sendData("temp_high"); alarmState=true;
    }
  } else tempAboveStart=0;
}

void updateDoor(){
  bool nowOpen = measureDistance()>distancia_limite;
  if(nowOpen!=doorOpen){
    doorOpen=nowOpen;
    if(doorOpen){
      if(millis()-lastRFID>UNAUTH_WINDOW_BEFORE){
        sendData("unauthorized_door"); alarmState=true;
      }
    } else alarmState=false;
  }
}

void verifyRFID(){
  if(nfc.inListPassiveTarget()){
    uint8_t uid[7],len;
    if(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A,uid,&len)){
      String u="";
      for(int i=0;i<len;i++) u+=String(uid[i],HEX);
      if(u!=usuario){
        usuario=u; lastRFID=millis();
        if(doorOpen) sendData("rfid_door");
      }
    }
  }
}

void updateLCD(){
  float ti=dht_in.readTemperature(), te=dht_ext.readTemperature(), he=dht_ext.readHumidity();
  lcd.clear();
  lcd.setCursor(0,0); lcd.printf("Int:%.1fC Ext:%.1fC",ti,te);
  lcd.setCursor(0,1); lcd.print("Porta:"); lcd.print(doorOpen?"Aberta":"Fechada");
  lcd.setCursor(0,2); lcd.printf("Umid Ext:%.1f%%",he);
  lcd.setCursor(0,3); lcd.print("Stat:"); lcd.print(alarmState?"ALERT":"OK");
}

void sendData(const String &reason){
  updateTempHist(); verifyRFID(); updateDoor(); checkEvents(); updateLCD();
  float ti=dht_in.readTemperature(), hi=dht_in.readHumidity();
  float te=dht_ext.readTemperature(), he=dht_ext.readHumidity();
  String portState = (measureDistance()<=distancia_limite?"Fechada":"Aberta");
  String payload = "{";
  payload += "\"temperatura_interna\":" + String(ti,2) + ",";
  payload += "\"umidade_interna\":" + String(hi,2) + ",";
  payload += "\"temperatura_externa\":" + String(te,2) + ",";
  payload += "\"umidade_externa\":" + String(he,2) + ",";
  payload += "\"estado_porta\":\"" + portState + "\",";
  payload += "\"usuario\":\"" + usuario + "\",";
  payload += "\"alarm_state\":\"" + String(alarmState?"ALERT":"OK") + "\",";
  payload += "\"motivo\":\"" + reason + "\"}";
  // compress + encrypt
  String comp = huffmanCompress(payload);
  String enc = aes_encrypt(comp);
  client.publish(mqtt_topic.c_str(), enc.c_str());
}

void callback(char* topic, byte* pl, unsigned int len){
  String msg=""; for(unsigned int i=0;i<len;i++) msg+=(char)pl[i];
  if(String(topic)==control_topic){
    String dec = aes_decrypt(msg);
    String cmd = huffmanDecompress(dec,huffmanTree);
    if(cmd.startsWith("change_topic")){
      int a=cmd.indexOf(';'), b=cmd.indexOf(';',a+1);
      centro   = cmd.substring(a+1,b);
      container= cmd.substring(b+1);
      mqtt_topic="/" + centro + "/" + container;
    }
  }
}