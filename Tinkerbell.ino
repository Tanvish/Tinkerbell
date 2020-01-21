#include "Arduino.h"
#include <ESP32Servo.h>
/* Comment this out to disable prints and save space */
//#define BLYNK_DEBUG
//#define BLYNK_PRINT Serial
#define BLYNK_NO_BUILTIN
#define BLYNK_NO_FLOAT
#define CAMERA_MODEL_AI_THINKER
#include <Adafruit_NeoPixel.h>
byte red = 0, green = 0, blue = 0;
#define BLYNK_R V2
#define BLYNK_G V3
// Enable Debug interface and serial prints over UART1
#define DEGUB_ESP
Adafruit_NeoPixel pixel = Adafruit_NeoPixel(16, 12, NEO_GRB + NEO_KHZ800);

//Servo myservodumm;
//Servo myservodummm2;
//Servo myservo;
// Blynk related Libs
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>
#include <TimeLib.h>
#include <WidgetRTC.h>
#define BAUD_RATE 115200

#include "esp_camera.h"
#include "esp_timer.h"
#include "img_converters.h"

//sensor
#define TRIG 2
#define ECHO 14

#include "esp_http_server.h"
#include "fb_gfx.h"
#include "fd_forward.h"
#include "fr_forward.h"
#include "soc/soc.h"           //disable brownout problems
#include "soc/rtc_cntl_reg.h"  //disable brownout problems
//#include "dl_lib.h"

// Connection timeout;
#define CON_TIMEOUT   10*1000                     // milliseconds

// Not using Deep Sleep on PCB because TPL5110 timer takes over.
#define TIME_TO_SLEEP (uint64_t)60*60*1000*1000   // microseconds

#ifdef DEGUB_ESP
#define DBG(x) Serial.println(x)
#else 
  #define DBG(...)
#endif

// FTP Client Lib
#include "ESP32_FTPClient.h"


// Go to the Project Settings (nut icon) and get Auth Token in the Blynk App.
char auth[] = "Auth code can be found in your email.";

// Your WiFi credentials.
char ssid[] = "SSID";
char pass[] = "password";

// FTP Server credentials
char* ftp_server = "files.000webhost.com";
char* ftp_user   = "doorcam";
char* ftp_pass   = "Hgr&xMXF72vAuCqkamD1";

// Camera buffer, URL and picture name
camera_fb_t *fb = NULL;
String pic_name = "";
String pic_url  = "http://doorcam.000webhostapp.com/zyro/gallery/";

// Variable marked with this attribute will keep its value during a deep sleep / wake cycle.
RTC_DATA_ATTR uint64_t bootCount = 0;

#define vPin V1;

WidgetRTC rtc;
ESP32_FTPClient ftp (ftp_server, ftp_user, ftp_pass);

void deep_sleep(void);
void FTP_upload( void );
bool take_picture(void);

BLYNK_CONNECTED()
{
  // Synchronize time on connection
  rtc.begin();
  Blynk.syncVirtual(V1);
}
void update_pixel() {
  for (int i = 0; i < pixel.numPixels(); i++)
    pixel.setPixelColor(i, pixel.Color(red, green, blue));
  delay(1);
  pixel.show();
  delay(1);
  pixel.show();
}
BLYNK_WRITE(V2) {
  red = param.asInt(); // assign V0 value to red
  update_pixel();
}
BLYNK_WRITE(V3) {
  green = param.asInt(); // assign V1 value to red
  update_pixel();
}
void setup()
{
  pixel.begin();
  //pinMode(LPin1, OUTPUT);
  //pinMode(LPin2, OUTPUT);
  //pinMode(EnableL, OUTPUT);
  update_pixel();

  //myservo.attach(12);
  
  pinMode(TRIG, OUTPUT);
  pinMode(ECHO, INPUT);
  digitalWrite(TRIG, LOW);
    Serial.println("attaching properly");

#ifdef DEGUB_ESP
  Serial.begin(115200);
  Serial.setDebugOutput(true);
#endif


  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;
  config.pin_d0       = 5;
  config.pin_d1       = 18;
  config.pin_d2       = 19;
  config.pin_d3       = 21;
  config.pin_d4       = 36;
  config.pin_d5       = 39;
  config.pin_d6       = 34;
  config.pin_d7       = 35;
  config.pin_xclk     = 0;
  config.pin_pclk     = 22;
  config.pin_vsync    = 25;
  config.pin_href     = 23;
  config.pin_sscb_sda = 26;
  config.pin_sscb_scl = 27;
  config.pin_pwdn     = 32;
  config.pin_reset    = -1;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  //init with high specs to pre-allocate larger buffers
  config.frame_size = FRAMESIZE_XGA; // set picture size, FRAMESIZE_XGA = 1024x768
  config.jpeg_quality = 10;          // quality of JPEG output. 0-63 lower means higher quality
  config.fb_count = 2;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.print("Camera init failed with error 0x%x");
    DBG(err);
    return;
  }

  // Change extra settings if required
  //sensor_t * s = esp_camera_sensor_get();
  //s->set_vflip(s, 0);       //flip it back
  //s->set_brightness(s, 1);  //up the blightness just a bit
  //s->set_saturation(s, -2); //lower the saturation
  // Enable timer wakeup for ESP32 sleep
  esp_sleep_enable_timer_wakeup( TIME_TO_SLEEP );

  WiFi.begin( ssid, pass );
  DBG("\nConnecting to WiFi");

  while ( WiFi.status() != WL_CONNECTED && millis() < CON_TIMEOUT )
  {
    delay(500);
    Serial.print(".");
  }

  if( !WiFi.isConnected() )
  {
    DBG("Failed to connect to WiFi, going to sleep");
    deep_sleep();
  }

  DBG("");
  DBG("WiFi connected");
  DBG( WiFi.localIP() );

  Blynk.config( auth );

}
// You can also use the servo to make the doorbell ring if you can figure out a way
// to make it use a different timer setting and enough power.
/*void servo(){
  Serial.println("Going inside servo");
  for (int pos = 0; pos <= 180; pos += 1) {
    myservo.write(pos);// tell servo to go to degree pos
    delay(5);
  }
  for (int pos = 180; pos >= 0; pos -= 1) {
    myservo.write(pos);
  }
}*/

