#include <Arduino.h>
#include <time.h>
#include <sys/time.h>

#include <mw_board.h>
#include <mw_nbiot.h>
#include <mw_loadcell.h>
#include <mw_sdcard.h>

#include <board_config.h>

#define SLEEP_PERIOD_US 1*1*1000000 // 

uint32_t last_time_sync = 0;

#define MAX_DATA_LENGTH 1800 // sampling rate 10 SPS, record time 180 s
int32_t data_buf[MAX_DATA_LENGTH];
size_t data_idx = 0;


void setup() {
  bool error_flag = false;
  esp_sleep_wakeup_cause_t wakeup_reason;

  size_t len;
  struct timeval tv;
  time_t now_time;
  struct tm *now_tm;
  char tm_buf[64];

  wakeup_reason = esp_sleep_get_wakeup_cause();

  board_init();

  switch (wakeup_reason)
  {
  // idle loop
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("sync time");
    board_display_waiting();

    if (!error_flag)
    {
      // sync time and get back to sleep
      Serial.println("sync time from sntp server");
      board_display_processing();
      nbiot_sync_time("1.th.pool.ntp.org");
      board_display_stop();
    }
    else
    {
      // display error and get back to sleep
      board_display_error();
      delay(5000);
      board_display_stop();
    }
    nbiot_deinit();
    
    break;

  // idle user btn interupt
  case ESP_SLEEP_WAKEUP_EXT0:
    Serial.println("user interupt");
    board_display_waiting();

    // if not charging, run normal operation
    if (!board_is_charging())
    {

      error_flag |= !sdcard_init();
      error_flag |= !loadcell_init();
      error_flag |= !nbiot_init();

      if (!error_flag)
      {
        gettimeofday(&tv, NULL);
        now_time = tv.tv_sec;
        now_tm = localtime(&now_time);
        strftime(tm_buf, sizeof tm_buf, "%Y-%m-%d-%H-%M-%S", now_tm);

        Serial.print("measuring ");
        Serial.println(tm_buf);
        board_display_measuring();
        len = loadcell_collect_data(data_buf, MAX_DATA_LENGTH, 30000);

        board_display_processing();
        sdcard_save_data(String(tm_buf)+".bin", (uint8_t*) data_buf, len*4);
        nbiot_upload_data("http://13.229.87.104", 3000, (uint8_t*) data_buf, len*4);

        board_display_stop();
      }
      else
      {
        // display error and get back to sleep
        board_display_error();
        delay(5000);
        board_display_stop();
      }
      
      nbiot_deinit();
      loadcell_deinit();
      sdcard_deinit();
    }
    // if charging, stop
    else
    {
      // display error and get back to sleep
      Serial.println("board is charging");
      board_display_error();
      delay(2000);
      board_display_stop();
    }

    break;
  
  // turn-on, watchdog reset
  default:
    Serial.println("init");
    // Serial.println(wakeup_reason);
    // check maintenance button -> maintenance mode flag

    // if (digitalRead(MODE_CONFIG_PIN)==LOW)
    // bypass
    if (false)
    {
      Serial.println("enter maintenance mode");
      
    }
    else
    {
      Serial.println("enter normal mode");
      board_display_waiting();
      error_flag |= !sdcard_init();
      error_flag |= !loadcell_init();
      error_flag |= !nbiot_init();

      if (!error_flag)
      {
        Serial.println("sync time from sntp server");
        board_display_processing();
        nbiot_sync_time("1.th.pool.ntp.org");
        board_display_stop();
      }
      else
      {
        board_display_error();
      }
      
      nbiot_deinit();
      loadcell_deinit();
      sdcard_deinit();
    }
  }

  if (error_flag)
  {
    board_stop_running();
  }

  Serial.println("sleep");
  esp_sleep_enable_ext0_wakeup((gpio_num_t) START_BTN_PIN, 0); // 0 falling edge
  // esp_sleep_enable_timer_wakeup(SLEEP_PERIOD_US);
  esp_deep_sleep_start();
}

void loop() {
  board_display_fatal();
  board_stop_running();
}


