/* pinout configuration */
#define GLED_PIN 25
#define RLED_PIN 33
#define START_BTN_PIN 32

#define MODE_CONFIG_PIN 13

#define HX711_DOUT_PIN 15
#define HX711_SCK_PIN 2
#define HX711_PWR_EN_PIN 13

#define ADC_BATT_EN_PIN 0
#define ADC_BATT_ADC_PIN 36
#define CHRG_DT_PIN 39

// #define SD_CS0_PIN 5
#define SD_CS_PIN 14
#define SD_CLK_PIN 18
#define SD_MISO_PIN 19
#define SD_WP_PIN 21
#define SD_CD_PIN 22
#define SD_MOSI_PIN 23

#define PROG_TX_PIN 12
#define PROG_RX_PIN 35

/* board configuration */
#define SLEEP_PERIOD_US 86400*1000000 // 1 day time sync
#define MAX_DATA_LENGTH 1800 // max data buffer, sampling rate 10 SPS, record time 180 s
// #define MAX_DATA_LENGTH 30 // max data buffer, sampling rate 10 SPS, record time 180 s

/* middleware  configuration */
#define HX711_INIT_TIMEOUT 2500
#define SIM7020_INIT_TIMEOUT 30000

// nb-iot config
#define UPLOAD_SERVER "http://urogo.medensy.com"
#define UPLOAD_PORT 80
#define UPLOAD_PATH "/api/nb"
#define SNTP_SERVER "1.th.pool.ntp.org"