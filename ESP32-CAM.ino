/*
  Board: AI-Thinker ESP32-CAM
  Version 1.0 with 80Mhz - no sleep.
  Version 1.1 todo sleep
  
  Tasks
    Wifi sleep mode
    On Startup create Dir
*/
//#include <iostream>           //for the send and retrive
#include <string.h>           //for the send and retrive

#include <Arduino.h>          //Für default Arduino
#include <esp_bt.h>           //Für Bluetooth
#include <WiFi.h>             //Für das Wlan
#include "OV2640.h"           //Für die Kamera
#include <WiFiClient.h>       //Keine Ahnung
#include <WebServer.h>        //Für den Webserver
#include <HTTPClient.h>       //Für den Count und das LOG
#include "camera_pins.h"      //Für die initialisierung der Kamera


#include <ESPmDNS.h>          //Für das OTA over WIFI
#include <WiFiUdp.h>          //Für das OTA over WIFI
#include <ArduinoOTA.h>       //Für das OTA over WIFI

OV2640 cam;
WebServer server(80);
//RemoteDebug Debug;
unsigned long previousMillis;
const char JHEADER[] = "HTTP/1.1 200 OK\r\n" \
                       "Content-disposition: inline; filename=capture.jpg\r\n" \
                       "Content-type: image/jpeg\r\n\r\n";
const int jhdLen = strlen(JHEADER);
const char HEADER[] = "HTTP/1.1 200 OK\r\n" \
                      "Access-Control-Allow-Origin: *\r\n" \
                      "Content-Type: multipart/x-mixed-replace; boundary=123456789000000000000987654321\r\n";
const char BOUNDARY[] = "\r\n--123456789000000000000987654321\r\n";
const char CTNTTYPE[] = "Content-Type: image/jpeg\r\nContent-Length: ";
const int hdrLen = strlen(HEADER);
const int bdrLen = strlen(BOUNDARY);
const int cntLen = strlen(CTNTTYPE);

String Read(String address) {
  HTTPClient http;
  String url = String(serverName) + "read/" + ESP32_NAME + "/" + String(address);
  http.begin(String(url));
  int httpResponseCode = http.GET();
  if (httpResponseCode > 0) {
      String response = http.getString();
      // Serial.println(httpResponseCode);
      // Serial.println(response);
      return response;
  } else {
      Serial.println("Error on HTTP request: ");
      // Serial.println(httpResponseCode);
      String notfond = String("Can't connect.");
      return notfond;
  }
  http.end();
}

void Count(String address) {
  HTTPClient http;
  String url = String(serverName) + "count/" + ESP32_NAME + "/" + String(address);
  http.begin(String(url));
  http.GET();
  http.end();
}


void writeLog(const String& message) {
  String result;
  for (size_t i = 0; i < message.length(); i++) {
    if (message[i] == ' ') {
      result += "%20";
    } else {
      result += message[i];
    }
  }
  HTTPClient http;
  String url = String(serverName) + "log/" + ESP32_NAME + "/" + String(result);
  http.begin(String(url));
  http.GET();
  http.end();
}

void handle_jpg_stream(void){
  Count("Stream");
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while searching for Stream");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while searching for Stream");
  char buf[32];
  int s;
  WiFiClient client = server.client();
  client.write(HEADER, hdrLen);
  client.write(BOUNDARY, bdrLen);

  while (true)  {
    if (!client.connected()) break;
    cam.run();
    s = cam.getSize();
    client.write(CTNTTYPE, cntLen);
    sprintf( buf, "%d\r\n\r\n", s );
    client.write(buf, strlen(buf));
    client.write((char *)cam.getfb(), s);
    client.write(BOUNDARY, bdrLen);
  }
  client.stop();
}

