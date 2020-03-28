#include <SPI.h>
#include <SD.h>
#include <FS.h>

#include <sdcard.h>

bool SDCARD::init(int ss_pin, bool debug)
{
  _ss_pin = ss_pin;
  _debug = debug;
  if(!SD.begin(ss_pin)){
    return false;
  }

  uint8_t cardType = SD.cardType();
  if(cardType == CARD_NONE){
    return false;
  }

  return true;
}

uint8_t SDCARD::getCardType()
{
  return SD.cardType();
}

uint64_t SDCARD::getUsedBytes()
{
  return SD.usedBytes();
}

uint64_t SDCARD::getTotalBytes()
{
  return SD.totalBytes();
}

void SDCARD::printCardDetails()
{
  sdcard_type_t cardType = SD.cardType();
  Serial.print("SD Card Type: ");
  if(cardType == CARD_MMC){
      Serial.println("MMC");
  } else if(cardType == CARD_SD){
      Serial.println("SDSC");
  } else if(cardType == CARD_SDHC){
      Serial.println("SDHC");
  } else {
      Serial.println("UNKNOWN");
  }

  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
}

void SDCARD::printFileList()
{
  File root = SD.open("/");
  File file = root.openNextFile();
  if(!root){
    Serial.println("failed to open directory");
    return;
  }
  while(file){
    if(!file.isDirectory()){
      Serial.print("file: ");
      Serial.print(file.name());
      Serial.print("size: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

size_t SDCARD::writeFile(String file_name, uint8_t buf[], size_t len)
{
  if (_debug) Serial.println("writing file: /"+file_name);
  File file = SD.open("/"+file_name, FILE_WRITE);
  size_t size = file.write(buf, len);
  file.close();
  if (_debug)
  {
    if (size)
    {
      Serial.println("file written");
    }
    else
    {
      Serial.println("fail to write file");
    }
  }
  return size;
}
