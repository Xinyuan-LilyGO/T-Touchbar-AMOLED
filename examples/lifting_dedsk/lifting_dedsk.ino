#include <Arduino.h>
#include "jd9613.h"
#include "image_logo.h"
#include "lvgl.h"
#include "pin_config.h"
#include "SPI.h"
#include "time_date.h"
#include <WiFi.h>
#include "time.h"
#include "sntp.h"
#define TOUCH_MODULES_CST_SELF
#include "TouchLib.h"
#include "Wire.h"
#include <WiFiClientSecure.h>

#ifndef BOARD_HAS_PSRAM
#error "Please turn on PSRAM option to OPI PSRAM"
#endif

static const uint16_t     screenWidth = 294*2; //screenWidth = 294 * 2;
static const uint16_t     screenHeight = 126;
static const size_t lv_buffer_size = screenWidth*screenHeight*sizeof(lv_color_t);
static lv_disp_draw_buf_t draw_buf;
static lv_color_t    *     buf=NULL;

lv_obj_t* lv_show_time_date_t = NULL;
lv_obj_t *wifi_test_obj = NULL;
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "time.nist.gov";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;
struct tm timeinfo;
struct tm show_timeinfo;

extern date_time_t date_time;
extern uint8_t logo_map[];

float desk_height = 77;
float memory_bnt1_t = 110;
float memory_bnt2_t = 77;
int lock_t = 0;
int state_rel = 0;
char standby_en = 1;

static EventGroupHandle_t touch_eg;
#define GET_TOUCH_INT _BV(1)

LV_IMG_DECLARE(dock_gif);
LV_IMG_DECLARE(touch_pr);
LV_IMG_DECLARE(touch_res);
LV_IMG_DECLARE(up_img);
LV_IMG_DECLARE(down_img);
LV_IMG_DECLARE(lock_img);
LV_IMG_DECLARE(unlock_img);
LV_FONT_DECLARE(AlimamaShuHeiTi_Bold_54);
LV_IMG_DECLARE(rgb454249);
LV_IMG_DECLARE(rgb686776);

const char* week[7] = {"Monday","Tuesday","Wednesday","Thursday","Friday","Saturday", "Sunday"};
const char* cn_week[7] = { "星期一","星期二","星期三","星期四","星期五","星期六","星期日" };

TouchLib    touch(Wire, PIN_IIC_SDA, PIN_IIC_SCL, CTS820_SLAVE_ADDRESS);

void lv_test(void);
void wifi_test(void);

void my_print(const char *buf) {
    Serial.printf(buf);
    Serial.flush();
}

inline void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p)
{
    //uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    int _w1 = 294 - area->x1;
    int _w2 = area->x2 - 294 + 1;

    if (_w1 > 0)
    {
        TFT_CS_0_L;
        lcd_PushColors_SoftRotation(area->x1,
                                    area->y1,
                                    _w1,
                                    h,
                                    (uint16_t *)&color_p->full,
                                    1); // Horizontal display
        TFT_CS_0_H;
    }
    if (_w2 > 0)
    {
        TFT_CS_1_L;
        lcd_PushColors_SoftRotation(0,
                                    area->y1,
                                    _w2,
                                    h,
                                    (uint16_t *)&color_p->full,
                                    2); // Horizontal display
        TFT_CS_1_H;
    }

    lv_disp_flush_ready(disp);
}

