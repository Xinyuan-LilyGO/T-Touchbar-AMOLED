#pragma once


#define WIFI_SSID             "xinyuandianzi"
#define WIFI_PASSWORD         "AA15994823428"

#define WIFI_CONNECT_WAIT_MAX (30 * 1000)

#define NTP_SERVER1           "pool.ntp.org"
#define NTP_SERVER2           "time.nist.gov"
#define GMT_OFFSET_SEC        0
#define DAY_LIGHT_OFFSET_SEC  0
#define GET_TIMEZONE_API      "https://ipapi.co/timezone/"

#define LV_DELAY(x)                                                                                                                                  \
  do {                                                                                                                                               \
    uint32_t t = x;                                                                                                                                  \
    while (t--) {                                                                                                                                    \
      lv_timer_handler();                                                                                                                            \
      delay(1);                                                                                                                                      \
    }                                                                                                                                                \
  } while (0);

#define TFT_DC        16
#define TFT_RES       15
#define TFT_CS_0      13
#define TFT_CS_1      14
#define TFT_MOSI      18
#define TFT_SCK       17


#define PIN_IIC_SCL   11
#define PIN_IIC_SDA   10
#define PIN_TOUCH_INT 12
#define PIN_TOUCH_RES 9