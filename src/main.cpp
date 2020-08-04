#include <Arduino.h>
#include <time.h>
#include <sys/time.h>

#include <mw_board.h>
#include <mw_nbiot.h>
#include <mw_loadcell.h>
#include <mw_sdcard.h>

#include <board_config.h>

/* initialize flag */
bool error_flag;

/* board information and configuration */
board_stored_data_t stored_data;

/* measuring data */
int16_t data_buf[MAX_DATA_LENGTH];
size_t data_idx = 0;

void enter_power_up_mode(void);
void enter_maintenance_mode(void);
void enter_time_sync_mode(void);
void enter_user_main_mode(void);

void enter_power_up_mode(void)
{
  int i;
  bool sync_status;
  bool sdcard_init_flag;
  bool loadcell_init_flag;
  bool nbiot_init_flag;

  struct timeval tv;
  time_t now_time;
  struct tm *now_tm;
  char tm_buf[64];

  String init_log = "";
  String tm_prefix = "";

  Serial.println("enter normal mode");
  board_display_waiting();
  sdcard_init_flag = sdcard_init();
  loadcell_init_flag = loadcell_init();
  nbiot_init_flag = nbiot_init();

  /* read time from rtc */
  gettimeofday(&tv, NULL);
  now_time = tv.tv_sec;
  now_tm = localtime(&now_time);
  strftime(tm_buf, sizeof(tm_buf), "%Y-%m-%d-%H-%M-%S", now_tm);

  /* prepare log message */
  tm_prefix = "[" + String(tm_buf) + "] ";
  init_log += tm_prefix;
  if (sdcard_init_flag) init_log += "INIT SD CARD [PASS]";
  else init_log += "INIT SD CARD [FAIL]";
  init_log += "\n";

  init_log += tm_prefix;
  if (loadcell_init_flag) init_log += "INIT LOAD CELL [PASS]";
  else init_log += "INIT LOAD CELL [FAIL]";
  init_log += "\n";
  
  init_log += tm_prefix;
  if (nbiot_init_flag) init_log += "INIT NB IOT [PASS]";
  else init_log += "INIT NB IOT [FAIL]";
  init_log += "\n";

  /* log initialization to sd card */      
  sdcard_log("initialize.log", init_log);

  error_flag |= !sdcard_init_flag;
  error_flag |= !loadcell_init_flag;
  error_flag |= !nbiot_init_flag;

  if (!error_flag)
  {
    Serial.println("sync time from sntp server");
    board_display_processing();
    /* time sync with server, retrying 3 times */
    for (i=0; i<3; i++)
    {
      sync_status = nbiot_sync_time(SNTP_SERVER);
      if (sync_status) break;
    }
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

void enter_maintenance_mode(void)
{
  String cmd_str;
  String param_str;

  int i;
  int32_t loadcell_sum;

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
        loadcell_collect_data(data_buf, 10, 3000, false);
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

void enter_time_sync_mode(void)
{
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
}

void enter_user_main_mode(void)
{
  size_t len;
  
  bool sdcard_init_flag;
  bool loadcell_init_flag;

  struct timeval tv;
  time_t now_time;
  struct tm *now_tm;
  char tm_buf[64];

  String init_log = "";
  String tm_prefix = "";

  int i;
  bool upload_status;

  Serial.println("user interupt");
  board_display_waiting();

  // if not charging, run normal operation
  if (!board_is_charging())
  {
    sdcard_init_flag = sdcard_init();
    loadcell_init_flag = loadcell_init();

    /* read time from rtc */
    gettimeofday(&tv, NULL);
    now_time = tv.tv_sec;
    now_tm = localtime(&now_time);
    strftime(tm_buf, sizeof(tm_buf), "%Y-%m-%d-%H-%M-%S", now_tm);

    /* prepare log message */
    tm_prefix = "[" + String(tm_buf) + "] ";
    init_log += tm_prefix;
    if (sdcard_init_flag) init_log += "INIT SD CARD [PASS]";
    else init_log += "INIT SD CARD [FAIL]";
    init_log += "\n";

    init_log += tm_prefix;
    if (loadcell_init_flag) init_log += "INIT LOAD CELL [PASS]";
    else init_log += "INIT LOAD CELL [FAIL]";
    init_log += "\n";

    /* log initialization to sd card */      
    sdcard_log("initialize.log", init_log);

    error_flag |= !sdcard_init_flag;
    error_flag |= !loadcell_init_flag;
    
    if (!error_flag)
    {
      /* start measuring */
      board_display_measuring();
      /* set load cell scaling */
      loadcell_set_config(stored_data.loadcell_m, stored_data.loadcell_a);
      // pop, back to 180 s -> 180000
      len = loadcell_collect_data(data_buf, MAX_DATA_LENGTH, 180000, true);

      board_display_processing();

      /* read time from rtc */
      gettimeofday(&tv, NULL);
      now_time = tv.tv_sec;
      now_tm = localtime(&now_time);
      strftime(tm_buf, sizeof(tm_buf), "%Y-%m-%d-%H-%M-%S", now_tm);

      Serial.println(tm_buf);

      /* save data to sd card */      
      sdcard_save_data(String(tm_buf)+".bin", (uint8_t*) data_buf, len*2);

      /* init nbiot */
      if (nbiot_init())
      {
        /* upload file, retrying 3 times*/
        for (i=0; i<3 ; i++)
        {
          upload_status = nbiot_upload_data(UPLOAD_SERVER, UPLOAD_PORT, UPLOAD_PATH, stored_data.serial_number, stored_data.secret_key, now_time, (uint8_t*) data_buf, len*2);
          Serial.print("upload status: ");
          Serial.println(upload_status);
          if (upload_status) break;
        }
        nbiot_deinit();
      }
      
      board_display_stop();
    }
    else
    {
      // display error and get back to sleep
      board_display_error();
      delay(5000);
      board_display_stop();
    }
    
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
}

void setup() {
  error_flag = false;

  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();

  board_init();
  stored_data = board_get_data();

  switch (wakeup_reason)
  {
  // timer interupt
  case ESP_SLEEP_WAKEUP_TIMER:
    enter_time_sync_mode();
    break;

  // idle user btn interupt
  case ESP_SLEEP_WAKEUP_EXT0:
    enter_user_main_mode();
    break;
  
  // turn-on, watchdog reset
  default:
    if (digitalRead(MODE_CONFIG_PIN)==LOW) enter_maintenance_mode();
    else enter_power_up_mode();
  }

  if (error_flag)
  {
    board_stop_running();
  }

  Serial.println("sleep");

  board_deinit();
  esp_sleep_enable_ext0_wakeup((gpio_num_t) START_BTN_PIN, 0); // 0 falling edge
  esp_sleep_enable_timer_wakeup(SLEEP_PERIOD_US);
  esp_deep_sleep_start();
}

void loop() {
  board_display_fatal();
  board_stop_running();
}
