#include <Arduino.h>
#include <time.h>
#include <sys/time.h>

#include <dv_sim7020e.h>
#include <mw_nbiot.h>

#include <board_config.h>

SIM7020 sim7020;

// void nbiot_test(void) {
//   int socket_id = 0;
//   int method = 1; // post
//   String path_str = "/setBikeData";
//   unsigned int path_len = path_str.length();
//   String header_str = "4163636570743a202a2f2a0d0a436f6e6e656374696f6e3a204b6565702d416c6976650d0a557365722d4167656e743a2053494d434f4d5f4d4f44554c450d0a";
//   unsigned int header_len = header_str.length();
//   String content_type_str = "application/json";
//   unsigned int content_type_len = content_type_str.length();
//   String content_str = "7B22646576534E223A3836383333343033303030393730322C22646174614C697374223A205B5B302E3137303030302C3131332E3633323737352C33342E3734383832372C3131332E35302C302E3030303030302C31322C302E33382C312C313532353538333938335D2C5B302E3436303030302C3131332E3633323737382C33342E3734383832312C3131352E31302C302E3030303030302C31322C302E33382C312C313532353538333938355D2C5B302E3635303030302C3131332E3633323737392C33342E3734383831332C3131362E37302C302E3030303030302C31322C302E33362C312C313532353538333938375D2C5B302E3730303030302C3131332E3633323830332C33342E3734383830342C3131372E33302C302E3030303030302C31302C302E33362C312C313532353538333939315D2C5B302E3338303030302C3131332E3633323830322C33342E3734383830342C3131372E39302C302E3030303030302C31302C302E33382C312C313532353538333939335D5D7D";
//   unsigned int content_len = content_str.length();

//   String combinded_data = String(socket_id) + "," + method +"," +
//     String(path_len) + ",\"" + path_str + "\"," +
//     header_len + "," + header_str + "," +
//     content_type_len + ",\"" + content_type_str + "\"," +
//     content_len + "," + content_str;

//   Serial.print("length: ");
//   Serial.println(combinded_data.length());
//   Serial.print("data: ");
//   Serial.println(combinded_data);
// }

bool nbiot_init(void)
{
  // nbiot_test();

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

  while(sim7020.getAttachStatus().indexOf("+CGATT: 1") < 0)
  {
    if (millis() - start_time >= SIM7020_INIT_TIMEOUT)
    {
      Serial.println("Initialize NB-IOT Card [FAIL]");
      return false;
    }
    sim7020.setAttachStatus(1);
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

bool nbiot_upload_data(String host, uint16_t port, String path, String serial_number, String secret_key, long timestamp, uint8_t *buffer, size_t length)
{
  int i;
  int last_index = 0;
  int find_index = 0;
  size_t total_len = 0;
  const int bytes_per_packet = 400;
  bool success_flag = false;

  int socket_id = 0;
  int method = 1; // post
  String path_str = path;
  unsigned int path_len = path_str.length();
  String header = "";
  header += "Accept: */*\r\n";
  header += "Connection: Keep-Alive\r\n";
  header += "User-Agent: SIMCOM_MODULE\r\n";
  header += "Device-Serial: " + String(serial_number) + "\r\n";
  header += "Device-Secret: " + String(secret_key) + "\r\n";
  header += "Time: " + String(timestamp) + "\r\n";

  Serial.println("header:");
  Serial.println(header);
  String header_str = sim7020._stringToHexString(header);
  
  unsigned int header_len = header_str.length();
  String content_type_str = "application/octet-stream";
  unsigned int content_type_len = content_type_str.length();
  String content_str = sim7020._bufferToHexString(buffer, length);
  unsigned int content_len = content_str.length();

  String packet_str = "";

  String combinded_data = String(socket_id) + "," + method +"," +
    String(path_len) + ",\"" + path_str + "\"," +
    header_len + "," + header_str + "," +
    content_type_len + ",\"" + content_type_str + "\"," +
    content_len + "," + content_str;

  total_len = combinded_data.length();

  sim7020.createHTTPSocket(host, port);
  sim7020.connectHTTPSocket(0);

  // find last "," before content
  for (i=0;i<8;i++)
  {
    find_index = combinded_data.indexOf(',', find_index) + 1;
  }

  // send first packet which excludes contents
  packet_str = combinded_data.substring(0, find_index);
  sim7020.sendLongHTTPData(1, total_len, packet_str.length(), packet_str);
  last_index += packet_str.length();

  // send later packet which consists of fragment of contents
  while (last_index + bytes_per_packet < total_len)
  {
    packet_str = combinded_data.substring(last_index, last_index+bytes_per_packet);
    sim7020.sendLongHTTPData(1, total_len, packet_str.length(), packet_str);
    last_index += packet_str.length();
  }

  // send last packet
  packet_str = combinded_data.substring(last_index, total_len);
  sim7020.sendLongHTTPData(0, total_len, packet_str.length(), packet_str);

  // check if http status is 200
  // +CHTTPNMIH: <httpclient_id>,<response_code>,<header_length>,<header>
  success_flag = sim7020.getOutputString().indexOf("+CHTTPNMIH: 0,200");

  sim7020.disconnectHTTPSocket(0);
  sim7020.closeHTTPSocket(0);

  return success_flag;
}

bool nbiot_sync_time(String host)
{
  int start_index;
  String output_str, time_str;
  struct tm now_tm;
  time_t now_time;
  struct timeval tv;
  bool success_flag = false;

  for (int i=0;i<5;i++)
  {
    sim7020.startQueryNetwork(host);
    output_str = sim7020.getOutputString();
    start_index = output_str.indexOf("+CSNTP:");

    if(start_index >= 0)
    {
      time_str = output_str.substring(start_index+8,start_index+25);

      now_tm.tm_year = time_str.substring(0,2).toInt() + 2000 - 1900;
      now_tm.tm_mon = time_str.substring(3,5).toInt() - 1;
      now_tm.tm_mday = time_str.substring(6,8).toInt();
      now_tm.tm_hour = time_str.substring(9,11).toInt();
      now_tm.tm_min = time_str.substring(12,14).toInt();
      now_tm.tm_sec = time_str.substring(15,17).toInt();
      now_tm.tm_isdst = -1;
      now_time = mktime(&now_tm);
      tv.tv_sec = now_time;
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
      
      success_flag = true;
      break;
    }
    else
    {
      delay(1000);
    }
  }
  sim7020.stopQueryNetwork();

  return success_flag;
}