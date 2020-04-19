#include <Arduino.h>
#include <dv_hx711.h>

#include <board_config.h>

HX711 scale;

bool loadcell_init(void)
{
  pinMode(HX711_PWR_EN_PIN, OUTPUT);
  digitalWrite(HX711_PWR_EN_PIN, HIGH);
  scale.power_up();
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

void loadcell_deinit(void)
{
  scale.power_down();
  digitalWrite(HX711_PWR_EN_PIN, LOW);
}

size_t loadcell_collect_data(int32_t *buffer, size_t max_length, uint32_t timeout)
{
  size_t i=0;
  uint32_t start_time=millis();
  
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
  return i;
}