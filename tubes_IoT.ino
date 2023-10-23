#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <UniversalTelegramBot.h>
#include <PubSubClient.h>
#include <DHTesp.h>


// Wifi network station credentials
const char* ssid = "DM";
const char* password = "kamardimana";
const char* mqtt_server = "ee.unsoed.ac.id";

// Telegram BOT Token (Get from Botfather)
#define BOT_TOKEN "5737422704:AAFnRbtSx1ywpb2MS-3CblmhxzE5QnKBHYE"
String id = "1061140968";
const unsigned long BOT_MTBS = 1000; // mean time between scan messages
DHTesp dht;
String ip;

//spreadsheet
const char* host = "script.google.com";
const int httpsPort = 443;
String GAS_ID = "AKfycbwJ1i7yuJtK10IuJXtnEEXXlEwXZL4b1kxQeFpWfepPN3IHGALEM68aOPTBCmBY2cT_";

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiClientSecure secure_client, sclient;
UniversalTelegramBot bot(BOT_TOKEN, secure_client);
WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];
int value = 0;
int pirValue;
String status_led;
String status_relay;
String status_SSrelay;
String status_cahaya;
const int led = 5;
const int relay = 0;
const int SSrelay = 4;
const int ldr = A0;
float humidity, temperature, lightPercent;
int LightPercent, ldrStatus;

//Checks for new messages every 1 second.
int botRequestDelay = 1000;
unsigned long lastTimeBotRan;


void baca_dht11(){
  delay(dht.getMinimumSamplingPeriod());
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();
}

void baca_ldr(){
  int ldrStatus = analogRead(ldr);
  if (ldrStatus <= 300) {
    digitalWrite(led, LOW);
    status_cahaya = "Terang";
    snprintf(msg, MSG_BUFFER_SIZE, "%s", status_cahaya);
    client.publish("iot22231/kelA/cahaya", msg);
    Serial.print("Its BRIGHT, Turn on the LED : ");
    Serial.println(ldrStatus);
    status_led = "Off";
    snprintf(msg, MSG_BUFFER_SIZE, "%s", status_led);
    client.publish("iot22231/kelA/led", msg);
  } 
  else {
    digitalWrite(led, HIGH);
    status_cahaya = "Gelap";
    snprintf(msg, MSG_BUFFER_SIZE, "%s", status_cahaya);
    client.publish("iot22231/kelA/cahaya", msg);
    Serial.print("Its DARK, Turn off the LED : ");
    Serial.println(ldrStatus);
    status_led = "On";
    snprintf(msg, MSG_BUFFER_SIZE, "%s", status_led);
    client.publish("iot22231/kelA/led", msg);
  }
}

void test(){
  baca_dht11();
  baca_ldr();
  ip = String(WiFi.localIP().toString()).c_str();
  char temp[2000];

  snprintf(temp, 2000,
  "<html>\
    <head>\
      <style>\
        body {text-align: center; font-size: 15px;}\
        .button {border: none; color: white; text-align: center; textdecoration: none; font-size: 10px; margin: 4px 2px; cursor: pointer; }\
        .button1 {background-color: #4CAF50;}\
        .button2 {background-color: #ba0000;}\
      </style>\
      <meta http-equiv='refresh' content='10'/>\
      <title>TUBES IOT</title>\
    </head>\
    <body>\
      <!-- Data diri -->\
      <h4> Angggota Kelompok </h4>\
      <p>Annas Abdillah H1A020015</p>\
      <p>Febriyanti Azzahra H1A020050</p>\
      <p>Nadira Nazwa Azzahra H1A020072</p>\
      <h4>Jurusan Teknik Elektro FT Unsoed</h4>\
      <!-- alamat ip -->\
      <h4>Alamat IP Perangkat</h4>\
      <p>%s</p>\
      <!-- suhu-->\
      <h4>Pembacaan suhu dan kelembaban</h4>\
      <p>Suhu: %.02f C</p>\
      <p>Kelembapan: %.02f %</p>\
      <!-- Intensitas Cahaya-->\
      <p>Kondisi Cahaya: %s </p>\
    </body>\
  </html>",
  (String(WiFi.localIP().toString()).c_str()),temperature,humidity,status_cahaya);
  server.send(200, "text/html", temp);
}

void handleRoot() {
  test();
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
  digitalWrite(led, 0);
}

