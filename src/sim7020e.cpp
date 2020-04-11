#include <Arduino.h>
#include <sim7020e.h>

#define isHwReset 1
// #define hwResetPin 26
#define isATReset 1
#define isNetLight 0

#define serialConfig 1
#define configParam SERIAL_8N1

#define DEFAULT_TX_PIN 17
#define DEFAULT_RX_PIN 16
#define DEFAULT_RESET_PIN 26

#define SIM7020_BAUDRATE 9600

void SIM7020::init(int baudrate, bool debug) {
  this->init(baudrate, DEFAULT_TX_PIN, DEFAULT_RX_PIN, DEFAULT_RESET_PIN, debug);
}

void SIM7020::init(int baudrate, int tx_pin, int rx_pin, int reset_pin, bool debug){
  this->debug = debug;
  if (debug) {
    Serial.println("SIM7020 initializing");
  }
  this->reset_pin = reset_pin;
  pinMode(this->reset_pin, OUTPUT);
  this->powerOff();
  this->serial_ptr = new HardwareSerial(1);
  this->serial_ptr->begin(baudrate, SERIAL_8N1, rx_pin, tx_pin);
  this->serial_ptr->flush();  
}

void SIM7020::deinit(void) {
  serial_ptr->end();
  powerOff();
}

void SIM7020::powerOn(void) {
  digitalWrite(reset_pin, HIGH);
  delay(200);
  digitalWrite(reset_pin, LOW);
  delay(800);
  digitalWrite(reset_pin, HIGH);
  delay(200);
  setEchoMode(0);
}

void SIM7020::powerOff(void) {
  digitalWrite(reset_pin, LOW);
}

bool SIM7020::readOk(void) {
  String output = serial_ptr->readStringUntil('\n');
  if (debug) {
    Serial.println(output);
  }
  return output.indexOf("OK")>=0;
}

bool SIM7020::sendCmd(String cmd, String ok_str, String err_str) {
  if (debug) {
    Serial.println(cmd);
  }
  serial_ptr->println(cmd);

  String tmp;
  String output;
  // _output_str.clear();
  uint32_t start_time = millis();
  bool timeout_flag = false;
  do {
    while ((!serial_ptr->available()) && (!timeout_flag)) {
      if (millis() - start_time >= 3000){
        timeout_flag = true;
        break;
      }
    }
    if (timeout_flag) break;
    tmp = serial_ptr->readStringUntil('\n');
    // Serial.println(tmp);
    if (tmp.indexOf(ok_str) >= 0) {
      _output_str = output;
      _return_str = tmp;
      if (debug) {
        Serial.println(_output_str);
        Serial.println(_return_str);
      }
      return true;
    } else if (tmp.indexOf(err_str) >= 0) {
      _output_str = output;
      _return_str = tmp;
      if (debug) {
        Serial.println(_output_str);
        Serial.println(_return_str);
      }
      return false;
    } else {
      output += tmp;
    }
  } while(true);

  Serial.println("timeout");
  return false;
}

bool SIM7020::sendCmd(String cmd, String ok_str) {
  return sendCmd(cmd, ok_str, "ERROR");
}

bool SIM7020::sendCmd(String cmd) {
  return sendCmd(cmd, "OK", "ERROR");
}

String SIM7020::getOutputString(void) {
  return _output_str;
}

String SIM7020::getReturnString(void) {
  return _return_str;
}

// 0-echo off, 1-echo on
int SIM7020::setEchoMode(int mode) {
  String cmd = "ATE" + String(mode);
  sendCmd(cmd);
  return 0;
}

// AT
int SIM7020::check() {
  String cmd = "AT";
  return sendCmd(cmd);
}

// AT+GSV
String SIM7020::getModuleDetails() {
  String cmd = "AT+GSV";
  sendCmd(cmd);
  String output = getOutputString();
  return output;
}

// AT+CSQ
int SIM7020::getSignalRSSI() {
  String cmd = "AT+CSQ";
  Serial.println(">> TMP <<");
  sendCmd(cmd);
  String output = getOutputString();
  return 0;
}

// AT*MCGDEFCONT?
String SIM7020::getPacketDataProtocol() {
  String cmd = "AT*MCGDEFCONT?";
  sendCmd(cmd);
  String output = getOutputString();
  return output;
}

// AT+CGREG?
String SIM7020::getRegistrationStatus() {
  String cmd = "AT+CGREG?";
  sendCmd(cmd);
  String output = getOutputString();
  return output;
}

