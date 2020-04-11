#include <Arduino.h>
#include <sdcard.h>
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

#define HX711_INIT_TIMEOUT 2500
#define SIM7020_INIT_TIMEOUT 10000

#define SLEEP_PERIOD_US 1*1*1000000 // 
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

SDCARD sdcard;
HX711 scale;

#define MAX_DATA_LENGTH 1800 // sampling rate 10 SPS, record time 180 s
int32_t data_buf[MAX_DATA_LENGTH];
size_t data_idx = 0;

bool init_board(void);
bool init_sdcard(void);
bool init_hx711(void);
bool init_nbiot(void);

void deinit_board(void);
void deinit_sdcard(void);
void deinit_hx711(void);
void deinit_nbiot(void);

bool self_test(void);

void sync_time(void);
size_t collect_data(int32_t *buffer, size_t max_length, uint32_t timeout);
void save_data_to_sdcard(String file_name, uint8_t *buffer, size_t length);
void send_data_to_server(String host, uint16_t port, uint8_t *buffer, size_t length);

bool init_board(void)
{
  Serial.begin(115200);

  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT);
  pinMode(HX711_PWR_EN_PIN, OUTPUT);
  // digitalWrite(BTN_PIN, HIGH);

  state = STATE_INITIALIZE;

  Serial.println("Initialize board [PASS]");
  return true;
}

bool init_sdcard(void)
{
  if (sdcard.init(SD_CS_PIN, true))
  {
    // sdcard.printCardDetails();
    // sdcard.printFileList();
    Serial.println("Initialize SD Card [PASS]");
    return true;
  }
  Serial.println("Initialize SD Card [FAIL]");
  return false;
}

bool init_hx711(void)
{
  digitalWrite(HX711_PWR_EN_PIN, HIGH);
  delay(1000);
  scale.begin(HX711_DOUT_PIN, HX711_SCK_PIN);

  static uint32_t t = millis();
  uint32_t start_time = millis();
  uint32_t tmp;
  while (millis() - start_time < HX711_INIT_TIMEOUT)
  {
    if (scale.is_ready())
    {
      tmp = millis() - t;
      // check 10 hz
      if (tmp >= 99 && tmp <= 101) {
        Serial.println("Initialize HX711 [PASS]");
        return true;
      }
      t = millis();
      scale.read();
    }
  }
  Serial.println("Initialize HX711 [FAIL]");
  return false;
}

bool init_nbiot(void)
{
  uint32_t start_time = millis();

  sim7020.init(9600, true);
  sim7020.powerOn();

  if (!sim7020.check())
  {
    Serial.println("Initialize NB-IOT Card [FAIL]");
    return false;
  }

  sim7020.setPhoneFunctionality(0);
  sim7020.setPacketDataProtocol("IPV4V6");
  sim7020.setPhoneFunctionality(1);
  sim7020.setAttachStatus(1);

  while(sim7020.getAttachStatus().indexOf("+CGATT: 1") < 0)
  {
    if (millis() - start_time >= SIM7020_INIT_TIMEOUT)
    {
      Serial.println("Initialize NB-IOT Card [FAIL]");
      return false;
    }
    sim7020.setAttachStatus(1);
    delay(1000);
  }

  Serial.println("Initialize NB-IOT Card [PASS]");
  return true;
}

void deinit_board(void)
{

}

void deinit_sdcard(void)
{
  sdcard.deinit();
}

void deinit_hx711(void)
{
  digitalWrite(HX711_PWR_EN_PIN, LOW);
}

void deinit_nbiot(void)
{
  sim7020.setPhoneFunctionality(0);
  sim7020.deinit();
}

bool self_test(void)
{

}

void sync_time(void) {
  static uint32_t last_time_sync = millis();
  if (last_time_sync == 0 || (millis() - last_time_sync > TIME_SYNC_TIMEOUT)) {
    last_time_sync = millis();
    init_nbiot();
    
    // pop
    // nbiot_get_time()

    deinit_nbiot();
  }
}

size_t collect_data(int32_t *buffer, size_t max_length, uint32_t timeout)
{
  size_t i=0;
  uint32_t start_time=millis();
  init_hx711();
  
  while((i < max_length) && (millis() - start_time < timeout))
  {
    if (scale.is_ready())
    {
      buffer[i] = scale.read();
      Serial.print(i);
      Serial.print(") ");
      Serial.println(buffer[i]);
      i++;

      // pop
      // break if no weight change

      delay(10);
    }
  }
  deinit_hx711();
  return i;
}

void save_data_to_sdcard(String file_name, uint8_t *buffer, size_t length)
{
  init_sdcard();
  sdcard.writeFile(file_name, buffer, length);
  deinit_sdcard();
}

void send_data_to_server(String host, uint16_t port, uint8_t *buffer, size_t length)
{
  init_nbiot();

  sim7020.createHTTPSocket(host, port);
  sim7020.connectHTTPSocket(0);

  String customer_header = "Accept: */*\r\nConnection: Keep-Alive\r\nUser-Agent: SIMCOM_MODULE\r\n";
  String content_type = "application/octet-stream";
  String data = sim7020._bufferToHexString(buffer, length);

  sim7020.sendHTTPData(0, 1, "/", customer_header, content_type, data);

  sim7020.disconnectHTTPSocket(0);
  sim7020.closeHTTPSocket(0);
  deinit_nbiot();
}


void setup() {
  size_t len;
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  init_board();

  switch (wakeup_reason)
  {
  // idle loop
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("sync time");
    sync_time();
    break;

  // idle user btn interupt
  // ESP_SLEEP_WAKEUP_EXT0 ?
  case ESP_SLEEP_WAKEUP_GPIO:
    Serial.println("user interupt");
    // check no charging

    len = collect_data(data_buf, MAX_DATA_LENGTH, 30000);
    Serial.print("len:");
    Serial.println(len);
    save_data_to_sdcard("test.bin", (uint8_t*) data_buf, len*4);
    send_data_to_server("http://13.229.87.104", 3000, (uint8_t*) data_buf, len*4);

    break;
  
  // turn-on, watchdog reset
  default:
    Serial.println("init");
    // Serial.println(wakeup_reason);
    // check maintenance button -> maintenance mode flag

    init_sdcard();
    init_hx711();
    init_nbiot();

    self_test();
    sync_time();

    deinit_sdcard();
    deinit_hx711();
    deinit_nbiot();

    
    // maintenance flag

    // just for test
    len = collect_data(data_buf, MAX_DATA_LENGTH, 10000);
    save_data_to_sdcard("test.bin", (uint8_t*) data_buf, len*4);
    send_data_to_server("http://13.229.87.104", 3000, (uint8_t*) data_buf, len*4);

    // break;
  }
  


  // uint8_t i;
  // for(i=0;i<10;i++) {
  //   digitalWrite(LED0_PIN,!digitalRead(LED0_PIN));
  //   digitalWrite(LED1_PIN,!digitalRead(LED1_PIN));
  //   delay(100);
  // }

  
  // if (digitalRead(BTN_PIN)==LOW) {
  //   Serial.println("entering maintenance mode");
  //   state = STATE_MAINTENANCE;
  // } else {
  //   Serial.println("entering idle mode");
  //   state = STATE_IDLE;
  // }

  Serial.println("sleep");
  // esp_sleep_enable_ext0_wakeup((gpio_num_t) BTN_PIN, 0); // 0 falling edge
  esp_sleep_enable_timer_wakeup(SLEEP_PERIOD_US);
  esp_deep_sleep_start();
  // state = STATE_NORMAL;
}

void loop() {
  Serial.println("ERROR");
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