void handleNewMessages(int numNewMessages) {
  Serial.println("handleNewMessages");
  Serial.println(String(numNewMessages));
  for (int i=0; i<numNewMessages; i++) {
    String chat_id = String(bot.messages[i].chat_id);
    String text = bot.messages[i].text;
    String from_name = bot.messages[i].from_name;
    if (from_name == "") from_name = "Guest";

    //Cek Pembacaan Sensor DHT11
    if (text == "/statussuhu"){
      temperature = dht.getTemperature();
      String temp = "Suhu saat ini : ";
      temp += int(temperature);
      temp +=" *C\n";
      bot.sendMessage(chat_id,temp, "");
    }
    if (text == "/statuskelembapan"){
      humidity = dht.getHumidity();
      String temp = "Kelembaban: ";
      temp += int(humidity);
      temp += " %";
      bot.sendMessage(chat_id,temp, "");
    }
    if (text == "/ledOn"){
      digitalWrite(led,HIGH);
      status_led = "On";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_led);
      client.publish("iot22231/kelA/led", msg);
      bot.sendMessage(chat_id, "Led Menyala");
    }
    if (text == "/ledOff"){
      digitalWrite(led,LOW);
      status_led = "Off";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_led);
      client.publish("iot22231/kelA/led", msg);
      bot.sendMessage(chat_id, "Led Padam"); 
    }
    if (text == "/kipasOn"){
      digitalWrite(relay, LOW);
      status_relay = "Aktif";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_relay);
      client.publish("iot22231/kelA/relay", msg);
      bot.sendMessage(chat_id, "Relay Aktif");
    }
    if (text == "/kipasOff"){
      digitalWrite(relay, HIGH);
      status_relay = "Non-Aktif";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_relay);
      client.publish("iot22231/kelA/relay", msg);
      bot.sendMessage(chat_id, "Relay Non-Aktif"); 
    }
    if (text == "/katelOn"){
      digitalWrite(SSrelay, HIGH);
      status_SSrelay = "On";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_SSrelay);
      client.publish("iot22231/kelA/SSrelay", msg);
      bot.sendMessage(chat_id, "Teko Listrik Aktif"); 
    }
    if (text == "/katelOff"){
      digitalWrite(SSrelay, LOW);
      status_SSrelay = "Off";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_SSrelay);
      client.publish("iot22231/kelA/SSrelay", msg);
      bot.sendMessage(chat_id, "Teko Listrik Mati"); 
    }
    if (text == "/cekCahaya"){
      bot.sendMessage(chat_id, status_cahaya);
    } 

    //Cek Command untuk setiap aksi
    if (text == "/start"){
      String welcome = "Welcome " + from_name + ".\n";
      welcome += "Monitor Temperatur Ruangan\n";
      welcome += "/statussuhu : Status Suhu\n";
      welcome += "/statuskelembapan : Status Kelembapan\n";
      welcome += "----------------------------------------\n"; 
      welcome += "Intensitas Cahaya\n";
      welcome += "/cekCahaya : Cek Intensitas Cahaya\n";
      welcome += "----------------------------------------\n";
      welcome += "Kendali LED\n";
      welcome += "/ledOn : Menyalakan LED\n";
      welcome += "/ledOff : Mematikan LED\n";
      welcome += "----------------------------------------\n";
      welcome += "Kendali Kipas\n";
      welcome += "/relayOn : Menyalakan Kipas\n";
      welcome += "/relayOff : Menyalakan Kipas\n";
      welcome += "----------------------------------------\n";
      welcome += "Kendali Katel Listrik\n";
      welcome += "/katelOn : Mengaktifkan Katel\n";
      welcome += "/katelOff : Menonaktifkan Katel\n"; 
      bot.sendMessage(chat_id, welcome, "Markdown");
    }
  }
}

void spreadsheet(float tem, int hum, int valueLDR){
  Serial.println("==========");
  Serial.print("connecting to ");
  Serial.println(host);
  //----------------------------------------Connect to Google host
  if (!sclient.connect(host, httpsPort)) {
    Serial.println("connection failed");
    return;
  }
  //----------------------------------------

  //----------------------------------------Processing data and sending data
  String string_ldr = String(valueLDR);
  String string_t = String(tem);
  // String string_temperature = String(tem, DEC); 
  String string_h = String(hum, DEC); 
  String url = "/macros/s/" + GAS_ID + "/exec?temperature=" + string_t + "&humidity=" + string_h + "&kondisikipas=" + status_relay + "&nilaildr=" + string_ldr + "&kondisicahaya=" + status_cahaya + "&kondisiled=" + status_led;
  Serial.print("requesting URL: ");
  Serial.println(url);
  sclient.print(String("GET ") + url + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "User-Agent: BuildFailureDetectorESP8266\r\n" + "Connection: close\r\n\r\n");
  Serial.println("request sent");
  //----------------------------------------

  //----------------------------------------Checking whether the data was sent successfully or not
  while (sclient.connected()) {
    String line = sclient.readStringUntil('\n');
    if (line == "\r") {
      Serial.println("headers received");
      break;
    }
  }
  String line = sclient.readStringUntil('\n');
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
}

