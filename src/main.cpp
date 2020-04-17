#include <Arduino.h>

#include <mw_board.h>
#include <mw_nbiot.h>
#include <mw_loadcell.h>
#include <mw_sdcard.h>

#include <board_config.h>

#define SLEEP_PERIOD_US 1*1*1000000 // 
#define TIME_SYNC_TIMEOUT 60*60*24 // time sync every 24 hours, expect 20ppm, error < 1.8s

uint32_t last_time_sync = 0;

#define MAX_DATA_LENGTH 1800 // sampling rate 10 SPS, record time 180 s
int32_t data_buf[MAX_DATA_LENGTH];
size_t data_idx = 0;

bool self_test(void);

void sync_time(void);

bool self_test(void)
{

}

void sync_time(void) {
  static uint32_t last_time_sync = millis();
  if (last_time_sync == 0 || (millis() - last_time_sync > TIME_SYNC_TIMEOUT)) {
    last_time_sync = millis();
    nbiot_init();
    
    // pop
    // nbiot_get_time()

    nbiot_deinit();
  }
}

void setup() {
  size_t len;
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  board_init();


  nbiot_sync_time("1.th.pool.ntp.org");

  while(true) {
    Serial.println("ok");
    delay(10000);
  }

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

    len = loadcell_collect_data(data_buf, MAX_DATA_LENGTH, 30000);
    Serial.print("len:");
    Serial.println(len);
    sdcard_save_data("test.bin", (uint8_t*) data_buf, len*4);
    nbiot_upload_data("http://13.229.87.104", 3000, (uint8_t*) data_buf, len*4);

    break;
  
  // turn-on, watchdog reset
  default:
    Serial.println("init");
    // Serial.println(wakeup_reason);
    // check maintenance button -> maintenance mode flag

    sdcard_init();
    loadcell_init();
    nbiot_init();

    self_test();
    sync_time();

    sdcard_deinit();
    loadcell_deinit();
    nbiot_deinit();

    
    // maintenance flag

    // just for test
    len = loadcell_collect_data(data_buf, MAX_DATA_LENGTH, 10000);
    sdcard_save_data("test.bin", (uint8_t*) data_buf, len*4);
    nbiot_upload_data("http://13.229.87.104", 3000, (uint8_t*) data_buf, len*4);

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
  // } else {
  //   Serial.println("entering idle mode");
  // }

  Serial.println("sleep");
  // esp_sleep_enable_ext0_wakeup((gpio_num_t) BTN_PIN, 0); // 0 falling edge
  esp_sleep_enable_timer_wakeup(SLEEP_PERIOD_US);
  esp_deep_sleep_start();
}

void loop() {
  while(true) {
    Serial.println("ERROR");
    delay(1000);
  }
}