void handle_status(void) {
  unsigned long uptime = millis() / 1000;  // Millisekunden in Sekunden umwandeln
  WiFiClient client = server.client();
  if (!client.connected()) return;
  client.println(F("HTTP/1.1 200 OK"));
  client.println(F("Content-Type: application/json"));
  client.println(F("Connection: close\r\n"));
  String buffer;
  buffer.reserve(200);
  buffer = "{\"ok\":true, \"name\":\""ESP32_NAME"\", \"version\":\"";
  buffer += Version;
  buffer += "\", \"compile\":\"";
  buffer += String(F(__DATE__)); 
  buffer += "\", ";
  buffer += "\"Uptime\": \"" + String(uptime) + "s\", ";
  buffer += "\"state\":{";
  buffer += "\"SignalStrength\":\"" + String(WiFi.RSSI()) + "dB\", ";
  buffer += "\"IPAddress\":\"" + WiFi.localIP().toString() + "\", ";
  buffer += "\"Client\":\"" + String(client) + "\", ";
  buffer += "\"Clientadress\":\"" + server.client().remoteIP().toString() + "\", ";
  buffer += "\"MacAddress\":\"" + String(WiFi.macAddress()) + "\"}, ";
  buffer += "\"Counter\":{";
  buffer += "\"Stream\":\"" + String(Read("Stream")) + "\", ";
  buffer += "\"Status\":\"" + String(Read("Status")) + "\", ";
  buffer += "\"JPG\":\"" + String(Read("JPG")) + "\", ";
  buffer += "\"NotFound\":\"" + String(Read("NotFound")) + "\", ";
  buffer += "\"Root\":\"" + String(Read("Root")) + "\", ";
  buffer += "\"Flash\":\"" + String(Read("Flash")) + "\", ";
  buffer += "\"Reboot\":\"" + String(Read("Reboot")) + "\", ";
  buffer += "\"Start times\":\"" + String(Read("StartUP")) + "\", ";
  buffer += "\"Front LED toggle\":\"" + String(Read("FrontLED")) + "\", ";
  buffer += "\"Back LED toggle\":\"" + String(Read("BackLED")) + "\"}";
  buffer += "}";  // Close the entire JSON object

  client.println(buffer);  // Send the constructed JSON to the client
  client.stop();
  delay(200);
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while searching for Status");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while searching for Status");
  Count("Status");
}

void handle_flash(void) {
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while searching for flash");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while searching for flash");
  WiFiClient client = server.client();
  if (!client.connected()) return;
  digitalWrite(LED_BUILTIN, HIGH);  
  delay(90);
  cam.run();
  delay(100);
  digitalWrite(LED_BUILTIN, LOW);
  client.write(JHEADER, jhdLen);
  client.write((char *)cam.getfb(), cam.getSize());
  client.stop();
  Count("Flash");
}

void handle_jpg(void) {
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while searching for jpg");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while searching for jpg");
  WiFiClient client = server.client();
  if (!client.connected()) return;
  cam.run();
  client.write(JHEADER, jhdLen);
  client.write((char *)cam.getfb(), cam.getSize());
  client.stop();
  Count("JPG");
  // Serial.println(F("Foto sent to client."));
}

void handle_NotFound() {
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while search not found.");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while search not found.");
  server.send(404, "text/plain", "Not found");
  Count("NotFound");
}

void handle_root() {
  String message = "API DEFINITION\n==============\n\n";
  message += "Version 1.0:\n";
  message += "http://" + WiFi.localIP().toString() +"/\n";
  message += "http://" + WiFi.localIP().toString() +"/status\n";  
  message += "http://" + WiFi.localIP().toString() +"/jpg\n";
  message += "http://" + WiFi.localIP().toString() +"/flash\n";
  message += "http://" + WiFi.localIP().toString() +"/stream\n";
  message += "http://" + WiFi.localIP().toString() +"/reboot\n";
  message += "http://" + WiFi.localIP().toString() +"/back-led-on\n";
  message += "http://" + WiFi.localIP().toString() +"/back-led-off\n";
  message += "http://" + WiFi.localIP().toString() +"/front-led-on\n";
  message += "http://" + WiFi.localIP().toString() +"/front-led-off\n";
  server.send(200, "text/plain", message);
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while searching for Root.");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while searching for Root.");
  Count("Root");
}


