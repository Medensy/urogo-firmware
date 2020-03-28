class SDCARD {
  private:
    int _ss_pin;
    bool _debug;
  public:
    bool init(int ss_pin, bool debug);
    uint8_t getCardType();
    uint64_t getUsedBytes();
    uint64_t getTotalBytes();

    void printCardDetails();
    void printFileList();

    size_t writeFile(String file_name, uint8_t buf[], size_t len);
};