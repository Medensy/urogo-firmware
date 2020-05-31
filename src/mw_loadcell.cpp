#include <Arduino.h>
#include <dv_hx711.h>
// esp32 rtc pin
#include <driver/rtc_io.h>

#include <board_config.h>

HX711 scale;
float loadcell_m;
int32_t loadcell_a;

// y = (x-a)*m
// y = mx - ma -> c = -ma

typedef enum {
  LOADCELL_NOT_READY,
  LOADCELL_MEASURING,
  LOADCELL_DONE,
} loadcell_state_t;

bool loadcell_init(void)
{
  rtc_gpio_hold_dis((gpio_num_t) HX711_SCK_PIN);
  rtc_gpio_hold_dis((gpio_num_t) HX711_PWR_EN_PIN);
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
      if (tmp >= 98 && tmp <= 102) {
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
  rtc_gpio_hold_en((gpio_num_t) HX711_SCK_PIN);
  rtc_gpio_hold_en((gpio_num_t) HX711_PWR_EN_PIN);
}

size_t loadcell_collect_data(int16_t *buffer, size_t max_length, uint32_t timeout, bool early_stop_enable)
{
  size_t i=0;
  uint32_t start_time=millis();
  float tmp;

  loadcell_state_t state = LOADCELL_NOT_READY;
  
  while((i < max_length) && (millis() - start_time < timeout))
  {
    if (scale.is_ready())
    {
      tmp = (scale.read()-loadcell_a)*loadcell_m;
      // clipping
      tmp = tmp <= 32767 ? tmp : 32767;
      tmp = tmp >= -32768 ? tmp :- 32768;
      // convert float to int
      buffer[i] = int(tmp);
      // first threshold 100 g
      if ((state == LOADCELL_NOT_READY) && (tmp >= 1000))
      {
        state = LOADCELL_MEASURING;
      }
      else if ((state == LOADCELL_MEASURING) && (tmp < 0))
      {
        state = LOADCELL_DONE;
      }
      Serial.print(i);
      Serial.print(") ");
      Serial.println(buffer[i]);
      i++;

      if ((state == LOADCELL_DONE) && early_stop_enable)
      {
        Serial.println("early stop");
        break;
      }
      
      delay(10);
    }
  }
  return i;
}

void loadcell_get_config(float *m, int32_t *a){
  *a = loadcell_a;
  *m = loadcell_m;
}

void loadcell_set_config(float m, int32_t a)
{
  loadcell_a = a;
  loadcell_m = m;
}

void loadcell_set_zero(void)
{
  int32_t val;
  
  while (!scale.is_ready()) {};
  
  // y = (x-a)*m
  // (y=0, m=1) -> 0 = x-a
  // a = x
  val = scale.read_average(10);
  loadcell_a = val;
}

void loadcell_set_weight(int32_t weight)
{
  int32_t val;
  
  while (!scale.is_ready()) {};
  
  val = scale.read_average(10);
  // y = (x-a)*m
  // m = y/(x-a)
  loadcell_m = weight/float(val-loadcell_a);
}