// AT+CGATT?
String SIM7020::getAttachStatus() {
  String cmd = "AT+CGATT?";
  sendCmd(cmd);
  String output = getOutputString();
  return output;
}


// AT+CFUN=0 -> AT+CFUN=1
bool SIM7020::setPhoneFunctionality(int mode) {
  String cmd = "AT+CFUN=" + String(mode);
  return sendCmd(cmd, "+CPIN:");
}

// AT*MCGDEFCONT="IPV4V6",""
bool SIM7020::setPacketDataProtocol(String settings) {
  // String cmd = "AT*MCGDEFCONT=\"" + settings + "\",\"\"";
  String cmd = "AT*MCGDEFCONT=\"" + settings + "\"";
  return sendCmd(cmd);
}

// AT+CGATT=1
bool SIM7020::setAttachStatus(int status) {
  String cmd = "AT+CGATT="+String(status);
  return sendCmd(cmd);
}

// AT+CGCONTRDP
String SIM7020::getAPN() {
  String cmd = "AT+CGCONTRDP";
  sendCmd(cmd);
  return getOutputString();
}

//AT+CGCONTRDP
String SIM7020::getPublicIP() {
  String cmd = "AT+CGCONTRDP";
  sendCmd(cmd);
  return getOutputString();
}

// AT+CDNSGIP=”www.google.com" 
String SIM7020::getIPFromDNS(String domain) {
  String cmd = "AT+CDNSGIP=\""+domain+"\"";
  sendCmd(cmd, "+CDNSGIP:");
  return getOutputString();
}

// AT+CIPPING="172.217.160.68,1"
bool SIM7020::ping(String host) {
  String cmd = "AT+CIPPING=\""+host+"\",1";
  return sendCmd(cmd, "+CIPPING:");
}

// AT+CHTTPCREATE
bool SIM7020::createHTTPSocket(String host, int port) {
  String cmd = "AT+CHTTPCREATE=\""+host+":"+String(port)+"/\"";
  return sendCmd(cmd);
}

// AT+CHTTPCON=0
bool SIM7020::connectHTTPSocket(int socket_id) {
  String cmd = "AT+CHTTPCON="+String(socket_id);
  return sendCmd(cmd);
}

// AT+CHTTPSEND=0,1,"/setBikeData",4163636570743a202a2f2a0d0a436f6e6e656374696f6e3a204b6565702d416c6976650d0a557365722d4167656e743a2053494d434f4d5f4d4f44554c450d0a,"application/json",7b22646576534e223a223131313132323232222c227370656564223a2232352e36222c226c6f6e676974756465223a2233362e32222c226c61746974756465223a2239382e36222c22616c746974756465223a2231302e38222c22646972656374696f6e223a2231352e38222c22736174656c6c697465223a2235222c22766f6c74616765223a22342e32227d
bool SIM7020::sendHTTPData(int socket_id, int method, String path, String customer_header, String content_type, String content_str) {
  String encoded_customer_header = _stringToHexString(customer_header);
  String encoded_content_str = _stringToHexString(content_str);

  String cmd = "AT+CHTTPSEND="+String(socket_id)+","+String(method)+",\""+path+ \
    "\","+encoded_customer_header+",\""+content_type+"\","+encoded_content_str+"";
  return sendCmd(cmd,"+CHTTPNMIC:");
}

// AT+CHTTPCREATE?
String SIM7020::listHTTPSockets() {
  String cmd = "AT+CHTTPCREATE?";
  sendCmd(cmd);
  return getOutputString();
}

// AT+CHTTPDISCON=0
bool SIM7020::disconnectHTTPSocket(int socket_id) {
  String cmd = "AT+CHTTPDISCON="+String(socket_id);
  return sendCmd(cmd);
}

 // AT+CHTTPDESTROY=0
bool SIM7020::closeHTTPSocket(int socket_id) {
  String cmd = "AT+CHTTPDESTROY="+String(socket_id);
  return sendCmd(cmd);
}

String SIM7020::_stringToHexString(String str) {
  String output;
  char buf[3];
  for (uint16_t i=0;i<str.length();i++) {
    sprintf(buf, "%.2x", int(str.charAt(i)));
    output += String(buf);
  }
  Serial.println(output);
  return output;
}

String SIM7020::_bufferToHexString(uint8_t buf[], size_t len) {
  String output;
  char tmp_buf[3];
  for (uint16_t i=0;i<len;i++) {
    sprintf(tmp_buf, "%.2x", buf[i]);
    output += String(tmp_buf);
  }
  Serial.println(output);
  return output;
}

String SIM7020::_hexStringToString(String hex_str){
  return String();
}