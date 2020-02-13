/*
 * Values to understand air quality
 * 350ppm Healthy, normal outside level
 * 450ppm Acceptable level
 * 600ppm Compliants os stiffness and odors
 * 1000ppm General drowsiness
 * 2500ppm Adverse health effects expected
 * 5000ppm Time weight average exposure limit < 8 hours 
 */

#include "mbedtls/md.h"
#include "time.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include "driver/adc.h"
#include "esp_sleep.h"
#include <Wire.h>    // I2C library
#include "ccs811.h"  // CCS811 library

#define SAMPLING 10               //Number of samples to take when measuring air quality
#define uS_TO_S_FACTOR 1000000    //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  600        //Time ESP32 will go to sleep (in seconds). Preferred is 10 minutes for air quality monitoring.

#define USER_ID 0                 //Place your Registration id here: to send data to the API endpoint
#define API_ENDPOINT "http://eldanor.hu/miniairq/api.php"  //URL of the API endpoint

RTC_DATA_ATTR int bootCount = 0;


// Wiring for ESP32 NodeMCU boards: VDD to 3V3, GND to GND, SDA to 21, SCL to 22, nWAKE to 23 (or GND)
CCS811 ccs811(23); // nWAKE on 23


/*
 * OTP variables
 */
String sKey = "01SAKDJAKJHFjkhs23456kjadH782"; //This needs to be the same as on the backend
const int totp_length = 6;
char totp[totp_length];

/*
 * Network settings
 */
const char* ssid = "xxx";     //  your network SSID (name) 
const char* pass = "xxx";    // your network password

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

struct tm timeinfo;

int i;