static void lv_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data)
{
    if (touch.read())
    {
        TP_Point t = touch.getPoint(0);
        int16_t x = 126-t.x;
        int16_t y = x;
        x = t.y;
        data->point.x = x;
        t.x = x;
        data->point.y = y;
        t.y = y;

        /* Adjust black shadow areas. */
        if (t.x > 326) data->point.x = t.x - 32;

        if (t.x > 294 && t.x < 326)
            data->state = LV_INDEV_STATE_REL;
        else
            data->state = LV_INDEV_STATE_PR;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
}

void setup()
{
    touch_eg = xEventGroupCreate();
    // put your setup code here, to run once:
    Serial.begin(115200);

    jd9613_init();
    TFT_CS_0_L;
    lcd_PushColors(0, 0,   294, 126,(uint16_t *)logo_map, 3);
    TFT_CS_0_H;
    TFT_CS_1_L;
    lcd_PushColors(0, 0,   294, 126,(uint16_t *)logo_map, 1);
    TFT_CS_1_H;
    delay(3000);

    lv_init();

    buf  = (lv_color_t*)ps_malloc(lv_buffer_size);
    assert(buf);
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * screenHeight);

    /*Initialize the display*/
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    /*Change the following line to your display resolution*/
    disp_drv.hor_res = screenWidth;
    disp_drv.ver_res = screenHeight;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    disp_drv.full_refresh = 1;
    lv_disp_drv_register(&disp_drv);

    Wire.begin(PIN_IIC_SDA, PIN_IIC_SCL);
    touch.init();
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lv_touchpad_read;
    lv_indev_drv_register(&indev_drv);

    sntp_servermode_dhcp(1);    // (optional)
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);

    wifi_test();
    lv_test();
    if(wifi_test_obj != NULL)
        lv_obj_del_delayed(wifi_test_obj,1);
}

void printLocalTime()
{
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("No time available (yet)");
        return;
    }
    show_timeinfo.tm_year = timeinfo.tm_year;
    show_timeinfo.tm_hour = timeinfo.tm_hour;
    show_timeinfo.tm_mon = timeinfo.tm_mon;
    show_timeinfo.tm_min = timeinfo.tm_min;
    show_timeinfo.tm_wday = timeinfo.tm_wday;
    show_timeinfo.tm_mday = timeinfo.tm_mday;
    show_timeinfo.tm_sec = timeinfo.tm_sec;
    //Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

// Callback function (get's called when time adjusts via NTP)
void timeavailable(struct timeval *t)
{
    Serial.println("Got time adjustment from NTP!");
    printLocalTime();
    WiFi.disconnect();
}

