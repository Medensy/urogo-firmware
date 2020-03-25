#include <Arduino.h>
#include <sim7020e.h>
#include <hx711.h>

// #define LED_PIN 2
// #define BTN_PIN 13

#define LED0_PIN 25
#define LED1_PIN 33
#define BTN_PIN 32

#define HX711_DOUT_PIN 15
#define HX711_SCK_PIN 2
#define HX711_PWR_EN_PIN 13
#define ADC_BATT_EN_PIN 0
#define ADC_BATT_ADC_PIN 36

#define SD_CS0_PIN 5
#define SD_CS_PIN 14
#define SD_CLK_PIN 18
#define SD_MISO_PIN 19
#define SD_WP_PIN 21
#define SD_CD_PIN 22
#define SD_MOSI_PIN 23


#define CHRG_DT_PIN 39

#define PROG_TX_PIN 12
#define PROG_RX_PIN 35

#define SLEEP_PERIOD_US 10*1*1000000
#define TIME_SYNC_TIMEOUT 60*60*24 // time sync every 24 hours, expect 20ppm, error < 1.8s

SIM7020 sim7020;

void sim7020_test();

typedef enum {
  STATE_INITIALIZE = 0,
  STATE_MAINTENANCE,
  STATE_TIME_SYNC,
  STATE_IDLE,
  STATE_NORMAL
} state_t;

state_t state = STATE_INITIALIZE;
uint32_t last_time_sync = 0;

HX711 scale;

void setup() {
  state = STATE_INITIALIZE;
  Serial.begin(115200);

  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT);
  pinMode(HX711_PWR_EN_PIN, OUTPUT);
  // digitalWrite(BTN_PIN, HIGH);

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason) {
    case ESP_SLEEP_WAKEUP_EXT0 : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case ESP_SLEEP_WAKEUP_EXT1 : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case ESP_SLEEP_WAKEUP_TIMER : Serial.println("Wakeup caused by timer"); break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD : Serial.println("Wakeup caused by touchpad"); break;
    case ESP_SLEEP_WAKEUP_ULP : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.printf("Wakeup was not caused by deep sleep: %d\n",wakeup_reason); break;
  }

  uint8_t i;
  for(i=0;i<10;i++) {
    digitalWrite(LED0_PIN,!digitalRead(LED0_PIN));
    digitalWrite(LED1_PIN,!digitalRead(LED1_PIN));
    delay(100);
  }
  
  // while(true) {
  //   Serial.println(digitalRead(BTN_PIN));
  //   delay(100);
  // }
  
  if (digitalRead(BTN_PIN)==LOW) {
    Serial.println("entering maintenance mode");
    state = STATE_MAINTENANCE;
  } else {
    Serial.println("entering idle mode");
    state = STATE_IDLE;
  }

  state = STATE_NORMAL;
}

void loop() {
  switch (state) {
    case STATE_IDLE:
      Serial.println("idle");
      if (last_time_sync == 0 || (millis() - last_time_sync > TIME_SYNC_TIMEOUT)) {
        last_time_sync = millis();
        state = STATE_TIME_SYNC;
        // continue;
      }
      break;
    case STATE_TIME_SYNC:
      Serial.println("time sync");
      state = STATE_IDLE;
      break;
    case STATE_NORMAL:
      Serial.println("normal");
      digitalWrite(HX711_PWR_EN_PIN, HIGH);
      delay(1000);
      scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);
      while(true){
        if (scale.is_ready()) {
          long reading = scale.read();
          Serial.print("HX711 reading: ");
          Serial.println(reading);
          delay(50);
        }
      }
      digitalWrite(HX711_PWR_EN_PIN, LOW);
      state = STATE_IDLE;
      break;
    case STATE_MAINTENANCE:
      Serial.println("maintenance");
      state = STATE_IDLE;
      break;

  }
  Serial.println("sleep");
  esp_sleep_enable_timer_wakeup(SLEEP_PERIOD_US);
  esp_deep_sleep_start();
  Serial.println("wakeup");
  // digitalWrite(LED0_PIN,!digitalRead(LED0_PIN));
  // Serial.println("Hello world!");
  // delay(1000);
}


void sim7020_test() {
  sim7020.init(9600, true);
  sim7020.powerOn();
  if (sim7020.check()) {
    Serial.println("sim7020 ok");
  }
  sim7020.getModuleDetails();
  sim7020.getSignalRSSI();
  sim7020.getPacketDataProtocol();
  sim7020.getRegistrationStatus();
  sim7020.getAttachStatus();

  sim7020.setPhoneFunctionality(0);
  sim7020.setPacketDataProtocol("IPV4V6");
  sim7020.setPhoneFunctionality(1);
  sim7020.setAttachStatus(1);

  while(sim7020.getAttachStatus().indexOf("+CGATT: 1")<0) {
    sim7020.setAttachStatus(1);
    delay(1000);
  }
  Serial.println("attached to network");
  // sim7020.getSignalRSSI();
  // sim7020.getPacketDataProtocol();
  // sim7020.getRegistrationStatus();
  // sim7020.getAPN();
  // sim7020.getIPFromDNS("www.google.com");
  sim7020.ping("172.217.160.68");
  // sim7020.ping("www.google.com");
  
  // sim7020.createHTTPSocket("http://postman-echo.com",80);
  // sim7020.createHTTPSocket("http://52.22.211.29",80);
  sim7020.createHTTPSocket("http://13.229.87.104",3000);
  sim7020.listHTTPSockets();
  sim7020.connectHTTPSocket(0);
  String customer_header = "Accept: */*\r\nConnection: Keep-Alive\r\nUser-Agent: SIMCOM_MODULE\r\n";
  String encoded_customer_header = "4163636570743a202a2f2a0d0a436f6e6e656374696f6e3a204b6565702d416c6976650d0a557365722d4167656e743a2053494d434f4d5f4d4f44554c450d0a";
  assert(sim7020._stringToHexString(customer_header)==encoded_customer_header);
  uint8_t buf[512];
  customer_header.getBytes(buf,sizeof(buf),0);
  assert(sim7020._bufferToHexString(buf,customer_header.length())==encoded_customer_header);
  
  // get
  // sim7020.sendHTTPData(0,0,"/get?x=123",customer_header,"application/json","{}");
  
  // String customer_header = "Accept: */*\r\nConnection: Keep-Alive\r\nUser-Agent: SIMCOM_MODULE\r\n";
  String content_type = "application/octet-stream";
  String data = "hello ok\r\n";
  // post
  sim7020.sendHTTPData(0, 1, "/", customer_header, content_type, data);

  // sim7020.listHTTPSockets();
  sim7020.disconnectHTTPSocket(0);
  sim7020.closeHTTPSocket(0);
  sim7020.listHTTPSockets();
}