void handle_reboot() {
  writeLog("Client " + server.client().remoteIP().toString() + " connected while search reboot.");
  writeLog(String(ESP32_NAME) + " will reboot now.");
  Serial.println(String(ESP32_NAME) + " will reboot now!");
  server.send(200, "text/plain", String(ESP32_NAME) + " will reboot now!");
  ESP.restart();
  Count("Reboot");
}

void initCam() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // Frame parameters
  // https://randomnerdtutorials.com/esp32-cam-ov2640-camera-settings/
  //  config.frame_size = FRAMESIZE_UXGA;
  // config.frame_size = FRAMESIZE_SXGA; 
  // config.jpeg_quality = 10; //10-63 lower number means higher quality
  // config.fb_count = 1;
  // cam.init(config);

  if(psramFound()){
    config.frame_size = FRAMESIZE_UXGA; // FRAMESIZE_ + QVGA|CIF|VGA|SVGA|XGA|SXGA|UXGA
    config.jpeg_quality = 8; //10-63 lower number means higher quality
    config.fb_count = 1;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 0);     // -2 to 2
  s->set_contrast(s, 0);       // -2 to 2
  s->set_saturation(s, 0);     // -2 to 2
  s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
  s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
  s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
  s->set_wb_mode(s, 0);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
  s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
  s->set_aec2(s, 0);           // 0 = disable , 1 = enable
  s->set_ae_level(s, 0);       // -2 to 2
  s->set_aec_value(s, 300);    // 0 to 1200
  s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
  s->set_agc_gain(s, 0);       // 0 to 30
  s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
  s->set_bpc(s, 0);            // 0 = disable , 1 = enable
  s->set_wpc(s, 1);            // 0 = disable , 1 = enable
  s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
  s->set_lenc(s, 1);           // 0 = disable , 1 = enable
  s->set_hmirror(s, 1);        // 0 = disable , 1 = enable
  s->set_vflip(s, 1);          // 0 = disable , 1 = enable
  s->set_dcw(s, 1);            // 0 = disable , 1 = enable
  s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

  cam.run();
  cam.run();
}
void handle_back_on() {
  digitalWrite(LED_BUILTIN_SMALL, LOW);
  server.send(200, "text/plain", "Background LED is ON now.");
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while searching for Background LED ON.");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while searching for Background LED ON.");
  Count("BackLED");
}

void handle_back_off() {
  digitalWrite(LED_BUILTIN_SMALL, HIGH);
  server.send(200, "text/plain", "Background LED is OFF now.");
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while searching for Background LED OFF.");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while searching for Background LED OFF.");
  Count("BackLED");
}

void handle_fornt_on() {
  digitalWrite(LED_BUILTIN, HIGH);
  server.send(200, "text/plain", "Front LED is ON now.");
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while searching for Front LED ON.");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while searching for Front LED ON.");
  Count("FrontLED");
}

void handle_front_off() {
  digitalWrite(LED_BUILTIN, LOW);
  server.send(200, "text/plain", "Front LED is OFF now.");
  Serial.println("Client " + server.client().remoteIP().toString() + " connected while searching for Front LED OFF.");
  writeLog("Client " + server.client().remoteIP().toString() + " connected while searching for Front LED OFF.");
  Count("FrontLED");
}

void initDirectory() {
  HTTPClient http;
  String url = String(serverName) + "addpc/" + ESP32_NAME;
  http.begin(String(url));
  http.GET();
  http.end();
}

