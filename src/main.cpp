#include <Arduino.h>
// camera Web-Server mit RGB und DHCP
#include <avr-libc.h>
#include <ESP_AT_Debug.h>  //(2.3.2023)
//#include <ESP_AT_Lib_Impl.h>
//#include <ESP_AT_Lib.h>
#include <WebServer.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <EEPROM.h>
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include "SPIFFS.h"
#include <ArduinoJson.h>
//
// WARNING!!! PSRAM IC required for UXGA resolution and high JPEG quality
//            Ensure ESP32 Wrover Module or other board with PSRAM is selected
//            Partial images will be transmitted if image exceeds buffer size
//

// Select camera model
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM - Camera Ready! Use 'http://' to connect ( Timer XCamera)
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM - camera_probe(): Detected camera not supported.
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM - camera_probe(): Detected camera not supported
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM
#define CAMERA_MODEL_AI_THINKER  // Has PSRAM - Standard SimParQ Boards
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM

//curl -v -d  'WiFiNo_new=1&SSID=FRITZ!Box Fon WLAN 7390&PWD=MgMdwndrm1956&Cwait=20&Try=3' 172.23.2.153:881/WiFiSet
//curl -v -d  'RGB=G' 172.23.2.153:881/color

#include "camera_pins.h"

// ledPin refers to ESP32-CAM GPIO 4 (flashlight)
//const byte ledPin = 4;

const byte RED = 14;    // PIN14
const byte GREEN = 15;  // PIN15
const byte BLUE = 2;    // Pin2

#define RGB_OFF 0      // 000
#define RGB_RED 1      // 001
#define RGB_GREEN 2    // 010
#define RGB_BLUE 4     // 100
#define RGB_MAGENTA 5  // 101
#define RGB_CYAN 6     // 110
#define RGB_YELLOW 3   // 011
#define RGB_WHITE 7    // 111

const byte RGB_Bits[8] = { RGB_OFF, RGB_RED, RGB_GREEN, RGB_YELLOW, RGB_BLUE, RGB_MAGENTA, RGB_CYAN, RGB_WHITE };
const char RGB_Colors[8] = { 'O', 'R', 'G', 'Y', 'B', 'M', 'C', 'W' };

const byte MaxAnzahl = 10;
String key[MaxAnzahl] = {"","","","","","","","","",""};
String value[MaxAnzahl] = {"","","","","","","","","",""};

String IP_string;
String MAC_string;
String RSSI;
byte CamNo;

byte WiFiNo_new;
byte Actual_WiFi_No;

byte ConnectWait;
byte ConnectWait_new;

byte WiFiConnectMaxTry;
byte WiFiConnectMaxTry_new;

byte Reboot;  // 0 oder 128
char Color[2]{ 'B', 'R' };
byte Color1_new;
byte Color2_new;
int i_color = 0;

String SSID_new;
String PSSWD_new;
byte EEPROM_Valid_Credential_NO;
const byte MaxCredentials = 5;

int Actual_WiFi_No_EEPRPOM_OFFSET = 0;
int ConnectWait_EEPRPOM_OFFSET = 1;
int WiFiConnectMaxTry_EEPRPOM_OFFSET = 2;
int Reboot_EEPRPOM_OFFSET = 3;
int CamNo_EEPRPOM_OFFSET = 4;
int Color1_EEPRPOM_OFFSET = 5;
int Color2_EEPRPOM_OFFSET = 6;

#define Credential_Sets 5
#define Start_EE_length 10
#define SSID_EE_length 30
#define Password_EE_length 30
#define CredentialParamter_EE_length 10

//#define Crendential_Set_EE_Total_lenght (SSID_EE_length + Password_EE_length + CredentialParamter_EE_length + 1)
#define Crendential_Set_EE_Total_lenght (SSID_EE_length + Password_EE_length + CredentialParamter_EE_length)

#define EEPROM_Size (Start_EE_length + SSID_EE_length * Credential_Sets + (Password_EE_length + CredentialParamter_EE_length) * Credential_Sets)

char buff1[SSID_EE_length];
char buff2[Password_EE_length];

//int RGB_Color;

// setting PWM properties
//const int freq = 5000;

unsigned long myTime;
unsigned long BufferSize;
unsigned long WiFi_Disconnect_Interval;
unsigned long currentMillis, previousMillis;

//char* ssid = "RepeaterS23";
//char* password = "TopSecret!Service2020";
String ssid = "RepeaterS23";
String password = "TopSecret!Service2020";


//IPAddress local_IP(192, 168, 60, 39);
//IPAddress gateway(192, 168, 60, 1);
									  
IPAddress subnet(255, 255, 255, 0);

/*
String ssid = "FRITZ!Box Fon WLAN 7390";
String password = "MgMdwndrm1956";
//IPAddress local_IP(172, 23, 2, 48);
//IPAddress gateway(172, 23, 2, 50);
//IPAddress subnet(255, 255, 255, 0);
*/

const char* program_version = "Version 2023-05-22 16:45:00";

void system_params (){
    Serial.println(program_version);
    Serial.print("Compile Date: ");
    Serial.print(__DATE__);
    Serial.print(" ");
    Serial.println(__TIME__);    
    Serial.println("Time: ");
    Serial.println (__FILE__);//: Der Name der aktuellen Datei als Zeichenkette.
    Serial.println (__LINE__);//: Die aktuelle Zeilennummer als Ganzzahl.
    Serial.println (__FUNCTION__);//: Der Name der aktuellen Funktion als Zeichenkette.
    Serial.println (__PRETTY_FUNCTION__);//: Eine benutzerfreundliche Version des Namens der aktuellen Funktion als Zeichenkette (nur für C++ verfügbar).
//    Serial.println (__AVR_ARCH__);//: Die Zielarchitektur des Arduino-Boards als Zeichenkette (z.B. "avr").
//    Serial.println (__AVR_LIBC_VERSION__);//: Die Version der verwendeten AVR-Libc als Zeichenkette.
}
//float Temperature;
//float Humidity;

String IpAddressToString(const IPAddress& ipAddress) {
  return String(ipAddress[0]) + String(".") + String(ipAddress[1]) + String(".") + String(ipAddress[2]) + String(".") + String(ipAddress[3]);
}