void wifi_test(void)
{
    String text;

    lv_obj_t *wifi_test_obj = lv_obj_create(lv_scr_act());
    lv_obj_align(wifi_test_obj, LV_ALIGN_CENTER, 0, 0);

    lv_obj_set_size(wifi_test_obj, 294*2, 126);
    lv_obj_set_style_bg_color(wifi_test_obj, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_opa(wifi_test_obj, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wifi_test_obj, 0, 0);
    lv_obj_set_style_radius(wifi_test_obj, 0, 0);

    lv_obj_t *log_label = lv_label_create(wifi_test_obj);
    lv_obj_align(log_label, LV_ALIGN_TOP_LEFT, 0, 0);
    lv_obj_set_width(log_label, LV_PCT(100));
    lv_label_set_long_mode(log_label, LV_LABEL_LONG_SCROLL);
    lv_label_set_recolor(log_label, true);
    lv_obj_set_style_text_color(log_label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(log_label, "Scan WiFi");
    LV_DELAY(1);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    LV_DELAY(100);
    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
        text = "no networks found";
    }
    else
    {
        text = n;
        text += " networks found\n";
        for (int i = 0; i < n; ++i)
        {
            text += (i + 1);
            text += ": ";
            text += WiFi.SSID(i);
            text += " (";
            text += WiFi.RSSI(i);
            text += ")";
            text += (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " \n" : "*\n";
            delay(10);
        }
    }
    lv_label_set_text(log_label, text.c_str());
    Serial.println(text);
    LV_DELAY(2000);
    text = "Connecting to ";
    Serial.print("Connecting to ");
    text += WIFI_SSID;
    text += "\n";
    Serial.print(WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    uint32_t last_tick = millis();

    bool is_smartconfig_connect = false;
    lv_label_set_long_mode(log_label, LV_LABEL_LONG_WRAP);
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        text += ".";
        lv_label_set_text(log_label, text.c_str());
        LV_DELAY(100);
        if (millis() - last_tick > WIFI_CONNECT_WAIT_MAX)
        { /* Automatically start smartconfig when connection times out */
            text += "\nConnection timed out, start smartconfig";
            lv_label_set_text(log_label, text.c_str());
            LV_DELAY(100);
            is_smartconfig_connect = true;
            WiFi.mode(WIFI_AP_STA);
            Serial.println("\r\n wait for smartconfig....");
            text += "\r\n wait for smartconfig....";
            text += "\nPlease use #ff0000 EspTouch# Apps to connect to the "
                    "distribution network";
            lv_label_set_text(log_label, text.c_str());
            WiFi.beginSmartConfig();
            while (1)
            {
                LV_DELAY(100);
                if (WiFi.smartConfigDone())
                {
                    Serial.println("\r\nSmartConfig Success\r\n");
                    Serial.printf("SSID:%s\r\n", WiFi.SSID().c_str());
                    Serial.printf("PSW:%s\r\n", WiFi.psk().c_str());
                    text += "\nSmartConfig Success";
                    text += "\nSSID:";
                    text += WiFi.SSID().c_str();
                    text += "\nPSW:";
                    text += WiFi.psk().c_str();
                    lv_label_set_text(log_label, text.c_str());
                    LV_DELAY(1000);
                    last_tick = millis();
                    break;
                }
            }
        }
    }
    if (!is_smartconfig_connect)
    {
        text += "\nCONNECTED \nTakes ";
        Serial.print("\n CONNECTED \nTakes ");
        text += millis() - last_tick;
        Serial.print(millis() - last_tick);
        text += " ms\n";
        Serial.println(" millseconds");
        lv_label_set_text(log_label, text.c_str());
    }
    LV_DELAY(2000);
}

static lv_obj_t *dis = NULL;

static void but_implement(lv_event_t* event)
{
    lv_obj_t* btn = lv_event_get_target(event); //获取事件对象
    lv_obj_t* obj = lv_obj_get_parent(btn); // 获取对象的父亲
    lv_obj_t* label = lv_obj_get_child(obj, 1);
    
    if(lock_t && ((int)event->user_data != 6))
        return;

    switch ((int)event->user_data)
    {
        case 1:
            desk_height >= 160 ? desk_height:desk_height += desk_height >= 100 ? 1:0.5;
        break;
        case 2:
            desk_height <= 70 ? desk_height : desk_height -= desk_height >= 100 ? 1:0.5;
        break;
        case 3:
            desk_height = memory_bnt1_t;
        break;            
        case 4:
            desk_height = memory_bnt2_t;
        break;    
        case 5:
        case 6:
        {
            if(state_rel)
                return;
            state_rel = 1;
            int i = 0;
            for(i = 0; i < 6; i++)
            {
                switch (i)
                {
                case 1:
                break;
                case 0:
                case 2:
                {
                    lv_obj_t* but = lv_obj_get_child(obj, i);
                    if(but != NULL)
                    {
                        (int)event->user_data == 6?lv_obj_add_flag(but, LV_OBJ_FLAG_CLICKABLE):lv_obj_clear_flag(but, LV_OBJ_FLAG_CLICKABLE);
                        lv_obj_t* img_t = lv_obj_get_child(but, 0);
                        if(img_t != NULL) 
                        {
                            lv_obj_set_style_img_recolor(img_t, (int)event->user_data == 6?lv_color_hex(0xffffff):lv_color_hex(0x615f5f), 0);
                        }
                    }
                }
                case 3:
                case 4:
                {
                    lv_obj_t* but = lv_obj_get_child(obj, i);
                    if(but != NULL)
                    {
                        (int)event->user_data == 6?lv_obj_add_flag(but, LV_OBJ_FLAG_CLICKABLE):lv_obj_clear_flag(but, LV_OBJ_FLAG_CLICKABLE);
                        lv_obj_t* lable_t = lv_obj_get_child(but, 0); 
                        if(lable_t != NULL) 
                        {
                            lv_obj_set_style_text_color(lable_t, (int)event->user_data == 6?lv_color_hex(0xffffff):lv_color_hex(0x615f5f), 0);
                        }
                    }
                }
                break;
                case 5:
                {
                    lv_obj_t* but = lv_obj_get_child(obj, i);
                    if(but != NULL)
                    {
                        (int)event->user_data == 6?lock_t = 0:lock_t = 1;
                        lv_obj_t* img_t = lv_obj_get_child(but, 0);
                        if(img_t != NULL) 
                        {
                            lv_img_set_src(img_t, (int)event->user_data == 6?&unlock_img:&lock_img);
                        }
                    }
                }
                break;
                default:
                    break;
                }
            }
        }
        break;
        case 7:    
            memory_bnt1_t = desk_height;
        break;
        case 8:    
            memory_bnt2_t = desk_height;
        break;
        default:
            break;
    }
    if(label != NULL)
    {
        char str[20] = {0};
        int i = 0;
        str[i] = desk_height/100 + '0';
        if(str[i] != '0')
        {
            i++;
        }
        str[i++] = (int)desk_height/10%10 + '0';
        str[i++] = (int)desk_height%10 + '0';
        if(desk_height < 100)
        {
            if(desk_height != (int)desk_height)
            {
                str[i++] = '.';
                str[i] = '5';
            }
            else 
            {
                str[i++] = '.';
                str[i] = '0';
            }
        }
        my_print(str);
        lv_label_set_text(label, str); 
    }
}

void lv_test(void)
{
    dis = lv_tileview_create(lv_scr_act());
    lv_obj_align(dis, LV_ALIGN_TOP_RIGHT, 0, 0);
    lv_obj_set_size(dis, screenWidth, screenHeight);
    lv_obj_set_style_pad_all(dis, 0, 0);
    lv_obj_set_style_bg_color(dis, lv_color_hex(0x000000), LV_PART_MAIN);
    
    lv_obj_t *lifting_desk_t = lv_tileview_add_tile(dis, 0, 0, LV_DIR_VER);
    lv_obj_t* up_btn = lv_btn_create(lifting_desk_t);
    lv_obj_add_event_cb(up_btn, but_implement, LV_EVENT_CLICKED, (void *)1); 
    lv_obj_set_style_pad_all(up_btn, 0, 0);
    lv_obj_set_size(up_btn, 80, 70); 
    lv_obj_set_pos(up_btn, 0, 28);
    lv_obj_set_style_bg_color(up_btn, lv_color_hex(0x454249), LV_PART_MAIN);
    lv_obj_set_style_shadow_opa(up_btn, LV_OPA_0, 0);
    lv_obj_set_style_pad_all(up_btn, 0, 0);
    lv_obj_set_style_radius(up_btn, 15, 0);
    lv_obj_t* up_btn_img = lv_img_create(up_btn);
    lv_obj_set_style_shadow_opa(up_btn_img, LV_OPA_0, 0); 
    lv_obj_set_size(up_btn_img, 60, 60);  
    lv_img_set_src(up_btn_img, &up_img);
    lv_obj_align(up_btn_img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_img_recolor(up_btn_img, lv_color_hex(0xffffff), 0); 
    lv_obj_set_style_img_recolor_opa(up_btn_img, LV_OPA_90, 0);
    
	lv_obj_t* label; 
	label = lv_label_create(lifting_desk_t);
	lv_obj_remove_style_all(label);
    lv_obj_add_state(label, LV_PART_MAIN);	
	lv_obj_set_style_bg_opa(label, LV_OPA_100, 0);
	lv_obj_set_style_text_color(label, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_bg_color(label, lv_color_hex(0x000000), 0);
	lv_label_set_text(label, "77.0"); 
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_pos(label, 85, 42);            
	lv_obj_set_size(label, 120, 40);				
    lv_obj_set_style_text_font(label, &AlimamaShuHeiTi_Bold_54, 0);

    lv_obj_t* down_btn = lv_btn_create(lifting_desk_t);
    lv_obj_add_event_cb(down_btn, but_implement, LV_EVENT_CLICKED, (void *)2); 
    lv_obj_set_size(down_btn, 80, 70); 
    lv_obj_set_pos(down_btn, 210, 28);;
    lv_obj_set_style_bg_color(down_btn, lv_color_hex(0x454249), 0);
    lv_obj_set_style_shadow_opa(down_btn, LV_OPA_0, 0);  
    lv_obj_set_style_pad_all(down_btn, 0, 0);  
    lv_obj_t* down_btn_img = lv_img_create(down_btn);
    lv_obj_set_style_shadow_opa(down_btn_img, LV_OPA_0, 0); 
    lv_obj_set_size(down_btn_img, 60, 60);  
    lv_img_set_src(down_btn_img, &down_img);
    lv_obj_align(down_btn_img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_img_recolor(down_btn_img, lv_color_hex(0xffffff), 0); 
    lv_obj_set_style_img_recolor_opa(down_btn_img, LV_OPA_90, 0);

    lv_obj_t* memory_bnt1 = lv_btn_create(lifting_desk_t);
    lv_obj_add_event_cb(memory_bnt1, but_implement, LV_EVENT_CLICKED, (void *)3); 
    lv_obj_add_event_cb(memory_bnt1, but_implement, LV_EVENT_LONG_PRESSED, (void *)7); 
    lv_obj_set_size(memory_bnt1, 80, 70); 
    lv_obj_set_pos(memory_bnt1, 297, 28);
    lv_obj_set_style_bg_color(memory_bnt1, lv_color_hex(0x454249), 0);
    lv_obj_set_style_shadow_opa(memory_bnt1, LV_OPA_0, 0);  
    lv_obj_set_style_pad_all(memory_bnt1, 0, 0);  
    lv_obj_t* memory_bnt1_label = lv_label_create(memory_bnt1);	
    lv_label_set_text(memory_bnt1_label, "1"); 
    lv_obj_set_style_text_font(memory_bnt1_label, &AlimamaShuHeiTi_Bold_54, LV_PART_MAIN);
    lv_obj_set_style_text_color(memory_bnt1_label, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_pad_all(memory_bnt1_label, 0, 0); 
    lv_obj_center(memory_bnt1_label);

    lv_obj_t* memory_bnt2 = lv_btn_create(lifting_desk_t);
    lv_obj_add_event_cb(memory_bnt2, but_implement, LV_EVENT_CLICKED, (void *)4);
    lv_obj_add_event_cb(memory_bnt2, but_implement, LV_EVENT_LONG_PRESSED, (void *)8);  
    lv_obj_set_size(memory_bnt2, 80, 70); 
    lv_obj_set_pos(memory_bnt2, 403, 28);
    lv_obj_set_style_bg_color(memory_bnt2, lv_color_hex(0x454249), 0);
    lv_obj_set_style_shadow_opa(memory_bnt2, LV_OPA_0, 0);  
    lv_obj_set_style_pad_all(memory_bnt2, 0, 0);  
    lv_obj_t* memory_bnt2_label = lv_label_create(memory_bnt2);	
    lv_label_set_text(memory_bnt2_label, "2");
    lv_obj_set_style_text_font(memory_bnt2_label, &AlimamaShuHeiTi_Bold_54, LV_PART_MAIN);
    lv_obj_set_style_text_color(memory_bnt2_label, lv_color_hex(0xffffff), 0);
	lv_obj_set_style_pad_all(memory_bnt2_label, 0, 0);  
    lv_obj_center(memory_bnt2_label);

    lv_obj_t* lock = lv_btn_create(lifting_desk_t);
    lv_obj_add_event_cb(lock, but_implement, LV_EVENT_CLICKED, (void *)5); 
    lv_obj_add_event_cb(lock, but_implement, LV_EVENT_LONG_PRESSED, (void *)6); 
    lv_obj_set_size(lock, 80, 70); 
    lv_obj_set_pos(lock, 507, 28);; 
    lv_obj_set_style_bg_color(lock, lv_color_hex(0x454249), 0);
    lv_obj_set_style_shadow_opa(lock, LV_OPA_0, 0);  
    lv_obj_set_style_pad_all(lock, 0, 0);  
    lv_obj_t* lock_btn_img = lv_img_create(lock);
    lv_obj_set_style_shadow_opa(lock_btn_img, LV_OPA_0, 0); 
    lv_obj_set_size(lock_btn_img, 60, 60);  
    lv_img_set_src(lock_btn_img, &unlock_img);
    lv_obj_align(lock_btn_img, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_img_recolor(lock_btn_img, lv_color_hex(0xffffff), 0); 
    lv_obj_set_style_img_recolor_opa(lock_btn_img, LV_OPA_90, 0);
}

void lv_show_time_date(void)
{
    lv_show_time_date_t = lv_obj_create(lv_layer_top());
    lv_obj_align(lv_show_time_date_t, LV_ALIGN_CENTER, 0, 0);

    lv_obj_set_size(lv_show_time_date_t, 294*2, 126);
    lv_obj_set_style_bg_color(lv_show_time_date_t, lv_color_hex(0x000000), LV_PART_MAIN);
    lv_obj_set_style_border_opa(lv_show_time_date_t, LV_OPA_0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(lv_show_time_date_t, 0, 0);
    lv_obj_set_style_radius(lv_show_time_date_t, 0, 0);

    char date_t[30] = { 0 };
    sprintf(date_t, "%s\n%04d/%02d/%02d",week[show_timeinfo.tm_wday-1], show_timeinfo.tm_year+1900, show_timeinfo.tm_mon+1, show_timeinfo.tm_mday);
    my_print(date_t);
    lv_obj_t* date_label = lv_label_create(lv_show_time_date_t);
    lv_obj_set_style_text_color(date_label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(date_label, date_t); 
    lv_obj_set_style_pad_all(date_label, 0, 0); 
    lv_obj_set_style_text_align(date_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(date_label, 12, 18);              
    lv_obj_set_size(date_label, 270, 100);				
    lv_obj_set_style_text_font(date_label, &AlimamaShuHeiTi_Bold_54, LV_PART_MAIN);

    char time_t[30] = { 0 };
    sprintf(time_t, "%02d:%02d",show_timeinfo.tm_hour+6>=24?show_timeinfo.tm_hour+6-24:show_timeinfo.tm_hour+6, show_timeinfo.tm_min);
    lv_obj_t* time_label = lv_label_create(lv_show_time_date_t);
    lv_obj_set_style_text_color(time_label, lv_color_hex(0xffffff), 0);
    lv_label_set_text(time_label, time_t); 
    lv_obj_set_style_pad_all(time_label, 0, 0); 
    lv_obj_set_style_text_align(time_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(time_label, 330, 40);              
    lv_obj_set_size(time_label, 200, 40);				
    lv_obj_set_style_text_font(time_label, &AlimamaShuHeiTi_Bold_54, LV_PART_MAIN);
}

void loop()
{
    // put your main code here, to run repeatedly:
    lv_timer_handler();
    delay(1);

    if(state_rel)
    {
        state_rel++;
        if(state_rel > 300)
            state_rel = 0;
    }

    if (((lv_disp_get_inactive_time(NULL) >= 10000) && standby_en)) 
    {
        printLocalTime();
        lv_show_time_date();
        standby_en = 0;
    }
    else if ((lv_disp_get_inactive_time(NULL) < 10000) && !standby_en)
    {
        standby_en = 1;
        if(lv_show_time_date_t !=NULL)
            lv_obj_del_delayed(lv_show_time_date_t,1);
        lv_show_time_date_t = NULL;
    }
    if (((lv_disp_get_inactive_time(NULL) >= 10000) && !standby_en)) 
    {
        printLocalTime();
        lv_obj_t* label = lv_obj_get_child(lv_show_time_date_t, 0);
        if(label != NULL)
        {
            char date_t[30] = { 0 };
            sprintf(date_t, "%s\n%04d/%02d/%02d",week[show_timeinfo.tm_wday-1], show_timeinfo.tm_year+1900, show_timeinfo.tm_mon+1, show_timeinfo.tm_mday);
            my_print(date_t);
            lv_label_set_text(label, date_t); 
        }
        label = lv_obj_get_child(lv_show_time_date_t, 1);
        if(label != NULL)
        {
            char time_t[30] = { 0 };
            sprintf(time_t, "%02d:%02d",show_timeinfo.tm_hour+6>=24?show_timeinfo.tm_hour+6-24:show_timeinfo.tm_hour+6, show_timeinfo.tm_min);
            lv_label_set_text(label, time_t); 
        }
    }
}
