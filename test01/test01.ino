//　OLED 9.1inch / SSD1306 に現在日時を表示するサンプルプログラム
//
// 構成
// 1. Arduino UNO R4 Wi-Fi
// 2. OLED 9.1 inch
//
//  主機能
// 1. 現在日時の表示（RTC）
// 2. Wi-Fi への接続
// 3. NTPサーバへ時刻同期（起動時、特定時刻）

// Include the RTC library
#include "RTC.h"

//Include the NTP library
#include <NTPClient.h>

#if defined(ARDUINO_PORTENTA_C33)
#include <WiFiC3.h>
#elif defined(ARDUINO_UNOWIFIR4)
#include <WiFiS3.h>
#endif

#include <WiFiUdp.h>
#include "arduino_secrets.h" 

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LOGO_HEIGHT   16
#define LOGO_WIDTH    16


///////please enter your sensitive data in the Secret tab/arduino_secrets.h
char ssid[] = SECRET_SSID;        // your network SSID (name)
char pass[] = SECRET_PASS;    // your network password (use for WPA, or use as key for WEP)

int wifiStatus = WL_IDLE_STATUS;
WiFiUDP Udp; // A UDP instance to let us send and receive packets over UDP

NTPClient timeClient(Udp);

volatile bool irqFlag = false;


void setup(){
  Serial.begin(9600);
  while (!Serial);

  connectToWiFi();
  RTC.begin();
  Serial.println("\nStarting connection to NTP server...");
  timeClient.begin();
  timeClient.update();

  // Get the current date and time from an NTP server and convert
  // it to UTC +2 by passing the time zone offset in hours.
  // You may change the time zone offset to your local one.
  auto timeZoneOffsetHours = 9;
  auto unixTime = timeClient.getEpochTime() + (timeZoneOffsetHours * 3600);
  Serial.print("Unix time = ");
  Serial.println(unixTime);
  RTCTime timeToSet = RTCTime(unixTime);
  RTC.setTime(timeToSet);

  // Retrieve the date and time from the RTC and print them
  RTCTime currentTime;
  RTC.getTime(currentTime); 
  Serial.println("The RTC was just set to: " + String(currentTime));

  if (!RTC.setPeriodicCallback(periodicCallback, Period::ONCE_EVERY_1_SEC)) {
    Serial.println("ERROR: periodic callback not set");
  }

  // Display Configuration 
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // Show initial display buffer contents on the screen --
  // the library initializes this with an Adafruit splash screen.
  //display.display();

  // 起動画面
  scrollNowLoading();
  delay(1000); // Pause for 2 seconds

  // Clear the buffer
  display.clearDisplay();

  // Draw a single pixel in white
  // display.drawPixel(10, 10, SSD1306_WHITE);

  // Show the display buffer on the screen. You MUST call display() after
  // drawing commands to make them visible on screen!
  display.display();
  delay(2000);


}

void loop(){

    // 1秒毎の時間を取得
    if(irqFlag){
    Serial.println("Timed CallBack");
    RTCTime currentTime;
    RTC.getTime(currentTime); 
    Serial.println(currentTime);
    irqFlag = false;

    // 特定時刻(23:59:59)にNTP Update

    int year = currentTime.getYear();
    int mon = Month2int(currentTime.getMonth());
    int day = currentTime.getDayOfMonth();

    int hour = currentTime.getHour();
    int min = currentTime.getMinutes();
    int sec = currentTime.getSeconds();

    if (hour == 23 && min == 59 && sec == 59){
      ntpUpdate();
    }

    displayDateTime(year, mon, day, hour, min, sec);

  
  }
}

void displayDateTime(int year, int mon, int day, int hour, int min, int sec) {

  display.clearDisplay();

  display.setTextSize(2);                     // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE);        // Draw white text
  display.setCursor(0,0);                     // Start at top-left corner

  // 桁不足の項目は０埋めをする
  display.print(year);
  display.print("-");
  if (mon < 10 ){ display.print("0");}
  display.print(mon);
  display.print("-");
  if (day < 10 ){ display.print("0");}
  display.println(day);

  if (hour < 10 ){ display.print("0");}
  display.print(hour);
  display.print(":");
  if (min < 10 ){ display.print("0");}
  display.print(min);
  display.print(":");
  if (sec < 10 ){ display.print("0");}
  display.println(sec);

  /*
  display.setTextSize(2);             // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.print(F("0x")); display.println(0xDEADBEEF, HEX);
  */

  display.display();
}


void scrollNowLoading(void) {
  display.clearDisplay();

  display.setTextSize(1); // Draw 2X-scale text
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 0);
  display.println(F(""));
  display.println(F("NOW LOADING.."));
  display.display();      // Show initial text
  delay(100);

  // Scroll in various directions, pausing in-between:
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}



void ntpUpdate(){
    Serial.println("\nUpdating RTC with NTP server...");
    timeClient.update();
}


// Callback に処理を直書きすると処理がバグる。
// https://docs.arduino.cc/tutorials/uno-r4-minima/rtc/
void periodicCallback()
{
  irqFlag = true;
}


// Wi-Fi 設定と接続状態を表示
void printWifiStatus() {
  // print the SSID of the network you're attached to:
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print your board's IP address:
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength:
  long rssi = WiFi.RSSI();
  Serial.print("signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}


// Wi-Fi APへの接続
void connectToWiFi(){
  // check for the WiFi module:
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("Communication with WiFi module failed!");
    // don't continue
    while (true);
  }

  String fv = WiFi.firmwareVersion();
  if (fv < WIFI_FIRMWARE_LATEST_VERSION) {
    Serial.println("Please upgrade the firmware");
  }

  // attempt to connect to WiFi network:
  while (wifiStatus != WL_CONNECTED) {
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(ssid);
    // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    wifiStatus = WiFi.begin(ssid, pass);

    // wait 10 seconds for connection:
    delay(10000);
  }

  Serial.println("Connected to WiFi");
  printWifiStatus();
}





