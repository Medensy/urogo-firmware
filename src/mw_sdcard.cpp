#include <Arduino.h>
#include <dv_sdcard.h>

#include <board_config.h>

SDCARD sdcard;

bool sdcard_init(void)
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

void sdcard_deinit(void)
{
  sdcard.deinit();
}

void sdcard_save_data(String file_name, uint8_t *buffer, size_t length){
  sdcard.writeFile(file_name, buffer, length);
}

void sdcard_log(String file_name, String str) {
  sdcard.appendString(file_name, str);
}