void initServer() {
  server.on("/jpg",             HTTP_GET, handle_jpg);
  server.on("/flash",           HTTP_GET, handle_flash);
  server.on("/stream",          HTTP_GET, handle_jpg_stream);
  server.on("/status",          HTTP_GET, handle_status);
  server.on("/reboot",          HTTP_GET, handle_reboot);
  server.on("/back-led-on",     HTTP_GET, handle_back_on);
  server.on("/back-led-off",    HTTP_GET, handle_back_off);
  server.on("/front-led-on",    HTTP_GET, handle_fornt_on);
  server.on("/front-led-off",   HTTP_GET, handle_front_off);
  server.on("/",                HTTP_GET, handle_root);
  server.onNotFound(handle_NotFound);
  server.begin();   
  Serial.println("Webserver Started");
  writeLog("Webserver Started");
}

void initWifi() {
  int round = 0;
  WiFi.disconnect(true);
  delay(1500);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(F("."));
    if (round > 35) {
      ESP.restart();
    }
    round++;
  }
  Serial.println("");
  String ip = WiFi.localIP().toString();
  Serial.print("WiFi connected: ");
  Serial.println(WiFi.localIP());
}

void initWifiOTA() {
  // Port defaults to 3232
  ArduinoOTA.setPort(OTA_Port);

  // Hostname defaults to esp3232-[MAC]
  ArduinoOTA.setHostname(ESP32_NAME);

  // No authentication by default
  ArduinoOTA.setPassword(OTA_Password);

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA
    .onStart([]() {
      String type;
      if (ArduinoOTA.getCommand() == U_FLASH)
        type = "sketch";
      else // U_SPIFFS
        type = "filesystem";

      // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
      Serial.println("Start updating " + type);
    })
    .onEnd([]() {
      Serial.println("\nEnd");
    })
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    })
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });

  ArduinoOTA.begin();
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(LED_BUILTIN_SMALL, OUTPUT);
  digitalWrite(LED_BUILTIN_SMALL, LOW);
  Serial.begin(115200);
  Serial.println("");

  initCam();
  initWifi();
  initWifiOTA();
  initDirectory();
  initServer();

  //https://deepbluembedded.com/esp32-change-cpu-speed-clock-frequency/
  // setCpuFrequencyMhz(80);  //lower than 80 dont work for stream and WiFi
  esp_bt_controller_disable();
  esp_log_level_set("*", ESP_LOG_WARN);
  btStop();
  
  Serial.println("Cam startup succesfull!");
  writeLog("Connecting to" + String(ssid));
  writeLog("WiFi connected: " + WiFi.localIP().toString());
  Count("StartUP");

  digitalWrite(LED_BUILTIN_SMALL, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN_SMALL, LOW);
  delay(200);
  digitalWrite(LED_BUILTIN_SMALL, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN_SMALL, LOW);
  delay(200);
  digitalWrite(LED_BUILTIN_SMALL, HIGH);
  delay(200);
  digitalWrite(LED_BUILTIN_SMALL, LOW);
  delay(200);
  digitalWrite(LED_BUILTIN_SMALL, HIGH);
}


void loop() {
  server.handleClient();
  ArduinoOTA.handle();
  delay(150);
  // https://microcontrollerslab.com/reconnect-esp32-to-wifi-after-lost-connection/
  if (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("WiFi connection lost. Going for restart ..."));
    //https://www.pieterverhees.nl/sparklesagarbage/esp8266/130-difference-between-esp-reset-and-esp-restart
    // delay(20*1000);
    ESP.restart();
  }
  if (millis() / 1000 > 3600) {
    writeLog(String(ESP32_NAME) + " is running to long.");
    writeLog(String(ESP32_NAME) + " will reboot now.");
    ESP.restart();
  }

  delay(150);
  // https://www.instructables.com/ESP32-Deep-Sleep-Tutorial/  
  // https://www.mischianti.org/2021/03/10/esp32-power-saving-modem-and-light-sleep-2/
  // https://www.mischianti.org/2021/03/06/esp32-practical-power-saving-manage-wifi-and-cpu-1/
  //esp_sleep_enable_timer_wakeup(0.2 * 1000 * 1000);
  //esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
  //WiFi.setSleep(true);
  //esp_light_sleep_start();
}