float distance = 0;
int duration;
float readDistance() {
  
  digitalWrite(TRIG, LOW);
delayMicroseconds(2);
// Sets the trigPin on HIGH state for 10 micro seconds
digitalWrite(TRIG, HIGH);
delayMicroseconds(10);
digitalWrite(TRIG, LOW);
// Reads the echoPin, returns the sound wave travel time in microseconds
 duration = pulseIn(ECHO, HIGH);
// Calculating the distance
distance= duration*0.034/2;
return distance;
}
void capture(){
  if(Blynk.connected() && timeStatus() == 2)
  {
      // Take picture
    if( take_picture() )
    {
      FTP_upload();
      //This is for the projects that run on battery.
      //deep_sleep();
    }

    else
    {
      DBG("Capture failed, sleeping");
    //  deep_sleep();
    }
  }
}
BLYNK_WRITE(V1) {
  DBG("Going in here though");// vPin is the Virtual Pin assigned to a Button Widget
int pinValue = param.asInt();
DBG("pin value");
DBG(pinValue);
 if (pinValue == 1) {
  capture();
 }
}
void loop()
{
// servo();
  Blynk.run();
float dist = readDistance();
if(dist<10){
  //servo();
  Serial.println(dist);
  Blynk.notify("There is someone at your door!");
}
}

/*void motors(){
  digitalWrite(EnableL, HIGH);
  digitalWrite(LPin1, HIGH);
  digitalWrite(LPin2, LOW);
  delay(1000);
  stop_motors();
}
*/
/*
void stop_motors(){
  digitalWrite(EnableL, LOW);
  digitalWrite(LPin1, LOW);
  digitalWrite(LPin2, LOW);
}
*/

void deep_sleep()
{
  DBG("Going to sleep after: " + String( millis() ) + "ms");
  Serial.flush();

  esp_deep_sleep_start();
}

bool take_picture()
{
  DBG("Taking picture now");

  fb = esp_camera_fb_get();  
  
  if(!fb)
  {
    DBG("Camera capture failed");
    return false;
  }
  
  // Rename the picture with the time string
  pic_name = String( now() ) + ".jpg";
  DBG("Camera capture success, saved as:");
  DBG( pic_name );
  return true;
}



void FTP_upload()
{
  DBG("Uploading via FTP");
  ftp.OpenConnection();
  
  //Create a file and write the image data to it;
  ftp.InitFile("Type I");
  ftp.ChangeWorkDir("/public_html/zyro/gallery/"); // change it to reflect your directory
  const char *f_name = pic_name.c_str();
  ftp.NewFile( f_name );
  ftp.WriteData(fb->buf, fb->len);
  ftp.CloseFile();
  esp_camera_fb_return(fb);

  // Change URL on Blynk App
  pic_url  = "http://doorcam.000webhostapp.com/zyro/gallery/";
  pic_url += pic_name;
  DBG("Change App URL to: ");
  DBG( pic_url );
  Blynk.setProperty(V0, "url", 1, pic_url);
  // Breath, withouth delay URL failed to update.
  delay(100);

}