void startOTA(){
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } 
    else{ // U_FS
      type = "filesystem";
    }
    
    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
  String sTopic = topic;

  if (!strcmp(topic,"iot22231/kelA/inTopicLed")){ 
    // Switch on the LED if an 1 was received as first character
    if ((char)payload[0] == '1') { 
      digitalWrite(led, HIGH); // Turn the LED on (Note that LOW is the voltage level
      // but actually the LED is on; this is because
      // it is active low on the ESP-01)
      Serial.print("LED Menyala\n");
      status_led = "On";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_led);
      client.publish("iot22231/kelA/led", msg);
    } 
    else {
      digitalWrite(led, LOW); // Turn the LED off by making the voltage HIGH
      Serial.print("LED Padam\n");
      status_led = "Off";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_led);
      client.publish("iot22231/kelA/led", msg);
    }
  }

  if (sTopic=="iot22231/kelA/inTopicRelay"){ // topic dirubah ke string
    if ((char)payload[0] == '0') {
      Serial.println("Relay : On");
      digitalWrite(relay, HIGH);
      status_relay = "Non-Aktif";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_relay);
      client.publish("iot22231/kelA/relay", msg);
    } 
    else {
      Serial.println("Relay : On");
      digitalWrite(relay, LOW);
      status_relay = "Aktif";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_relay);
      client.publish("iot22231/kelA/relay", msg);
    }
  }

  else{
    if ((char)payload[0] == '0') {
      Serial.println("Katel Listrik : ON");
      digitalWrite(SSrelay, HIGH);
      status_SSrelay = "Katel Aktif";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_SSrelay);
      client.publish("iot22231/kelA/SSrelay", msg);
    } 
    else {
      Serial.println("Katel System : OFF");
      digitalWrite(SSrelay, LOW);
      status_SSrelay = "Katel Mati";
      snprintf(msg, MSG_BUFFER_SIZE, "%s", status_SSrelay);
      client.publish("iot22231/kelA/SSrelay", msg);
    }
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
      // Once connected, publish an announcement...
      client.publish("iot22231/kelA/outTopic1", "hello world");
      client.publish("iot22231/kelA/outTopic2", "selamat datang");
      // ... and resubscribe
      client.subscribe("iot22231/kelA/inTopicLed");
      client.subscribe("iot22231/kelA/inTopicRelay");
      client.subscribe("iot22231/kelA/inTopicSSRelay");
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}


void setup() {
  Serial.begin(115200);
  dht.setup(15, DHTesp::DHT11);
  pinMode(led, OUTPUT);
  pinMode(relay, OUTPUT);
  pinMode(SSrelay, OUTPUT);
  pinMode(ldr, INPUT);
  secure_client.setInsecure();
  sclient.setInsecure();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  httpUpdater.setup(&server);

  if (MDNS.begin("esp8266")){
    MDNS.addService("http", "tcp", 80);
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);
  server.on("/inline", [](){
  server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("HTTP server started");
}


void loop() {
  temperature = dht.getTemperature();
  humidity = dht.getHumidity();
  if (temperature >= 31){
    digitalWrite(relay, LOW);
    status_relay = "Aktif";
    snprintf(msg, MSG_BUFFER_SIZE, "%s", status_relay);
    client.publish("iot22231/kelA/relay", msg); 
  }
  else{
    digitalWrite(relay, HIGH);
    status_relay = "Non-Aktif";
    snprintf(msg, MSG_BUFFER_SIZE, "%s", status_relay);
    client.publish("iot22231/kelA/relay", msg);
  }

  baca_ldr();

  if (millis() > lastTimeBotRan + botRequestDelay){
    int numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    while(numNewMessages) {
      Serial.println("got response");
      handleNewMessages(numNewMessages);
      numNewMessages = bot.getUpdates(bot.last_message_received + 1);
    }
    lastTimeBotRan = millis();
  }

  server.handleClient();
  ArduinoOTA.handle();
  MDNS.update();

  if (!client.connected()) {
    reconnect();
  }

  client.loop();
  unsigned long now = millis();
  if (now - lastMsg > 1000) {
    baca_dht11();
    lastMsg = now;
    ++value;
    snprintf (msg, MSG_BUFFER_SIZE, "Hello World #%d", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("iot22231/kelA/outTopic1", msg);

    snprintf (msg, MSG_BUFFER_SIZE, "Selamat Datang #%d", value);
    Serial.print("Publish message: ");
    Serial.println(msg);
    client.publish("iot22231/kelA/outTopic2", msg);

    Serial.printf("Suhu = ");
    Serial.println(temperature);
    snprintf (msg, MSG_BUFFER_SIZE, String(temperature).c_str());
    client.publish("iot22231/kelA/Suhu", msg); 

    Serial.printf("Kelembaban = ");
    Serial.println(humidity);
    snprintf (msg, MSG_BUFFER_SIZE, String(humidity).c_str());
    client.publish("iot22231/kelA/Kelembaban", msg);
    delay(2000);

    Serial.printf("IP Address = ");
    Serial.println(WiFi.localIP());
    snprintf (msg, MSG_BUFFER_SIZE, String(WiFi.localIP().toString()).c_str()); 
    client.publish("iot22231/kelA/IPAddress", msg);

    Serial.printf("Chip ID = ");
    Serial.println(ESP.getFlashChipId());
    snprintf (msg, MSG_BUFFER_SIZE, String(ESP.getFlashChipId()).c_str()); 
    client.publish("iot22231/kelA/ChipID", msg);

    Serial.printf("Kondisi cahaya = ");
    Serial.println(lightPercent);
  }
  spreadsheet(temperature, humidity, ldrStatus);
}