//WebServer server(82);
WebServer server(881);

void startCameraServer();
/*
void setParameter(byte EEPROM_Credential_SetNo, byte Position, byte Parameter) {

  Serial.print("setParameter(byte EEPROM_Credential_SetNo, byte Parameter): ");
  Serial.print(EEPROM_Credential_SetNo);
  Serial.print(", Position ");
  Serial.print(Position);
  Serial.print(", Parameter ");
  Serial.println(Parameter);

  //  int addr1 = Start_EE_length + ((int)EEPROM_Credential_SetNo - 1) * Crendential_Set_EE_Total_lenght + SSID_EE_length;
  int addr1 = Crendential_Set_EE_Total_lenght * ((int)EEPROM_Credential_SetNo) + ((int)Position);

  EEPROM.put(addr1, Parameter);

  Serial.print("written to EEPROM addr : ");
  Serial.println(addr1);

  EEPROM.commit();
  Serial.println("EEPROM.commit()");
}


byte getParameter(byte EEPROM_Credential_SetNo, byte Position) {

  byte Parameter;

  Serial.print("getParameter(byte EEPROM_Credential_SetNo, byte Position): ");
  Serial.print(EEPROM_Credential_SetNo);
  Serial.print(", Position ");
  Serial.print(Position);

  //  int addr1 = Start_EE_length + ((int)EEPROM_Credential_SetNo - 1) * Crendential_Set_EE_Total_lenght + SSID_EE_length;
  int addr1 = Crendential_Set_EE_Total_lenght * ((int)EEPROM_Credential_SetNo) + ((int)Position);
  //  Parameter = EEPROM.read(addr1);
  EEPROM.get(addr1, Parameter);

  Serial.print("read from EEPROM addr : ");
  Serial.print(addr1);

  Serial.print(", Parameter ");
  Serial.println(Parameter);

  return (Parameter);
}
*/

//-------------------------------- Store WIFI credentials to EEPROM ---------
byte setWiFiCredentials(byte EEPROM_Credential_SetNo) {

  Serial.print("void setWiFiCredentials(int EEPROM_Credential_SetNo) --- EEPROM_Credential_SetNo : ");
  Serial.println(EEPROM_Credential_SetNo);
  if (EEPROM_Credential_SetNo < 1 || EEPROM_Credential_SetNo > Credential_Sets) {

    return (99);
  }

  EEPROM.write(Actual_WiFi_No_EEPRPOM_OFFSET, EEPROM_Credential_SetNo);

  Serial.print("WifI Credentials to EEPROM");
  Serial.print(SSID_new);
  Serial.print(" ");
  Serial.println(PSSWD_new);

  SSID_new.toCharArray(buff1, 30);
  PSSWD_new.toCharArray(buff2, 30);

  Serial.print("buff to EEPROM ");
  Serial.print(buff1);
  Serial.print(" ");
  Serial.println(buff2);

  int addr1 = Start_EE_length + ((int)EEPROM_Credential_SetNo - 1) * Crendential_Set_EE_Total_lenght;  //
  int addr2 = Start_EE_length + ((int)EEPROM_Credential_SetNo - 1) * Crendential_Set_EE_Total_lenght + SSID_EE_length;

  Serial.print("addr1 ");
  Serial.print(addr1);
  Serial.print(" ");
  Serial.println(buff1);
  EEPROM.writeString(addr1, buff1);

  Serial.print("addr2 ");
  Serial.print(addr2);
  Serial.print(" ");
  Serial.println(buff2);
  EEPROM.writeString(addr2, buff2);


  EEPROM.commit();
  Serial.println("EEPROM.commit()");

  return (0);
}

//-------------------------------- GET WIFI credentials from EEPROM ---------
void getWiFiCredentials(int EEPROM_Credential_SetNo) {

  Serial.print("-------- void getWiFiCredentials(int EEPROM_Credential_SetNo) ---- EEPROM_Credential_SetNo :");
  Serial.println(EEPROM_Credential_SetNo);

  int addr1 = Start_EE_length + ((int)EEPROM_Credential_SetNo - 1) * Crendential_Set_EE_Total_lenght;
  int addr2 = Start_EE_length + ((int)EEPROM_Credential_SetNo - 1) * Crendential_Set_EE_Total_lenght + SSID_EE_length;
 
// addr = Start_EE_length + ((int) EEPROM_Credential_SetNo) * (SSID_EE_length + Password_EE_length) + ((int) EEPROM_Credential_SetNo - 1) * CredentialParamter_EE_length  +  position;   

  SSID_new = EEPROM.readString(addr1);
  Serial.print("addr1 ");
  Serial.print(addr1);
  Serial.print(" ");
  Serial.println(SSID_new);

  PSSWD_new = EEPROM.readString(addr2);
  Serial.print("addr2 ");
  Serial.print(addr2);
  Serial.print(" ");
  Serial.println(PSSWD_new);

  //  EEPROM.commit();

  Serial.println("EEPROM-SSID : " + SSID_new);
  Serial.println("EEPROM-PWD  : " + PSSWD_new);

}


//++++++++++++++++++++++++++
int setParameters(byte EEPROM_Credential_SetNo, byte position, byte value) {

  int addr;
  
  Serial.print("byte setParameters(byte EEPROM_Credential_SetNo, byte position, byte value)");
  Serial.print(EEPROM_Credential_SetNo);
  Serial.print(" byte position: ");
  Serial.print(position);
  Serial.print(" byte value: ");
  Serial.print(value);

  if (EEPROM_Credential_SetNo > MaxCredentials) {
	  return (999);
  } else if (EEPROM_Credential_SetNo == 0) {
	addr = position;
	Serial.print ("EEPROM_Credential_SetNo == 0 => addr = position : ");
	Serial.println (addr);	
  } else {
	addr = Start_EE_length + ((int) EEPROM_Credential_SetNo) * (SSID_EE_length + Password_EE_length) + ((int) EEPROM_Credential_SetNo - 1) * CredentialParamter_EE_length  +  position;   
  }

  Serial.print("EEPROM before write addr ");
  Serial.print(addr);
  Serial.print(" ");
  Serial.println(value);

  EEPROM.put(addr, value);

  EEPROM.commit();

  Serial.println("EEPROM.commit()");

  EEPROM.get(addr, value);

  Serial.print("EEPROM addr READ ...");
  Serial.print(addr);
  Serial.print(" value READ ...");
  Serial.print(value);

  return (value);
}

