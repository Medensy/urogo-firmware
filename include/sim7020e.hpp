class SIM7020 {
  private:
    int reset_pin;
    HardwareSerial* serial_ptr;
    bool debug;
    
    String _output_str;
    String _return_str;
    
    bool readOk(void);
  public:
    void init(int baudrate, bool debug);
    void init(int baudrate, int tx_pin, int rx_pin, int reset_pin, bool debug);
    void powerOn(void);
    void powerOff(void);
    bool sendCmd(String cmd, String ok_str, String err_str);
    bool sendCmd(String cmd, String ok_str);
    bool sendCmd(String cmd);
    String getOutputString(void);
    String getReturnString(void);
    
    int setEchoMode(int mode);
    int check(); // AT
    String getModuleDetails(); // AT+GSV

    int getSignalRSSI(); // AT+CSQ
    String getPacketDataProtocol(); // AT*MCGDEFCONT?
    String getRegistrationStatus(); //AT+CGREG?
    String getAttachStatus(); // AT+CGATT?

    bool setPhoneFunctionality(int mode); //AT+CFUN=0 -> AT+CFUN=1
    bool setPacketDataProtocol(String settings); // AT*MCGDEFCONT="IPV4V6",""
    bool setAttachStatus(int status); // AT+CGATT=1

    String getAPN(); // AT+CGCONTRDP
    String getPublicIP(); //AT+CGCONTRDP
    String getIPFromDNS(String domain); // AT+CDNSGIP=”www.google.com" 
    bool ping(String host); // AT+CIPPING="172.217.160.68"
    
    bool createHTTPSocket(String host, int port); // AT+CHTTPCREATE
    bool connectHTTPSocket(int socket_id); // AT+CHTTPCON=0
    bool sendHTTPData(int socket_id, int method, String path, String customer_header, String content_type, String content_str); // AT+CHTTPSEND=0,1,"/setBikeData",4163636570743a202a2f2a0d0a436f6e6e656374696f6e3a204b6565702d416c6976650d0a557365722d4167656e743a2053494d434f4d5f4d4f44554c450d0a,"application/json",7b22646576534e223a223131313132323232222c227370656564223a2232352e36222c226c6f6e676974756465223a2233362e32222c226c61746974756465223a2239382e36222c22616c746974756465223a2231302e38222c22646972656374696f6e223a2231352e38222c22736174656c6c697465223a2235222c22766f6c74616765223a22342e32227d
    String listHTTPSockets(); // AT+CHTTPCREATE?
    bool disconnectHTTPSocket(int socket_id); // AT+CHTTPDISCON=0
    bool closeHTTPSocket(int socket_id); // AT+CHTTPDESTROY=0


    String _stringToHexString(String str);
    String _hexStringToString(String hex_str);
};
