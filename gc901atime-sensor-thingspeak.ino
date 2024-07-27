#include <WiFi.h>
#include "time.h"
#include "ThingSpeak.h"
//#include "esp_sntp.h"
#include <Arduino.h>
#include <TFT_eSPI.h> // Master copy here: https://github.com/Bodmer/TFT_eSPI
#include <SPI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

const char *ssid = "Mohammed Abdulla";
const char *password = "Buomar03";

const char *ntpServer1 = "asia.pool.ntp.org";
const char *ntpServer2 = "time.nist.gov";
const long gmtOffset_sec = 5400;
const int daylightOffset_sec = 5400;

WiFiClient espClient;
unsigned long myChannelNumber = 1;
const char * myWriteAPIKey = "PUZSHIRXCNB5N339";

Adafruit_MPU6050 mpu;
int step_count=0;
float y=0.0;
float y_old=0.0;
float delta= 0.0;


TFT_eSPI tft = TFT_eSPI();  // Invoke library, pins defined in User_Setup.h
TFT_eSprite timedate    = TFT_eSprite(&tft); // Sprite for clock reading

const char *time_zone = "CET-1CEST,M3.5.0,M10.5.0/3";  // TimeZone rule for Europe/Rome including daylight adjustment rules (optional)


void setup() {
  tft.init();

  tft.fillScreen(TFT_BLACK);
//  tft.drawRect(0, 0, tft.width(), tft.height(), TFT_GREEN);

//  // Set "cursor" at top left corner of display (0,0) and select font 4
  tft.setCursor(2, 2, 4);

  // Set the font colour to be white with a black background
  tft.setTextColor(TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  Serial.begin(115200);

  // First step is to configure WiFi STA and connect in order to get the current time and date.
  Serial.print("Connecting to %s ");
  Serial.print(ssid);
  WiFi.begin(ssid, password);

  /**
   * NTP server address could be acquired via DHCP,
   *
   * NOTE: This call should be made BEFORE esp32 acquires IP address via DHCP,
   * otherwise SNTP option 42 would be rejected by default.
   * NOTE: configTime() function call if made AFTER DHCP-client run
   * will OVERRIDE acquired NTP server address
   */
//  esp_sntp_servermode_dhcp(1);  // (optional)

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" CONNECTED");
  
  tft.println("CONNECTED");

  // set notification call-back function
//  sntp_set_time_sync_notification_cb(timeavailable);

  /**
   * This will set configured ntp servers and constant TimeZone/daylightOffset
   * should be OK if your time zone does not need to adjust daylightOffset twice a year,
   * in such a case time adjustment won't be handled automagically.
   */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

  /**
   * A more convenient approach to handle TimeZones with daylightOffset
   * would be to specify a environment variable with TimeZone definition including daylight adjustmnet rules.
   * A list of rules for your zone could be obtained from https://github.com/esp8266/Arduino/blob/master/cores/esp8266/TZ.h
   */
  //configTzTime(time_zone, ntpServer1, ntpServer2);
  Serial.println("Adafruit MPU6050 test!");

  // Try to initialize!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  Serial.print("Accelerometer range set to: ");
  switch (mpu.getAccelerometerRange()) {
  case MPU6050_RANGE_2_G:
    Serial.println("+-2G");
    break;
  case MPU6050_RANGE_4_G:
    Serial.println("+-4G");
    break;
  case MPU6050_RANGE_8_G:
    Serial.println("+-8G");
    break;
  case MPU6050_RANGE_16_G:
    Serial.println("+-16G");
    break;
  }
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  Serial.print("Gyro range set to: ");
  switch (mpu.getGyroRange()) {
  case MPU6050_RANGE_250_DEG:
    Serial.println("+- 250 deg/s");
    break;
  case MPU6050_RANGE_500_DEG:
    Serial.println("+- 500 deg/s");
    break;
  case MPU6050_RANGE_1000_DEG:
    Serial.println("+- 1000 deg/s");
    break;
  case MPU6050_RANGE_2000_DEG:
    Serial.println("+- 2000 deg/s");
    break;
  }

  mpu.setFilterBandwidth(MPU6050_BAND_5_HZ);
  Serial.print("Filter bandwidth set to: ");
  switch (mpu.getFilterBandwidth()) {
  case MPU6050_BAND_260_HZ:
    Serial.println("260 Hz");
    break;
  case MPU6050_BAND_184_HZ:
    Serial.println("184 Hz");
    break;
  case MPU6050_BAND_94_HZ:
    Serial.println("94 Hz");
    break;
  case MPU6050_BAND_44_HZ:
    Serial.println("44 Hz");
    break;
  case MPU6050_BAND_21_HZ:
    Serial.println("21 Hz");
    break;
  case MPU6050_BAND_10_HZ:
    Serial.println("10 Hz");
    break;
  case MPU6050_BAND_5_HZ:
    Serial.println("5 Hz");
    break;
  }

  Serial.println("");
  delay(100);
  ThingSpeak.begin(espClient);  // Initialize ThingSpeak


}
void printLocalTime() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    tft.setCursor(70, 70, 4);
    tft.println("No time available (yet)");
    return;
  }
  tft.setCursor(50, 70, 4);
  char timeWeekDay[10];
  strftime(timeWeekDay,10, "%A", &timeinfo);
  char firstFour[3];
  strncpy(firstFour, timeWeekDay, 3);
  firstFour[3] = '\0'; // Terminate the string
  Serial.println(firstFour);
  Serial.println();
  tft.print(firstFour);
  tft.print(&timeinfo, " , %B %d ");
  tft.println("");
  tft.setCursor(60, 100, 6);
//  tft.println(&timeinfo, "%B %d %Y");
//  tft.setCursor(60, 130, 2);
  tft.println(&timeinfo,"%H:%M");
  delay(5000);
}

// Callback function (gets called when time adjusts via NTP)
void timeavailable(struct timeval *t) {
  tft.setCursor(70, 70, 4);
  tft.println("Got time adjustment from NTP!");
  printLocalTime();
}

void step_counter(){
    /* Get new sensor events with the readings */
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  /* Print out the values */
  Serial.print("Acceleration X: ");
  Serial.print(a.acceleration.x);
  Serial.print(", Y: ");
  Serial.print(a.acceleration.y);
  Serial.println("");
  y=a.acceleration.y;
  delta=abs(y_old-y);
  if (delta>1) {
  step_count=step_count+1;
}
  Serial.print("Steps:");
  Serial.print(step_count);
  tft.setCursor(75, 150, 4);
  tft.print("Steps:");
  tft.print(step_count);
  y_old=a.acceleration.y;
  int x = ThingSpeak.writeField(myChannelNumber,1, step_count, myWriteAPIKey);

  if(x == 200){
      Serial.println("Channel update successful.");
    }
  else{
      Serial.println("Problem updating channel. HTTP error code " + String(x));
    }

}

void loop() {
  
  delay(5000);
  
  tft.fillScreen(TFT_BLACK);
  step_counter();
  printLocalTime();  // it will take some time to sync time :)
  

  
}