int getParameters(byte EEPROM_Credential_SetNo, byte position) {

  int addr;
  byte value;
  
  Serial.print("byte getParameters(byte EEPROM_Credential_SetNo, byte position, byte value)");
  Serial.print(EEPROM_Credential_SetNo);
  Serial.print(" byte position: ");
  Serial.println (position);

  if (EEPROM_Credential_SetNo > MaxCredentials) {
	Serial.println ("ERROR 999 - falsches Credential Set");
	  
	  return (999);
  } else if (EEPROM_Credential_SetNo == 0) {
	addr = position;
	Serial.print ("EEPROM_Credential_SetNo == 0 => addr = position : ");
	Serial.println (addr);

  } else {
	addr = Start_EE_length + ((int) EEPROM_Credential_SetNo) * (SSID_EE_length + Password_EE_length) + ((int) EEPROM_Credential_SetNo - 1) * CredentialParamter_EE_length  +  position;   
  }

  Serial.print("EEPROM before READ addr ... ");
  Serial.print(addr);
  Serial.print(" ");

  EEPROM.get(addr, value);

  Serial.print("EEPROM addr after  READ ...");
  Serial.print(addr);
  Serial.print(" value READ ...");
  Serial.println (value);

  return (value);
}

//++++++++++++++++++++++++++

void CheckWlanStatus() {
  const String wiFiStatus[] = { "WL_IDLE_STATUS", "WL_NO_SSID_AVAIL", "WL_SCAN_COMPLETED", "WL_CONNECTED", "WL_CONNECT_FAILED", "WL_CONNECTION_LOST", "WL_DISCONNECTED" };
  static int wlanStatusAlt = -99;
  int wlanStatus = WiFi.status();
  if (wlanStatus != wlanStatusAlt) {
    Serial.print("  ...  WlanStatus: ");
    Serial.print(wlanStatus);
    Serial.print(" ");
    Serial.println(wiFiStatus[wlanStatus]);
    wlanStatusAlt = wlanStatus;
  }
}

