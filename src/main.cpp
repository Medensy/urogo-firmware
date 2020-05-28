#include <Arduino.h>
#include <time.h>
#include <sys/time.h>

#include <mw_board.h>
#include <mw_nbiot.h>
#include <mw_loadcell.h>
#include <mw_sdcard.h>

#include <board_config.h>

int32_t data_buf[MAX_DATA_LENGTH];
size_t data_idx = 0;

board_stored_data_t stored_data;

// curl -X POST --data-binary @02.jpg --header "Content-Type: application/octet-stream" 13.250.110.28/1234/02-05-20
// curl -X POST --data-binary @debug.txt --header "Content-Type: application/octet-stream" 13.250.110.28/1234/02-05-20

void setup() {
  bool error_flag = false;
  esp_sleep_wakeup_cause_t wakeup_reason;

  int i;
  String cmd_str;
  String param_str;
  int64_t loadcell_sum;

  size_t len;
  struct timeval tv;
  time_t now_time;
  struct tm *now_tm;
  char tm_buf[64];

  String path_str;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  board_init();
  stored_data = board_get_data();

  switch (wakeup_reason)
  {
  // timer interupt
  case ESP_SLEEP_WAKEUP_TIMER:
    Serial.println("sync time");
    board_display_waiting();

    if (!error_flag)
    {
      // sync time and get back to sleep
      Serial.println("sync time from sntp server");
      board_display_processing();
      nbiot_sync_time(SNTP_SERVER);
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
        nbiot_sync_time(SNTP_SERVER);
        
        gettimeofday(&tv, NULL);
        now_time = tv.tv_sec;
        now_tm = localtime(&now_time);
        strftime(tm_buf, sizeof tm_buf, "%Y-%m-%d-%H-%M-%S", now_tm);

        Serial.print("measuring ");
        Serial.println(tm_buf);
        board_display_measuring();
        // set load cell scaling
        loadcell_set_config(stored_data.loadcell_m, stored_data.loadcell_a);
        // pop, back to 180 s -> 180000
        len = loadcell_collect_data(data_buf, MAX_DATA_LENGTH, 180000);

        board_display_processing();
        sdcard_save_data(String(tm_buf)+".bin", (uint8_t*) data_buf, len*4);
        
        path_str = "/api/nb";
        
        nbiot_upload_data(UPLOAD_SERVER, UPLOAD_PORT, path_str, stored_data.serial_number, stored_data.secret_key, now_time, (uint8_t*) data_buf, len*4);

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
    if (digitalRead(MODE_CONFIG_PIN)==LOW)
    {
      Serial.println("enter maintenance mode");
      board_display_waiting();
      while(true)
      {
        if (Serial.available())
        {
          cmd_str = Serial.readStringUntil('\n');
          Serial.print("cmd: ");
          Serial.println(cmd_str);
          if (cmd_str.indexOf("DEINIT") >= 0)
          {
            Serial.println("Deinitialize");
            sdcard_deinit();
            loadcell_deinit();
            nbiot_deinit();
          }
          else if (cmd_str.indexOf("INIT") >= 0)
          {
            Serial.println("Initialize");

            Serial.print("SD CARD ");
            if(sdcard_init()) Serial.println("[PASS]");
            else Serial.println("[FAIL]");

            Serial.print("LOADCELL ");
            if(loadcell_init()) Serial.println("[PASS]");
            else Serial.println("[FAIL]");

            Serial.print("NB-IOT ");
            if(nbiot_init()) Serial.println("[PASS]");
            else Serial.println("[FAIL]");
            
          }
          else if (cmd_str.indexOf("EXIT") >= 0)
          {
            Serial.println("Exit");
            break;
          }
          else if (cmd_str.indexOf("GET_SN") >= 0)
          {
            Serial.println("Get serial number");
            Serial.println(stored_data.serial_number);
          }
          else if (cmd_str.indexOf("SET_SN") >= 0)
          {
            Serial.println("Set serial number");
            param_str = cmd_str.substring(7, cmd_str.length());
            param_str.toCharArray(stored_data.serial_number, param_str.length());
            Serial.println(stored_data.serial_number);
          }
          else if (cmd_str.indexOf("GET_SK") >= 0)
          {
            Serial.println("Get secret key");
            Serial.println(stored_data.secret_key);
          }
          else if (cmd_str.indexOf("SET_SK") >= 0)
          {
            Serial.println("Set secret key");
            param_str = cmd_str.substring(7, cmd_str.length());
            param_str.toCharArray(stored_data.secret_key, param_str.length());
            Serial.println(stored_data.secret_key);
          }
          else if (cmd_str.indexOf("CAL 0") >= 0)
          {
            Serial.println("Calibrate weight 0");
            loadcell_set_zero();
            loadcell_get_config(&stored_data.loadcell_m, &stored_data.loadcell_a);
            Serial.println("Done");
          }
          else if (cmd_str.indexOf("CAL") >= 0)
          {
            Serial.print("Calibrate weight ");
            param_str = cmd_str.substring(4, cmd_str.length());
            Serial.println(param_str.toInt());
            loadcell_set_weight(param_str.toInt());
            loadcell_get_config(&stored_data.loadcell_m, &stored_data.loadcell_a);
            Serial.println("Done");
          }
          else if (cmd_str.indexOf("WEIGHT") >= 0)
          {
            Serial.println("Get weight");
            loadcell_set_config(stored_data.loadcell_m, stored_data.loadcell_a);
            loadcell_collect_data(data_buf, 10, 3000);
            loadcell_sum = 0;
            for(i=0;i<10;i++)
            {
              loadcell_sum += data_buf[i];
            }
            Serial.println(long(loadcell_sum/10));
          }
          else if (cmd_str.indexOf("SAVE") >= 0)
          {
            Serial.println("Save");
            board_save_data(stored_data);
          }
          else if (cmd_str.indexOf("PRINT") >= 0)
          {
            Serial.println("Print");
            Serial.print("Serial number: ");
            Serial.println(stored_data.serial_number);
            Serial.print("Secret key: ");
            Serial.println(stored_data.secret_key);
            Serial.print("Load cell m: ");
            Serial.println(stored_data.loadcell_m);
            Serial.print("Load cell a: ");
            Serial.println(stored_data.loadcell_a);
            
          }
        }
        
      }
    }
    // normal mode
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
        nbiot_sync_time(SNTP_SERVER);
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
