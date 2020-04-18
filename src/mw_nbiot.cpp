#include <Arduino.h>
#include <dv_sim7020e.h>
#include <mw_nbiot.h>

#include <board_config.h>

SIM7020 sim7020;

bool nbiot_init(void)
{
  uint32_t start_time = millis();

  sim7020.init(9600, true);
  sim7020.powerOn();

  if (!sim7020.check())
  {
    Serial.println("Initialize NB-IOT Card [FAIL]");
    return false;
  }

  sim7020.setPhoneFunctionality(0);
  sim7020.setPacketDataProtocol("IPV4V6");
  sim7020.setPhoneFunctionality(1);
  sim7020.setAttachStatus(1);
  Serial.print(">>>>");
  Serial.println(sim7020.getReturnString());

  while(sim7020.getAttachStatus().indexOf("+CGATT: 1") < 0)
  {
    if (millis() - start_time >= SIM7020_INIT_TIMEOUT)
    {
      Serial.println("Initialize NB-IOT Card [FAIL]");
      return false;
    }
    sim7020.setAttachStatus(1);
    Serial.print(">>>>");
    Serial.println(sim7020.getReturnString());
    delay(1000);
  }

  Serial.println("Initialize NB-IOT Card [PASS]");
  return true;
}

void nbiot_deinit(void)
{
  sim7020.setPhoneFunctionality(0);
  sim7020.deinit();
}

void nbiot_upload_data(String host, uint16_t port, uint8_t *buffer, size_t length)
{
  nbiot_init();

  sim7020.createHTTPSocket(host, port);
  sim7020.connectHTTPSocket(0);

  String customer_header = "Accept: */*\r\nConnection: Keep-Alive\r\nUser-Agent: SIMCOM_MODULE\r\n";
  String content_type = "application/octet-stream";
  String data = sim7020._bufferToHexString(buffer, length);

  sim7020.sendHTTPData(0, 1, "/", customer_header, content_type, data);

  sim7020.disconnectHTTPSocket(0);
  sim7020.closeHTTPSocket(0);
  nbiot_deinit();
}

void nbiot_sync_time(String host)
{
  nbiot_init();
  for (int i=0;i<5;i++)
  {
    sim7020.startQueryNetwork(host);
    Serial.print("1) ");
    Serial.println(sim7020.getOutputString());
    Serial.print("2) ");
    Serial.println(sim7020.getOutputString().indexOf("+CSNTP:"));
    Serial.print("3) ");
    Serial.println(sim7020.getReturnString());
    Serial.println();
    delay(1000);
  }
  
  sim7020.stopQueryNetwork();

  nbiot_deinit();
}
// void sim7020_test() {
//   sim7020.init(9600, true);
//   sim7020.powerOn();
//   if (sim7020.check()) {
//     Serial.println("sim7020 ok");
//   }
//   sim7020.getModuleDetails();
//   sim7020.getSignalRSSI();
//   sim7020.getPacketDataProtocol();
//   sim7020.getRegistrationStatus();
//   sim7020.getAttachStatus();

//   sim7020.setPhoneFunctionality(0);
//   sim7020.setPacketDataProtocol("IPV4V6");
//   sim7020.setPhoneFunctionality(1);
//   sim7020.setAttachStatus(1);

//   while(sim7020.getAttachStatus().indexOf("+CGATT: 1")<0) {
//     sim7020.setAttachStatus(1);
//     delay(1000);
//   }
//   Serial.println("attached to network");
//   // sim7020.getSignalRSSI();
//   // sim7020.getPacketDataProtocol();
//   // sim7020.getRegistrationStatus();
//   // sim7020.getAPN();
//   // sim7020.getIPFromDNS("www.google.com");
//   sim7020.ping("172.217.160.68");
//   // sim7020.ping("www.google.com");
  
//   // sim7020.createHTTPSocket("http://postman-echo.com",80);
//   // sim7020.createHTTPSocket("http://52.22.211.29",80);
//   sim7020.createHTTPSocket("http://13.229.87.104",3000);
//   sim7020.listHTTPSockets();
//   sim7020.connectHTTPSocket(0);
//   String customer_header = "Accept: */*\r\nConnection: Keep-Alive\r\nUser-Agent: SIMCOM_MODULE\r\n";
//   String encoded_customer_header = "4163636570743a202a2f2a0d0a436f6e6e656374696f6e3a204b6565702d416c6976650d0a557365722d4167656e743a2053494d434f4d5f4d4f44554c450d0a";
//   assert(sim7020._stringToHexString(customer_header)==encoded_customer_header);
//   uint8_t buf[512];
//   customer_header.getBytes(buf,sizeof(buf),0);
//   assert(sim7020._bufferToHexString(buf,customer_header.length())==encoded_customer_header);
  
//   // get
//   // sim7020.sendHTTPData(0,0,"/get?x=123",customer_header,"application/json","{}");
  
//   // String customer_header = "Accept: */*\r\nConnection: Keep-Alive\r\nUser-Agent: SIMCOM_MODULE\r\n";
//   String content_type = "application/octet-stream";
//   String data = "hello ok\r\n";
//   // post
//   sim7020.sendHTTPData(0, 1, "/", customer_header, content_type, data);

//   // sim7020.listHTTPSockets();
//   sim7020.disconnectHTTPSocket(0);
//   sim7020.closeHTTPSocket(0);
//   sim7020.listHTTPSockets();
// }