void setup() {
  // GPIO for Flash
  // initialize digital pin ledPin as an output.

  pinMode(BLUE, OUTPUT);
  pinMode(RED, OUTPUT);
  pinMode(GREEN, OUTPUT);

  switch_color('G');

  //  delay (5000);
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println();

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
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
  //                      for larger pre-allocated frame buffer.
  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

#if defined(CAMERA_MODEL_ESP_EYE)
  pinMode(13, INPUT_PULLUP);
  pinMode(14, INPUT_PULLUP);
#endif

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();
  // initial sensors are flipped vertically and colors are a bit saturated
  if (s->id.PID == OV3660_PID) {
    s->set_vflip(s, 1);        // flip it back
    s->set_brightness(s, 1);   // up the brightness just a bit
    s->set_saturation(s, -2);  // lower the saturation
  }
  // drop down frame size for higher initial frame rate
  s->set_framesize(s, FRAMESIZE_QVGA);

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
  s->set_vflip(s, 1);
  s->set_hmirror(s, 1);
#endif


  /////////////////////////////////
  /////////////////////////////////
  /////////////////////////////////

  system_params ();

  Serial.print("EEPROM_Size = ");
  Serial.println(EEPROM_Size);

  EEPROM.begin(EEPROM_Size);

  //  setParameter (0, Actual_WiFi_No_EEPRPOM_OFFSET, 2);  //1.Byte nur zum Testen

//int EEPROM_Valid_Credential_NO = getParameters(byte EEPROM_Credential_SetNo, byte position, byte value) {
  EEPROM_Valid_Credential_NO = getParameters(0, 0);
	Serial.print ("EEPROM_Valid_Credential_NO = ");
	Serial.println (EEPROM_Valid_Credential_NO);
	
//  EEPROM_Valid_Credential_NO = getParameter(0, Actual_WiFi_No_EEPRPOM_OFFSET);          //1.Byte
//    EEPROM_Valid_Credential_NO = 0;
                                                                                        // EEPROM_Valid_Credential_NO = 0;  // zu Testzwecken
  if (EEPROM_Valid_Credential_NO < 1 || EEPROM_Valid_Credential_NO > MaxCredentials) {  // für den ersten Boot ab Werk
    EEPROM_Valid_Credential_NO = 1;
    SSID_new = ssid;
    PSSWD_new = password;
    byte status = setWiFiCredentials(EEPROM_Valid_Credential_NO);                // SSID_new, PSSWD_new
//    setParameter(0, Actual_WiFi_No_EEPRPOM_OFFSET, EEPROM_Valid_Credential_NO);  // 1 WiFi als Parameter ins EEPROM schreiben
    setParameters(Actual_WiFi_No_EEPRPOM_OFFSET, Actual_WiFi_No_EEPRPOM_OFFSET, EEPROM_Valid_Credential_NO);

    //String ssid = "FRITZ!Box Fon WLAN 7390";
    //String password = "MgMdwndrm1956";
  }
//-------

  Color1_new = getParameters(EEPROM_Valid_Credential_NO, Color1_EEPRPOM_OFFSET);
  Color2_new = getParameters(EEPROM_Valid_Credential_NO, Color2_EEPRPOM_OFFSET);

	Color[0] = (char) Color1_new;
	Color[1] = (char) Color2_new;
 
  Serial.print("Color[0] ... : ");
  Serial.print (Color[0]);
  Serial.print (" dec: ");
  Serial.println(Color1_new);
 
  Serial.print("Color[1] ... : ");
  Serial.print (Color[1]);
  Serial.print (" dec: ");
  Serial.println(Color2_new); 
  
//  EEPROM.get(ConnectWait_EEPRPOM_OFFSET, ConnectWait);
  ConnectWait = getParameters(EEPROM_Valid_Credential_NO, ConnectWait_EEPRPOM_OFFSET);
  Serial.print("EEPROM.get ... ConnectWait =  ");
  Serial.println(ConnectWait);

//  EEPROM.get(WiFiConnectMaxTry_EEPRPOM_OFFSET, WiFiConnectMaxTry);
  WiFiConnectMaxTry = getParameters(EEPROM_Valid_Credential_NO, WiFiConnectMaxTry_EEPRPOM_OFFSET);

  Serial.print("EEPROM.get ... WiFiConnectMaxTry =  ");
  Serial.println(WiFiConnectMaxTry);
//----
  //  switch_color(RGB_Colors[RGB_WHITE]);
  Actual_WiFi_No = EEPROM_Valid_Credential_NO;

  getWiFiCredentials(EEPROM_Valid_Credential_NO);
  Actual_WiFi_No = EEPROM_Valid_Credential_NO;
  ssid = SSID_new;
  password = PSSWD_new;

  //switch_color(RGB_Colors[RGB_WHITE]);
  //const byte RGB_Bits[8] = { RGB_OFF, RGB_RED, RGB_GREEN, RGB_YELLOW, RGB_BLUE, RGB_MAGENTA, RGB_CYAN, RGB_WHITE };
  //const char RGB_Colors[8] = { 'O', 'R', 'G', 'Y', 'B', 'M', 'C', 'W' };
  ////////////////////////////////

  //  WiFi.begin(ssid, password);             // Connect to the network
  Serial.print("Trying to connect to WiFi via DHCP ... ");
  Serial.print(ssid);
  Serial.print("; (");
  Serial.print(Actual_WiFi_No);
  Serial.println(")");

  //if(!WiFi.config(local_IP, gateway, subnet))
  //{Serial.println("Fehler beim IP-Adresse zuweisen!");}
  /////////////////////////////////

  Serial.print("S-SSID : " + SSID_new);
  Serial.println("; S-PWD  : " + PSSWD_new);

  SSID_new.toCharArray(buff1, 30);
  PSSWD_new.toCharArray(buff2, 30);

  //  Serial.println("C-SSID : " + buff1);
  //  Serial.println("C-PWD  : " + buff2);

  WiFi.begin(ssid.c_str(), password.c_str());

  //-----------------------------------------------------
  Serial.print("ESP Board MAC Address:  ");
  Serial.println(WiFi.macAddress());

  //----------------------------------------------------------------
  CheckWlanStatus();

  //  digitalWrite(RED, HIGH);
  //  digitalWrite(GREEN, LOW);
  //  digitalWrite(BLUE, LOW);
  switch_color_silent('R');



  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(" . ");
    Serial.print(i_color);

	if (i_color >= ConnectWait){
       Serial.print("");
       Serial.print("                   *** Reboot after WiFi connect Try n(max): ");
       Serial.println(i_color);
       Serial.print(" (");
       Serial.print(ConnectWait);
       Serial.println(") ***");

		byte Next_Valid_Credential_NO = (EEPROM_Valid_Credential_NO  % MaxCredentials) + 1;
		setParameters(0, 0, Next_Valid_Credential_NO); // Credentials for next Boot ...!
	   
       delay(5000);		
	   ESP.restart();
	} 

    i_color += 1;	
    switch_color_silent(Color[i_color % 2]);

    //switch_color(RGB_Colors[RGB_WHITE]);
    //const byte RGB_Bits[8] = { RGB_OFF, RGB_RED, RGB_GREEN, RGB_YELLOW, RGB_BLUE, RGB_MAGENTA, RGB_CYAN, RGB_WHITE };
    //const char RGB_Colors[8] = { 'O', 'R', 'G', 'Y', 'B', 'M', 'C', 'W' };
  }
  CheckWlanStatus();
  //----------------------------------------------------------------
  Serial.println("");
  Serial.println("WiFi connected");

  switch_color_silent('W');

  delay(2000);

  switch_color_silent('G');

  delay(2000);

  switch_color_silent('B');

  startCameraServer();

  Serial.print("Camera Ready! Use 'http://");
  Serial.println("' to connect");

  server.on("/data", handle_OnConnect_data);
  server.on("/WiFiSet", handle_OnConnect_WiFiSet);
  server.on("/WiFiGet", handle_OnConnect_WiFiGet);

  server.on("/ParamGet", handle_OnConnect_ParamGet);
  server.on("/ParamSet", handle_OnConnect_ParamSet);

  //~ $ curl '172.23.2.153:881/color?"RGB"="White"'
  server.on("/color", handle_OnConnect_color);  // control RGB- Colors
                                                //{"IP_string" : "172.23.2.153","MAC_string" : "94:B9:7E:F9:FA:20",## Ack: "RGB" = "White":OK~ $

  server.onNotFound(handle_NotFound);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  // put your main code here, to run repeatedly:
  //  Serial.println("' Delay ");

  BufferSize = Serial.availableForWrite();
  if (BufferSize > 40) {

    system_params ();

    myTime = millis();
    Serial.println(myTime);  // prints time since program started
    Serial.print("BS: ");
    Serial.println(BufferSize);
    Serial.print("WiFi IP: ");
    Serial.print(WiFi.localIP());

    Serial.print("; RSSI: ");
    Serial.println(WiFi.RSSI());

    Serial.print("  ESP Board MAC Address:  ");
    Serial.println(WiFi.macAddress());
  }
  ////////////////////////////////////////////////
  WiFi_Disconnect_Interval = 5000;

  currentMillis = millis();
  CheckWlanStatus();

  // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
  if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= WiFi_Disconnect_Interval)) {

    Serial.print("                 *** Reconnect Try n(max): ");
    Serial.print(i_color);
     Serial.print(" (");
     Serial.print(WiFiConnectMaxTry);
     Serial.println(") *** ");

	if (i_color >= WiFiConnectMaxTry){
		ESP.restart();
	} 

    i_color += 1;
	
    switch_color_silent(Color[1]);

    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
    previousMillis = currentMillis;
	
  } else if (WiFi.status() == WL_CONNECTED)  {

	if (i_color > 0 ) {
    switch_color_silent(Color[0]);
    };
	i_color = 0;

  } else {
    switch_color_silent(Color[0]);
  }

  CheckWlanStatus();

  ////////////////////////////////////////////////
  server.handleClient();

  delay(1000);  // wait a second so as not to send massive amounts of data

  Serial.println(WiFi.localIP());
  IP_string = IpAddressToString(WiFi.localIP());
  MAC_string = String(WiFi.macAddress());
  RSSI = String(WiFi.RSSI());

}  // ====================================== end void loop () ======================================================