void setup() {
  Serial.begin(115200);   
  delay(500);

  bootCount++;
  Serial.print("Number of measurements: ");
  Serial.println(bootCount);

  // Enable I2C
  Wire.begin(); 
  
  // Enable CCS811
  ccs811.set_i2cdelay(50); // Needed for ESP8266 because it doesn't handle I2C clock stretch correctly
  bool ok= ccs811.begin();
  if( !ok ) Serial.println("setup: CCS811 begin FAILED");

  // Print CCS811 versions
  Serial.print("setup: hardware    version: "); Serial.println(ccs811.hardware_version(),HEX);
  Serial.print("setup: bootloader  version: "); Serial.println(ccs811.bootloader_version(),HEX);
  Serial.print("setup: application version: "); Serial.println(ccs811.application_version(),HEX);
  
  // Start measuring
  ok = ccs811.start(CCS811_MODE_1SEC);
  if( !ok ) Serial.println("setup: CCS811 start FAILED");

  /*
   * Connecting to WiFi to get and send data
   */
  Serial.println("Connecting to WiFi router");
  WiFi.begin(ssid, pass);
  i = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    i++;
    
    //If not connected after 10 tries automatic restart
    if ( i == 11 ) {
      ESP.restart();
    }
  }
  Serial.println("WiFi connected.");  
  Serial.println(WiFi.localIP());
  
  
  uint16_t eco2, etvoc, errstat, raw;
  for (i=0; i < SAMPLING; i++) {
    ccs811.read(&eco2,&etvoc,&errstat,&raw); 
    delay(1000);
  }
  
  // Print measurement results based on status
  if( errstat==CCS811_ERRSTAT_OK ) { 
    Serial.print("CCS811: ");
    Serial.print("eco2=");  Serial.print(eco2);     Serial.print(" ppm  ");
    Serial.print("etvoc="); Serial.print(etvoc);    Serial.print(" ppb  ");
    Serial.println();
  } else if( errstat==CCS811_ERRSTAT_OK_NODATA ) {
    Serial.println("CCS811: waiting for (new) data");
  } else if( errstat & CCS811_ERRSTAT_I2CFAIL ) { 
    Serial.println("CCS811: I2C error");
  } else {
    Serial.print("CCS811: errstat="); Serial.print(errstat,HEX); 
    Serial.print("="); Serial.println( ccs811.errstat_str(errstat) ); 
  }

  /* 
   *  Calculate air quality based on ppm and make it able to use by HomeKit as well
   *  HomeKit values for Air Quality
   *  AirQuality.UNKNOWN = 0;
   *  AirQuality.EXCELLENT = 1;
   *  AirQuality.GOOD = 2;
   *  AirQuality.FAIR = 3;
   *  AirQuality.INFERIOR = 4;
   *  AirQuality.POOR = 5;  
   */

  int AirQuality;
  
  if ( eco2 <= 0 ) {
    AirQuality = 0;
  }
  if ( eco2 > 0 && eco2 <= 450 ) {
    AirQuality = 1;
  }
  if ( eco2 > 450 && eco2 <= 600 ) {
    AirQuality = 2;
  }
  if ( eco2 > 600 && eco2 <= 1000 ) {
    AirQuality = 3;
  }
  if ( eco2 > 1000 && eco2 <= 2500 ) {
    AirQuality = 4;
  }
  if ( eco2 > 2500 ) {
    AirQuality = 5;
  }

  Serial.print("Air Quality: ");
  Serial.println( AirQuality );


  /*
   * Create OTP
   */
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); //Get the current time
  getLocalTime(&timeinfo);
  time_t now;
  time(&now);
  double counter = floor(now / 30); //The generated OTP will be the same for 30 seconds
  char timestamp[10];
  ltoa(counter, timestamp, 10); //Unix timestamp is the text which will be encoded
  Serial.print("Unix time: ");
  Serial.println(now);

  //Secret key padding for SHA256 coding
  sKey = sKey + sKey.substring(0, 12);
  int keyLen = sKey.length()+12;
  char key[ keyLen ];
  sKey.toCharArray(key, keyLen);

  //HMAC HASH with SHA256
  byte hmacResult[32];
   
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;
   
  const size_t timestampLength = strlen(timestamp);
  const size_t keyLength = strlen(key);

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 1);
  mbedtls_md_hmac_starts(&ctx, (const unsigned char *) key, keyLength);
  mbedtls_md_hmac_update(&ctx, (const unsigned char *) timestamp, timestampLength);
  mbedtls_md_hmac_finish(&ctx, hmacResult);
  mbedtls_md_free(&ctx);

  Serial.println("Key: "+String(key));
  Serial.println("Timestamp: "+String(timestamp));

  //Calculating TOTP value
  int offset = (int)hmacResult[sizeof(hmacResult)-1] & 0xf;
  int truncatedHash = (int)(hmacResult[offset] & 0x7f) << 24 | (hmacResult[offset+1] & 0xff) << 16 | (hmacResult[offset+2] & 0xff) << 8 | (hmacResult[offset+3] & 0xff);

  //Format TOTP value to proper length
  int finalOTP = truncatedHash % (int)pow(10, totp_length);
  sprintf(totp, "%06d", finalOTP);

  Serial.println("TOTP: "+String(totp));
  
  /*
   * Send information to the PHP backend
   */
  HTTPClient http;   
  http.begin(String(API_ENDPOINT)+"?otp="+String(totp)+"&ppm="+String(eco2)+"&tvoc="+String(etvoc)+"&airq="+String(AirQuality)+"&user_id="+String(USER_ID)); //HTTP
  int httpCode = http.GET();   
  if(httpCode > 0) {
      if(httpCode == HTTP_CODE_OK) {
        Serial.println("Sending air quality data was successful");
      }
  } else {
      Serial.printf("Set airquality failed, error: %s\n", http.errorToString(httpCode).c_str());
  }  
  http.end();

  /*
   * Going to deep sleep
   */
  Serial.println("Going to sleep now...");  
  adc_power_off();
  WiFi.mode(WIFI_OFF);
  btStop();  
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);
  esp_deep_sleep_start();
  
}

void loop() {

}
