#include <Arduino.h>
#include <mw_board.h>

#include <board_config.h>


bool board_init(void)
{
  Serial.begin(115200);

  pinMode(LED0_PIN, OUTPUT);
  pinMode(LED1_PIN, OUTPUT);
  pinMode(BTN_PIN, INPUT);
  
  // digitalWrite(BTN_PIN, HIGH);

  Serial.println("Initialize board [PASS]");
  return true;
}


void board_deinit(void)
{

}