void handle_OnConnect_data() {
  byte anzahl = server.args();  // Achtung Eigene + 1 für plain (als letztes)
  String param;
  byte Reboot_Flag = 0;
  Serial.print("Anzahl Arg: ");
  Serial.println(anzahl);

  for (byte i = 0; i < min(anzahl, MaxAnzahl); i++) {
    param = server.argName(i);
    //    Serial.print(param);
    //    Serial.print(" = ");

    if (server.hasArg(param)) {
      Serial.print(param);
      Serial.print(" = ");
      Serial.println(server.arg(param));
      key[i] = param;
      value[i] = server.arg(param);
      Serial.println(key[i] + " ==== " + value[i]);

      if ((String)key[i] == "CamNo") {
        CamNo = value[i].toInt();
      } else if ((String)key[i] == "Reboot") {
        if (value[i] == "TRUE") Reboot_Flag = 1;
      } else if ((String)key[i] == "WiFi") {
        WiFiNo_new = value[i].toInt();
        if (WiFiNo_new > 0 && WiFiNo_new <= Credential_Sets) {
//          setParameter(0, Actual_WiFi_No_EEPRPOM_OFFSET, WiFiNo_new);  //evtl neues WiFi für Reboot festlegen
		 setParameters(0, Actual_WiFi_No_EEPRPOM_OFFSET, (byte) WiFiNo_new);
        }
      }

      Serial.print("Kontrolle key[i] = ");
      Serial.println(CamNo);
    } else {
      Serial.print(i);
      Serial.println(" fehlt");
    }
  }
  if (Reboot_Flag == 1) ESP.restart();
  Serial.println("handle_OnConnect()");
  server.send(200, "text/html", SendHTML_data());
}

String SendHTML_data() {
  Serial.println("String SendHTML");

  String ptr = "{\"CamNo\" : \"";
  ptr += CamNo;
  ptr += "\",";
  ptr += "\"IP_string\" : \"";
  ptr += IP_string;
  ptr += "\",";
  ptr += "\"MAC_string\" : \"";
  ptr += MAC_string;
  ptr += "\",";
  ptr += "\"RSSI\" : \"";
  ptr += String(RSSI);
  ptr += "\"}";

  Serial.println("============ send back to client =====================>>>> " + ptr);

  return ptr;
}

void handle_OnConnect_ParamSet() {
  //getParameter
  byte anzahl = server.args();  // Achtung Eigene + 1 für plain (als letztes)
  String param;

  Serial.print("Anzahl Arg: ");
  Serial.println(anzahl);
  byte ERROR_FLAG = 0;
  for (byte i = 0; i < min(anzahl, MaxAnzahl); i++) {
    param = server.argName(i);
    //    Serial.print(param);
    //    Serial.print(" = ");

    if (server.hasArg(param)) {
      Serial.print(param);
      Serial.print(" = ");
      Serial.println(server.arg(param));
      key[i] = param;
      value[i] = server.arg(param);
      Serial.println(key[i] + " ==== " + value[i]);

      //curl '172.23.2.153:881/WiFiSet?WiFiNo_new=1&SSID=FritzBox&PWD=windream&Cwait=20&Try=3'
      //byte  ConnectWait;
      //byte  WiFiConnectMaxTry;
      //byte  Reboot = 0; // 0 oder 128

      if ((String)key[i] == "CamNo") {
        CamNo = value[i].toInt();
        //        EEPROM.write(CamNo_EEPRPOM_OFFSET, CamNo);
        //		EEPROM.commit();
        //        EEPROM_writeByte(CamNo_EEPRPOM_OFFSET, CamNo);
        EEPROM.put(CamNo_EEPRPOM_OFFSET, CamNo);

      } else if ((String)key[i] == "WiFiNo") {
        WiFiNo_new = value[i].toInt();
        Serial.print("WiFiNo_new = ");
        Serial.println(WiFiNo_new, WiFiNo_new);
        //        EEPROM.write(Actual_WiFi_No_EEPRPOM_OFFSET, WiFiNo_new);
        //		EEPROM.commit();
        //        EEPROM_writeByte(Actual_WiFi_No_EEPRPOM_OFFSET, WiFiNo_new);
        EEPROM.put(Actual_WiFi_No_EEPRPOM_OFFSET, WiFiNo_new);

      } else if ((String)key[i] == "Cwait") {
        ConnectWait = value[i].toInt();
        Serial.print("ConnectWait = ");
        Serial.println(ConnectWait);
        //        EEPROM.write(ConnectWait_EEPRPOM_OFFSET, ConnectWait);
        //		EEPROM.commit();
        //        EEPROM_writeByte(ConnectWait_EEPRPOM_OFFSET, ConnectWait);
        EEPROM.put(ConnectWait_EEPRPOM_OFFSET, ConnectWait);

      } else if ((String)key[i] == "Try") {
        WiFiConnectMaxTry = value[i].toInt();
        Serial.print("WiFiConnectMaxTry = ");
        Serial.println(WiFiConnectMaxTry);
        //        EEPROM.write(WiFiConnectMaxTry_EEPRPOM_OFFSET, WiFiConnectMaxTry);
        //		EEPROM.commit();
        //        EEPROM_writeByte(WiFiConnectMaxTry_EEPRPOM_OFFSET, WiFiConnectMaxTry);
        EEPROM.put(WiFiConnectMaxTry_EEPRPOM_OFFSET, WiFiConnectMaxTry);

      } else if ((String)key[i] == "Reboot") {
        Reboot = 0;
        if (value[i] == "TRUE") {
          Reboot = 1;
        }
        //       EEPROM.write(Reboot_EEPRPOM_OFFSET, Reboot);
        //		EEPROM.commit();
        //        EEPROM_writeByte(Reboot_EEPRPOM_OFFSET, Reboot);
        EEPROM.put(Reboot_EEPRPOM_OFFSET, Reboot);

      } else {
        Serial.println(key[i] + " ==== " + value[i]);
        Serial.print("ERROR wrong Keyword >>> ");
        Serial.println(key[i] + " ==== " + value[i]);
        //        ERROR_FLAG = 1;
      }


    } else {
      Serial.print(i);
      Serial.println("Ohne Parameter ....");
      //      ERROR_FLAG = 1;
    }
  }

  Serial.println("handle_OnConnect()");
  server.send(200, "text/html", SendHTML_data_ParamGet());  // .... wie bei Set
}

