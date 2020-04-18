#include <Arduino.h>
#include <mw_board.h>

#include <board_config.h>

hw_timer_t * timer = NULL;

void IRAM_ATTR toggle_red_led(){
  digitalWrite(RLED_PIN, !digitalRead(RLED_PIN));
}

void IRAM_ATTR toggle_green_led(){
  digitalWrite(GLED_PIN, !digitalRead(GLED_PIN));
}

bool board_init(void)
{
  Serial.begin(115200);

  pinMode(GLED_PIN, OUTPUT);
  pinMode(RLED_PIN, OUTPUT);
  pinMode(MODE_CONFIG_PIN, INPUT);
  pinMode(START_BTN_PIN, INPUT);
  
  timer = timerBegin(0, 80, true);

  // digitalWrite(BTN_PIN, HIGH);

  Serial.println("Initialize board [PASS]");
  return true;
}


void board_deinit(void)
{

}

void board_display_stop(void)
{
  timerAlarmDisable(timer);
  timerDetachInterrupt(timer);
  digitalWrite(RLED_PIN, LOW);
  digitalWrite(GLED_PIN, LOW);
}

void board_display_waiting(void)
{
  board_display_stop();
  timerAttachInterrupt(timer, &toggle_red_led, true);
  timerAlarmWrite(timer, 250000, true); // 250 ms
  timerAlarmEnable(timer);
}

void board_display_measuring(void)
{
  board_display_stop();
  digitalWrite(RLED_PIN, HIGH);
}

void board_display_processing(void)
{
  board_display_stop();
  timerAttachInterrupt(timer, &toggle_green_led, true);
  timerAlarmWrite(timer, 250000, true); // 250 ms
  timerAlarmEnable(timer);
}

void board_display_error(void)
{
  board_display_stop();
  timerAttachInterrupt(timer, &toggle_red_led, true);
  timerAlarmWrite(timer, 100000, true); // 250 ms
  timerAlarmEnable(timer);
}