void handle_OnConnect_ParamGet() {
  byte anzahl = server.args();  // Achtung Eigene + 1 für plain (als letztes)
  String param;

  Serial.print("Anzahl Arg: ");
  Serial.println(anzahl);

  byte ERROR_FLAG = 0;

  for (byte i = 0; i < min(anzahl, MaxAnzahl); i++) {
    param = server.argName(i);
    //    Serial.print(param);
    //    Serial.print(" = ");

    if (server.hasArg(param)) {
      Serial.print(param);
      Serial.print(" = ");
      Serial.println(server.arg(param));
      key[i] = param;
      value[i] = server.arg(param);
      Serial.println(key[i] + " ==== " + value[i]);

    } else {
      Serial.print(i);
      Serial.println("Parameter fehlt");
      ERROR_FLAG = 1;
    }
  }
  //  CamNo = EEPROM.read(CamNo_EEPRPOM_OFFSET);
  EEPROM.get(CamNo_EEPRPOM_OFFSET, CamNo);

  Serial.print("EEPROM.get ... CamNo =  ");
  Serial.println(CamNo);

  //  Actual_WiFi_No = EEPROM.read(Actual_WiFi_No_EEPRPOM_OFFSET);
  EEPROM.get(Actual_WiFi_No_EEPRPOM_OFFSET, Actual_WiFi_No);

  Serial.print("EEPROM.get ... Actual_WiFi_No =  ");
  Serial.println(Actual_WiFi_No);

  //  ConnectWait = EEPROM.read(ConnectWait_EEPRPOM_OFFSET);
  EEPROM.get(ConnectWait_EEPRPOM_OFFSET, ConnectWait);

  Serial.print("EEPROM.get ... ConnectWait =  ");
  Serial.println(ConnectWait);

  //  WiFiConnectMaxTry = EEPROM.read(WiFiConnectMaxTry_EEPRPOM_OFFSET);
  EEPROM.get(WiFiConnectMaxTry_EEPRPOM_OFFSET, WiFiConnectMaxTry);

  Serial.print("EEPROM.get ... WiFiConnectMaxTry =  ");
  Serial.println(WiFiConnectMaxTry);

  //  Reboot = EEPROM.read(Reboot_EEPRPOM_OFFSET);
  EEPROM.get(Reboot_EEPRPOM_OFFSET, Reboot);

  Serial.print("EEPROM.get ... Reboot =  ");
  Serial.println(Reboot);

  Serial.println("handle_OnConnect()");
  server.send(200, "text/html", SendHTML_data_ParamGet());
}

String SendHTML_data_ParamGet() {
  Serial.println("String SendHTML");

  String ptr = "{\"CamNo\" : \"";
  ptr += CamNo;
  ptr += "\",";

  Serial.print("ptr ...CamNo = ");
  Serial.println(CamNo);

  ptr += "\"ConnectWait\" : \"";
  ptr += ConnectWait;
  ptr += "\",";

  Serial.print("ptr ... ConnectWait = ");
  Serial.println(ConnectWait);

  ptr += "\"WiFiConneCwaitctMaxTry\" : \"";
  ptr += WiFiConnectMaxTry;
  ptr += "\",";

  Serial.print("ptr ... WiFiConnectMaxTry = ");
  Serial.println(WiFiConnectMaxTry);

  ptr += "\"Reboot\" : \"";
  ptr += Reboot;
  ptr += "\",";

  Serial.print("ptr ... Reboot = ");
  Serial.println(Reboot);

  ptr += "\"IP_string\" : \"";
  ptr += IP_string;
  ptr += "\",";
  ptr += "\"MAC_string\" : \"";
  ptr += MAC_string;
  ptr += "\",";

  ptr += "\"Actual_WiFi_No\" : \"";
  ptr += Actual_WiFi_No;
  ptr += "\",";

  ptr += "\"WiFiNo\" : \"";
  ptr += WiFiNo_new;
  ptr += "\",";
  ptr += "\"SSID\" : \"";
  ptr += SSID_new;
  ptr += "\",";
  ptr += "\"PWD\" : \"";
  ptr += PSSWD_new;
  ptr += "\",";

  ptr += "\"Color1\" : \"";
  ptr += Color[0];
  ptr += "\",";

  ptr += "\"Color2\" : \"";
  ptr += Color[1];
  ptr += "\",";

  ptr += "\"RSSI\" : \"";
  ptr += String(RSSI);
  ptr += "\"}";

  Serial.println("============ send back to client =====================>>>> " + ptr);

  return ptr;
}

void handle_OnConnect_WiFiSet() {

  byte anzahl = server.args();  // Achtung Eigene + 1 für plain (als letztes)
  String param;
  byte credential_count = 0;

  Serial.print("Anzahl Arg: ");
  Serial.println(anzahl);
  byte ERROR_FLAG = 0;
  WiFiNo_new = 0;

  for (byte i = 0; i < min(anzahl, MaxAnzahl); i++) {
    param = server.argName(i);
    //    Serial.print(param);
    //    Serial.print(" = ");

    if (server.hasArg(param)) {
      Serial.print(param);
      Serial.print(" = ");
      Serial.println(server.arg(param));
      key[i] = param;
      value[i] = server.arg(param);
      Serial.println(key[i] + " ==== " + value[i]);


      //curl '172.23.2.153:881/WiFiSet?WiFiNo_new=1&SSID=FritzBox&PWD=windream&Cwait=20&Try=3'
      //byte  ConnectWait;
      //byte  WiFiConnectMaxTry;
      //byte  Reboot = 0; // 0 oder 128
      if ((String)key[i] == (String) "CamNo") {
        CamNo = value[i].toInt();
		setParameters(0, CamNo_EEPRPOM_OFFSET, (byte) CamNo);
		
      } else if ((String)key[i] == "WiFiNo") {
        WiFiNo_new = value[i].toInt();
        Serial.print("WiFiNo_new = ");
        Serial.println(WiFiNo_new);
//		setParameters(0, WiFi_No_EEPRPOM_OFFSET, Byte (WiFiNo_new));
		credential_count += 1;

      } else if ((String)key[i] == "SSID") {
        SSID_new = value[i];
        Serial.print("SSID_new = ");
        Serial.println(SSID_new);
		credential_count += 1;

      } else if ((String)key[i] == "PWD") {
        PSSWD_new = value[i];
        Serial.print("PSSWD_new = ");
        Serial.println(PSSWD_new);
		credential_count += 1;

      } else if ((String)key[i] == "Cwait") {
        ConnectWait_new = value[i].toInt();
        Serial.print("ConnectWait = ");
        Serial.println(ConnectWait_new);
		setParameters(WiFiNo_new, ConnectWait_EEPRPOM_OFFSET, (byte) ConnectWait_new);

      } else if ((String)key[i] == "Try") {
        WiFiConnectMaxTry_new = value[i].toInt();
        Serial.print("WiFiConnectMaxTry = ");
        Serial.println(WiFiConnectMaxTry);
		setParameters(WiFiNo_new, WiFiConnectMaxTry_EEPRPOM_OFFSET, (byte) WiFiConnectMaxTry_new);

      } else if ((String)key[i] == "Color1") {
        value[i].toCharArray(buff1, 30);
        Color1_new = buff1[0];
        //		Color[0] = value[i];
        Serial.print("Color1 = ");
        Serial.println(Color1_new);
		setParameters(WiFiNo_new, Color1_EEPRPOM_OFFSET, (byte) Color1_new);

      } else if ((String)key[i] == "Color2") {
        value[i].toCharArray(buff2, 30);
        Color2_new = buff2[0];

        //		Color[1] = value[i];
        Serial.print("Color2 = ");
        Serial.println(Color2_new);
		setParameters(WiFiNo_new, Color2_EEPRPOM_OFFSET, (byte) Color2_new);

      } else {
        Serial.println(key[i] + " ==== " + value[i]);
        Serial.print("ERROR wrong Keyword >>> ");
        Serial.println(key[i] + " ==== " + value[i]);
        ERROR_FLAG = 1;
      }


    } else {
      Serial.print(i);
      Serial.println("Parameter fehlt");
      ERROR_FLAG = 1;
    }
  }
  byte status = 50;

  if (ERROR_FLAG == 0 && credential_count == 3) {
    status = setWiFiCredentials(WiFiNo_new);  // SSID_new, PSSWD_new
  }
  Serial.println("handle_OnConnect()");
  server.send(200, "text/html", SendHTML_data_WiFiSetGet(status));
}


String SendHTML_data_WiFiSetGet(byte status) {
  Serial.println("String SendHTML");

  String ptr = "Status of new WifI credentials stored to ";
  ptr += WiFiNo_new;
  ptr += " ";
  ptr += status;
  ptr += "; ";
  ptr += "{\"CamNo\" : \"";
  ptr += CamNo;
  ptr += "\",";
  ptr += "\"IP_string\" : \"";
  ptr += IP_string;
  ptr += "\",";
  ptr += "\"MAC_string\" : \"";
  ptr += MAC_string;
  ptr += "\",";

  ptr += "\"Actual_WiFi_No\" : \"";
  ptr += Actual_WiFi_No;
  ptr += "\",";

  ptr += "\"WiFiNo\" : \"";
  ptr += WiFiNo_new;
  ptr += "\",";
  ptr += "\"SSID\" : \"";
  ptr += SSID_new;
  ptr += "\",";
  ptr += "\"PWD\" : \"";
  ptr += PSSWD_new;
  ptr += "\",";

  ptr += "\"Color1\" : \"";
//  sprintf (packetBuffer, "%c", readCard)

  ptr += (char) Color1_new;
  //sprintf (packetBuffer, "%c", readCard)
  ptr += "\",";

  ptr += "\"Color2\" : \"";
  ptr += (char) Color2_new;
  ptr += "\",";

  ptr += "\"Cwait\" : \"";
  ptr += ConnectWait_new;
  ptr += "\",";
  
  ptr += "\"Try\" : \"";
  ptr += WiFiConnectMaxTry_new;
  ptr += "\",";
  
  ptr += "\"RSSI\" : \"";
  ptr += String(RSSI);
  ptr += "\"}";

  Serial.println("============ send back to client =====================>>>> " + ptr);

  return ptr;
}


void handle_OnConnect_WiFiGet() {
  byte anzahl = server.args();  // Achtung Eigene + 1 für plain (als letztes)
  String param;

  Serial.print("Anzahl Arg: ");
  Serial.println(anzahl);

  byte ERROR_FLAG = 0;

  for (byte i = 0; i < min(anzahl, MaxAnzahl); i++) {
    param = server.argName(i);
    //    Serial.print(param);
    //    Serial.print(" = ");

    if (server.hasArg(param)) {
      Serial.print(param);
      Serial.print(" = ");
      Serial.println(server.arg(param));
      key[i] = param;
      value[i] = server.arg(param);
      Serial.println(key[i] + " ==== " + value[i]);


      //curl '172.23.2.153:881/WiFiSet?WiFiNo_new=1&SSID=FritzBox&PWD=windream&Cwait=20&Try=3'
      //byte  ConnectWait;
      //byte  WiFiConnectMaxTry;
      //byte  Reboot = 0; // 0 oder 128

      if ((String)key[i] == (String) "CamNo") {
        CamNo = value[i].toInt();
		
      } else if ((String)key[i] == (String) "WiFiNo") {
        WiFiNo_new = value[i].toInt();
        Serial.print("WiFiNo_new = ");
        Serial.println(WiFiNo_new);
		
		Color1_new = getParameters(WiFiNo_new, Color1_EEPRPOM_OFFSET);
        Serial.print("Color1 = ");
        Serial.println(Color1_new);
		
		Color2_new = getParameters(WiFiNo_new, Color2_EEPRPOM_OFFSET);
        Serial.print("Color2 = ");
        Serial.println(Color2_new);
		
		ConnectWait_new = getParameters(WiFiNo_new, ConnectWait_EEPRPOM_OFFSET);
        Serial.print("ConnectWait = ");
        Serial.println(ConnectWait);

		WiFiConnectMaxTry_new = getParameters(WiFiNo_new, WiFiConnectMaxTry_EEPRPOM_OFFSET);
        Serial.print("WiFiConnectMaxTry_new = ");
        Serial.println(WiFiConnectMaxTry_new);

        getWiFiCredentials(WiFiNo_new);
        Serial.print("WiFiNo_new = ");
        Serial.println(WiFiNo_new);
		ERROR_FLAG = 0;
      }

    } else {
      Serial.print(i);
      Serial.println("Parameter fehlt");
      ERROR_FLAG = 1;
    }
  }

  //  if (ERROR_FLAG == 0) {
  //    getWiFiCredentials(WiFiNo_new);

  //  }

  Serial.println("handle_OnConnect()");
  server.send(200, "text/html", SendHTML_data_WiFiSetGet(ERROR_FLAG));
}

void handle_OnConnect_color() {
  Serial.println("============ request from client ===================== <<<<  ");

  byte anzahl = server.args();  // Achtung Eigene + 1 für plain (als letztes)
  String param;
  Serial.print("Anzahl Arg: ");
  Serial.println(anzahl);
  String message = "";

  for (byte i = 0; i < min(anzahl, MaxAnzahl); i++) {
    param = server.argName(i);
    //    Serial.print(param);
    //    Serial.print(" = ");
    if (server.hasArg(param)) {
      //      Serial.print(param);
      //      Serial.print(" = ");
      //      Serial.println(server.arg(param));
      key[i] = param;
      value[i] = server.arg(param);
      message = "## Ack: ";
      message += key[i] + " = " + value[i] + ":";

      if ((String)key[i] == (String) "RGB") {

        Serial.println("... received" + key[i] + " = " + value[i]);

        value[i].toCharArray(buff1, 30);
        //	  char ColorCharacter = buff1[1];   //0. Char=buff1[0] = " , 1. Char=buff1[1] = ...
        message += switch_color(buff1[0]);
      } else {
        message += "ERR: Invalid Key ";
      }
    } else {
      Serial.print(i);
      Serial.println(" fehlt");
    }
  }

  Serial.println("handle_OnConnect_color()");
  server.send(200, "text/html", SendHTML_color(message));
  Serial.println("============ request from client end ===================== <<<<  ");
}

String switch_color(char ColorCharacter) {

  for (byte j = 0; j < 8; j++) {
    Serial.println(j);
    //    Serial.println(key[i] + " = " + value[i]);
    Serial.print(ColorCharacter);
    Serial.println((byte)ColorCharacter);
    Serial.print(ColorCharacter);
    Serial.print(" ?=? ");
    Serial.println(RGB_Colors[j]);

    //		Serial.println(buff1[1] + " = " + RGB_Colors [j]);		 funzt nicht

    //		if ( 0 == 1){
    if ((byte)ColorCharacter == (byte)RGB_Colors[j]) {

      byte OnOff = RGB_Bits[j];
      Serial.println(j);

      Serial.print("................. Color-Match ...... Byte: ");
      Serial.println((byte)OnOff);
      //      Serial.println((byte)(OnOff << 7));
      //      Serial.println((byte)(OnOff << 6));
      //      Serial.println((byte)(OnOff << 5));

      Serial.print("bits to write ... ");
      Serial.print((byte)(OnOff >> 0));
      Serial.print((byte)(OnOff >> 1));
      Serial.println((byte)(OnOff >> 2));

      digitalWrite(RED, 1 & (OnOff >> 0));
      digitalWrite(GREEN, 1 & (OnOff >> 1));
      digitalWrite(BLUE, 1 & (OnOff >> 2));

      Serial.print("bits read ... ");

      Serial.print(" R: ");
      Serial.print(digitalRead(RED));

      Serial.print(" G: ");
      Serial.print(digitalRead(GREEN));

      Serial.print(" B: ");
      Serial.println(digitalRead(BLUE));

      Serial.println("................END SET GPIOs RED, GREEN BLUE ..................");
      return "OK";
    } else {
      Serial.println("................No Color Match ..................");
    }
  }
  return "NOK";
}


String switch_color_silent(char ColorCharacter) {

  for (byte j = 0; j < 8; j++) {

    if ((byte)ColorCharacter == (byte)RGB_Colors[j]) {

      byte OnOff = RGB_Bits[j];

      digitalWrite(RED, 1 & (OnOff >> 0));
      digitalWrite(GREEN, 1 & (OnOff >> 1));
      digitalWrite(BLUE, 1 & (OnOff >> 2));

      return "OK";
    }
  }
  Serial.println("....switch_color_silent............No Color Match ..................");
  return "NOK";
}


//================================================== RGB - Colors ======================================================================

void handle_NotFound() {
  Serial.println("handle_NotFound()");

  server.send(404, "text/plain", "Error - undefined");
}

// --------------------------------- end Handles -------------------------------------------

String SendHTML_color(String Message) {
  Serial.println("String SendHTML");

  String ptr = "{\"IP_string\" : \"";
  ptr += IP_string;
  ptr += "\",";

  ptr += "\"MAC_string\" : \"";
  ptr += MAC_string;
  ptr += "\",";
  ptr += "\"RSSI\" : \"";
  ptr += String(RSSI);
  ptr += "\",";

  //  ptr += "\"RGB\" : \"";
  //  ptr += "OFF";
  //  ptr += "\"}";
  ptr += Message;
  return ptr;
}
/*
// Return JSON with Current Output States
String getOutputStates(){
const int NUM_OUTPUTS = 4 ;
int outputGPIOs[NUM_OUTPUTS] = {2, 4, 12, 14};

JSONVar myArray;
for (int i =0; i<NUM_OUTPUTS; i++){
myArray["gpios"][i]["output"] = String(outputGPIOs[i]);
myArray["gpios"][i]["state"] = String(digitalRead(outputGPIOs[i]));
}
String jsonString = JSON.stringify(myArray);
Serial.print(jsonString);
return jsonString;
}
*/