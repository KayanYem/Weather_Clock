#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include <WiFi.h>
#include <Arduino.h>
#include <U8g2lib.h>
#include "time.h"
#include <ArduinoJson.h>
#include <Adafruit_NeoPixel.h>

/***************创建任务****************/
/*
* 初始化
* 执行周期：0xffff
* 优先度
*/
#define ProjInit_TASK_PRIO  1          // 任务优先级
#define ProjInit_STK_SIZE   1024 * 3        // 任务堆栈大小 8 * 10字节
TaskHandle_t ProjInit_TaskHandle = NULL; // 任务句柄
void ProjInit(void *pvParameters);
/*
* 获取天气数据更新
* 执行周期：6000ms
* 优先度1
*/
#define WeatherUpdate_TASK_PRIO  1          // 任务优先级
#define WeatherUpdate_STK_SIZE   1024 * 5        // 任务堆栈大小
TaskHandle_t WeatherUpdate_TaskHandle = NULL; // 任务句柄
void WeatherUpdate(void *pvParameters); //任务函数
/*
* LED时钟更新
* 执行周期：100ms
* 优先度1
*/
#define LEDclock_TASK_PRIO  1          // 任务优先级
#define LEDclock_STK_SIZE   1024 * 2        // 任务堆栈大小
TaskHandle_t LEDclock_TaskHandle = NULL; // 任务句柄
void LEDclock(void *pvParameters); //任务函数
/*
* OLED更新
* 执行周期：500ms
* 优先度1
*/
#define OLEDupdate_TASK_PRIO  1        // 任务优先级
#define OLEDupdate_STK_SIZE   1024 * 3        // 任务堆栈大小
TaskHandle_t OLEDupdate_TaskHandle = NULL; // 任务句柄
void OLEDupdate(void *pvParameters); //任务函数

/**************按键中断********************/
#define GPIO_KEY 0
#define HOMEPAGE 0
#define CLOCKPAGE 1
#define WEATHERPAGE 2
int display_page = HOMEPAGE;
int display_delay10s = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

/***********ws2812 LED clock**************/
#define PIN_hour  32
#define NUM_hour 12
#define PIN_minsec  33
#define NUM_minsec 24
Adafruit_NeoPixel hour(NUM_hour, PIN_hour, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel minsec(NUM_minsec, PIN_minsec, NEO_GRB + NEO_KHZ800);

int tmhour = 0;     // 12小时制
int tmmin = 0;      // 60分钟分为12格
int tmsec = 0;      // 60秒分为12格
int sec_color = 0;  // 一格5分钟

struct rgb {
    int r;
    int g;
    int b;
};


/**************OLED初始化*********************/
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/U8X8_PIN_NONE, /* clock=*/18, /* data=*/19);  // ESP32 Thing, HW I2C with pin remapping


/**************Weather LOGO******************/
const uint8_t qingtianbaitian[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,
  0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,0xC0,0x80,0x01,0x03,0xC0,0x81,0x81,0x03,
  0x80,0x03,0xC0,0x03,0x00,0xE7,0xE7,0x01,0x00,0xFE,0xDF,0x00,0x00,0x3C,0x3C,0x00,
  0x00,0x1C,0x78,0x00,0x00,0x0E,0x70,0x00,0x00,0x06,0x60,0x00,0xFC,0x06,0xE0,0x3F,
  0xFC,0x06,0xE0,0x3F,0x00,0x06,0x60,0x00,0x00,0x0E,0x60,0x00,0x00,0x1C,0x70,0x00,
  0x00,0x3C,0x38,0x00,0x00,0xFA,0x7F,0x00,0x00,0xF7,0xEF,0x00,0x80,0x87,0xC1,0x01,
  0xC0,0x83,0x81,0x03,0xC0,0x81,0x01,0x03,0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,
  0x00,0x80,0x01,0x00,0x00,0x80,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const uint8_t qingtianyewan[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFC,0x07,0x00,
  0x00,0x18,0x1C,0x00,0x00,0x10,0x70,0x00,0x00,0x30,0xC0,0x00,0x00,0x20,0x80,0x01,
  0x00,0x60,0x00,0x01,0x00,0x40,0x00,0x02,0x00,0x40,0x00,0x06,0x00,0x40,0x00,0x04,
  0x00,0x40,0x00,0x04,0x00,0x40,0x00,0x0C,0x00,0x40,0x00,0x0C,0x00,0x40,0x00,0x0C,
  0x00,0x40,0x00,0x0C,0x00,0x60,0x00,0x0C,0x00,0x20,0x00,0x0C,0x00,0x30,0x00,0x04,
  0x00,0x18,0x00,0x04,0x00,0x0C,0x00,0x06,0x00,0x06,0x00,0x02,0x80,0x03,0x00,0x01,
  0xE0,0x00,0x80,0x01,0x60,0x00,0xC0,0x00,0xC0,0x01,0x70,0x00,0x00,0x07,0x1C,0x00,
  0x00,0xFC,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const uint8_t baoxue[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0xF8,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x78,0x00,0x00,0x02,0xF0,0x01,0x00,0x03,0x00,0x03,0xC0,0x01,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x06,0x40,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0xFE,0x3F,0x00,
  0x00,0x00,0x00,0x00,0x80,0x20,0x04,0x01,0xA0,0x2A,0x54,0x05,0xE0,0x79,0x9E,0x07,
  0xE0,0x79,0x9E,0x07,0xA0,0x20,0x44,0x05,0x80,0x00,0x00,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const uint8_t baoyu[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0x38,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x70,0x00,0x00,0x02,0xF0,0x01,0x80,0x01,0x00,0x03,0xC0,0x00,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x02,0xC0,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x40,0x00,0x00,0x44,0x44,0x00,0x00,0x66,0x66,0x00,0x00,0x22,0x32,0x00,
  0x00,0x33,0x11,0x00,0x00,0x91,0x19,0x00,0x80,0x88,0x08,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

const uint8_t daxue[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0xF8,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x78,0x00,0x00,0x02,0xF0,0x01,0x00,0x03,0x00,0x03,0xC0,0x01,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x06,0x40,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0xFE,0x3F,0x00,
  0x00,0x00,0x00,0x00,0x00,0x02,0x40,0x00,0x80,0xCA,0x53,0x01,0x00,0xCF,0xF3,0x00,
  0x00,0xCF,0xF3,0x00,0x80,0x0A,0x50,0x01,0x00,0x02,0x40,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t dayu[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0x38,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x70,0x00,0x00,0x02,0xF0,0x01,0x80,0x01,0x00,0x03,0xC0,0x00,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x02,0xC0,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x40,0x00,0x00,0x44,0x44,0x00,0x00,0x66,0x66,0x00,0x00,0x22,0x32,0x00,
  0x00,0x33,0x11,0x00,0x00,0x91,0x19,0x00,0x80,0x88,0x08,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t dongyu[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0xF8,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x78,0x00,0x00,0x02,0xD0,0x01,0x80,0x03,0x00,0x03,0xC0,0x00,0x00,0x02,
  0x40,0x00,0x00,0x02,0x60,0x00,0x00,0x02,0x60,0x00,0x00,0x02,0x40,0x00,0x00,0x02,
  0xC0,0x00,0x00,0x03,0x80,0x01,0xC0,0x01,0x00,0xFF,0x7F,0x00,0x00,0x00,0x00,0x00,
  0x00,0x12,0x89,0x00,0x00,0x93,0x49,0x00,0x00,0x99,0x64,0x00,0x80,0xC9,0x26,0x00,
  0x80,0x40,0x22,0x00,0x40,0x20,0x01,0x00,0x40,0x22,0x11,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t duoyun[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,0x00,0x00,0x02,0x00,
  0x00,0x08,0x82,0x01,0x00,0x18,0xC0,0x00,0x00,0xB0,0x4F,0x00,0x00,0xC0,0x18,0x00,
  0x00,0x60,0x30,0x00,0x00,0xF8,0x23,0x00,0x00,0x0C,0x6E,0x07,0x00,0x06,0x78,0x07,
  0x00,0x02,0xF0,0x00,0x00,0x01,0x80,0x01,0xC0,0x01,0x00,0x03,0x40,0x00,0x00,0x02,
  0x20,0x00,0x00,0x06,0x20,0x00,0x00,0x06,0x20,0x00,0x00,0x02,0x20,0x00,0x00,0x02,
  0x60,0x00,0x00,0x03,0xC0,0xFF,0xFF,0x01,0x00,0xFF,0x7F,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t leizhenyu[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0xF8,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x78,0x00,0x00,0x02,0xF0,0x01,0x80,0x03,0x00,0x03,0xC0,0x00,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x06,
  0x60,0x00,0x00,0x06,0xC0,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x8C,0x41,0x00,0x00,0x84,0x21,0x00,0x00,0xC6,0x31,0x00,
  0x00,0x82,0x11,0x00,0x00,0x81,0x18,0x00,0x00,0x41,0x08,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t leizhenyubanbingbao[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0x38,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x78,0x00,0x00,0x02,0xC0,0x01,0x80,0x03,0x00,0x03,0xC0,0x00,0x00,0x02,
  0x40,0x00,0x00,0x02,0x60,0x00,0x00,0x02,0x60,0x00,0x00,0x02,0x40,0x00,0x00,0x02,
  0xC0,0x00,0x00,0x01,0x80,0xFF,0xFF,0x01,0x00,0xFE,0x7F,0x00,0x00,0x00,0x00,0x00,
  0x00,0x84,0x20,0x00,0x00,0xC4,0x20,0x00,0x00,0xC6,0x31,0x00,0x00,0xC2,0x11,0x00,
  0x00,0x83,0x00,0x00,0x80,0x41,0x04,0x00,0x00,0x40,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t shachenbao[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0xC0,0xFF,0xFF,0x00,0xC0,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xF8,0xFF,0x03,0x00,0xFC,0xFF,0x03,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xFC,0x7F,0x00,0x00,0xFE,0x7F,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xFF,0x03,0x00,0x00,0xFF,0x03,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xFC,0x01,0x00,0x00,0xFC,0x01,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0xF0,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t wu[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0x38,0x06,0x00,0x00,0x0C,0x08,0x00,
  0x00,0x06,0x70,0x00,0x00,0x02,0xC0,0x01,0x80,0x01,0x00,0x03,0xC0,0x00,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x02,0xC0,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xFF,0xFF,0x00,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t wumai[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0xFF,0xFF,0x03,0xC0,0xFF,0xFF,0x03,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x00,0x00,0x01,0xC0,0xFF,0xFF,0x03,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0xFF,0xFF,0x03,
  0x80,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x80,0x99,0x99,0x01,
  0x80,0x08,0x10,0x01,0x00,0x62,0x66,0x00,0x00,0x66,0x66,0x00,0x00,0x00,0x00,0x00,
  0x80,0x99,0x99,0x01,0x80,0x98,0x19,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t xiaoxue[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0xF8,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x78,0x00,0x00,0x02,0xF0,0x01,0x00,0x03,0x00,0x03,0xC0,0x00,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x02,0xC0,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0xFE,0x3F,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xC0,0x03,0x00,0x00,0xC0,0x03,0x00,
  0x00,0xC0,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t xiaoyu[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0x38,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x70,0x00,0x00,0x02,0xF0,0x01,0x00,0x03,0x00,0x03,0xC0,0x00,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x02,0x40,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x8C,0x31,0x00,0x00,0x84,0x10,0x00,0x00,0x00,0x00,0x00,
  0x00,0x43,0x08,0x00,0x00,0x21,0x0C,0x00,0x00,0x21,0x04,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t yangsha[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xC0,0x01,0x00,0x00,0x40,0xC2,0x03,0x00,0x00,0x43,0x06,0xE0,0xFF,0x61,0x04,
  0x00,0x00,0x00,0x06,0xE0,0xFF,0xFF,0x03,0xE0,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0xE0,0xFF,0xFF,0x00,0x00,0x00,0x80,0x01,0x00,0x00,0x00,0x01,0x60,0x93,0x10,0x01,
  0x00,0x00,0xF0,0x00,0x80,0x64,0x00,0x00,0x00,0x00,0x00,0x00,0x60,0x93,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t yejianduoyun[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1C,0x00,0x00,0x00,0x68,0x00,0x00,
  0x00,0xC8,0x00,0x00,0x00,0x88,0x01,0x00,0x00,0x08,0x01,0x00,0x00,0x0C,0x01,0x00,
  0x00,0x04,0x03,0x00,0x00,0xE6,0x1F,0x00,0x00,0x33,0x30,0x00,0xF0,0x18,0xE0,0x00,
  0x30,0x08,0xC0,0x03,0x60,0x06,0x00,0x06,0xC0,0x03,0x00,0x0C,0x80,0x01,0x00,0x08,
  0x80,0x00,0x00,0x08,0x80,0x00,0x00,0x08,0x80,0x00,0x00,0x08,0x80,0x01,0x00,0x0C,
  0x00,0x03,0x00,0x06,0x00,0xFE,0xFF,0x03,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t yejianzhenxue[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x50,0x00,0x00,0x00,0x90,0x00,0x00,
  0x00,0x10,0x01,0x00,0x00,0x10,0x01,0x00,0x00,0x88,0x07,0x00,0x00,0x64,0x08,0x00,
  0x80,0x13,0x30,0x00,0x80,0x10,0xC0,0x00,0x00,0x0D,0x00,0x01,0x00,0x06,0x00,0x03,
  0x00,0x02,0x00,0x02,0x00,0x02,0x00,0x02,0x00,0x02,0x00,0x01,0x00,0x04,0x80,0x00,
  0x00,0xF8,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x40,0x00,0x00,0x00,0xE0,0x10,0x00,
  0x00,0xE0,0x38,0x00,0x00,0x40,0x38,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t yejianzhenyu[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x0C,0x00,0x00,0x00,0x38,0x00,0x00,0x00,0x48,0x00,0x00,
  0x00,0xC8,0x00,0x00,0x00,0xE8,0x07,0x00,0x00,0x34,0x0C,0x00,0x00,0x1A,0x30,0x00,
  0xC0,0x09,0xF0,0x01,0x80,0x06,0x00,0x03,0x00,0x03,0x00,0x02,0x00,0x01,0x00,0x02,
  0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x02,0x00,0x01,0x00,0x03,0x00,0x02,0x80,0x01,
  0x00,0xFC,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0xC8,0x24,0x00,0x00,0x00,0x01,0x00,
  0x00,0x22,0x09,0x00,0x00,0x40,0x02,0x00,0x00,0x48,0x12,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t yintian[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0xF8,0x07,0x00,0x00,0x0E,0x0C,0x00,0x00,0x03,0x10,0x00,0x00,0x01,0xE0,0x01,
  0x80,0x01,0xE0,0x07,0x80,0x00,0x00,0x0C,0xE0,0x00,0x00,0x08,0x30,0x00,0x00,0x10,
  0x18,0x00,0x00,0x10,0x08,0x00,0x00,0x10,0x08,0x00,0x00,0x10,0x08,0x00,0x00,0x10,
  0x08,0x00,0x00,0x18,0x18,0x00,0x00,0x08,0x30,0x00,0x00,0x0C,0xE0,0xFF,0xFF,0x03,
  0x80,0xFF,0x7F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t yujiabingbao[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0xF8,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x78,0x00,0x00,0x02,0xD0,0x01,0x80,0x03,0x00,0x03,0xC0,0x00,0x00,0x02,
  0x40,0x00,0x00,0x02,0x60,0x00,0x00,0x02,0x60,0x00,0x00,0x02,0x40,0x00,0x00,0x02,
  0xC0,0x00,0x00,0x03,0x80,0x01,0xC0,0x01,0x00,0xFF,0x7F,0x00,0x00,0x00,0x00,0x00,
  0x00,0x04,0x21,0x00,0x00,0x84,0x31,0x00,0x00,0x86,0x10,0x00,0x00,0x42,0x18,0x00,
  0x00,0x43,0x08,0x00,0x80,0x01,0x0C,0x00,0x80,0x20,0x04,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t yujiaxue[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0xF8,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x78,0x00,0x00,0x02,0xF0,0x01,0x00,0x03,0x00,0x03,0xC0,0x01,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x06,0xC0,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0xFE,0x3F,0x00,
  0x00,0x00,0x00,0x00,0x00,0x04,0xC2,0x00,0x00,0x1F,0x42,0x00,0x00,0x0E,0x00,0x00,
  0x00,0x9F,0x30,0x00,0x00,0x84,0x10,0x00,0x00,0x80,0x10,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t zhenxue[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xF0,0x07,0x00,0x00,0x0C,0x0C,0x00,0x00,0x06,0x18,0x00,
  0x00,0x02,0xF0,0x00,0x00,0x03,0x00,0x03,0x80,0x01,0x00,0x02,0x40,0x00,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x06,
  0x40,0x00,0x00,0x03,0xC0,0x01,0x80,0x01,0x00,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,
  0x00,0x08,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x1C,0x10,0x00,0x00,0x3E,0x7C,0x00,
  0x00,0x08,0x38,0x00,0x00,0x00,0x7C,0x00,0x00,0x00,0x10,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t zhenyu[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0x38,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x70,0x00,0x00,0x02,0xF0,0x01,0x80,0x03,0x00,0x03,0xC0,0x00,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x02,0xC0,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xCC,0xCC,0x00,0x00,0x00,0x00,0x00,0x00,0x33,0x33,0x00,
  0x00,0x00,0x00,0x00,0x00,0x44,0x44,0x00,0x00,0x44,0x44,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t zhongxue[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0xF8,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x78,0x00,0x00,0x02,0xF0,0x01,0x00,0x03,0x00,0x03,0xC0,0x01,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x06,0x40,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0xFE,0x3F,0x00,
  0x00,0x00,0x00,0x00,0x00,0x08,0x10,0x00,0x00,0x28,0x14,0x00,0x00,0x3C,0x3C,0x00,
  0x00,0x3C,0x3C,0x00,0x00,0x08,0x10,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t zhongyu[] PROGMEM = {
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0xE0,0x01,0x00,0x00,0x38,0x07,0x00,0x00,0x0C,0x0C,0x00,
  0x00,0x06,0x70,0x00,0x00,0x02,0xF0,0x01,0x80,0x01,0x00,0x03,0xC0,0x00,0x00,0x06,
  0x60,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,0x20,0x00,0x00,0x04,
  0x60,0x00,0x00,0x02,0xC0,0x00,0x00,0x03,0x80,0xFF,0xFF,0x01,0x00,0x00,0x00,0x00,
  0x00,0x00,0x20,0x00,0x00,0x84,0x30,0x00,0x00,0xC6,0x10,0x00,0x00,0x42,0x18,0x00,
  0x00,0x63,0x08,0x00,0x00,0x21,0x04,0x00,0x80,0x31,0x04,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};  

const uint8_t N_A[] PROGMEM = {
  0x00,0xF0,0x0F,0x00,0x00,0x3E,0x7C,0x00,0x80,0x03,0xC0,0x01,0xC0,0x00,0xE0,0x03,
  0x60,0x00,0xF0,0x07,0x30,0x00,0xF8,0x0F,0x38,0x00,0xFC,0x1F,0x7C,0x00,0xFE,0x3F,
  0x7C,0xE0,0xFF,0x3F,0x7E,0xE0,0xFF,0x7F,0x7E,0xE0,0xFF,0x7F,0xFE,0xE0,0xFF,0x7F,
  0xFF,0xE7,0xFF,0xFF,0xFF,0xCF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xC7,0xFF,0xFF,0xFF,0x81,0xFF,0xFF,0xFF,0x01,0xFF,0xFF,0xFF,0x00,0xFE,
  0xFE,0xFF,0x01,0x78,0xFE,0xFF,0x03,0x78,0xFE,0xFF,0x03,0x78,0xFC,0xFF,0x07,0x38,
  0xFC,0xFF,0x0F,0x38,0xF8,0xFF,0x1F,0x1C,0xF0,0xFF,0x1F,0x0C,0xE0,0xFF,0x1F,0x06,
  0xC0,0xFF,0x1F,0x03,0x80,0xFF,0xDF,0x01,0x00,0xFE,0x7F,0x00,0x00,0xF0,0x0F,0x00,
};

/*********获取天气*********/
String cityName;
String weather;
String temperature;
// String code = "";                        //天气现象代码
int LOGO_CODE;                            //天气现象代码
//今天天气
String weather_day_0;                     //白天天气现象文字
String weather_night_0;                   //晚间天气现象文字
String tempH_0;                           //当天最高温度
String tempL_0;                           //当天最低温度
String humidity_0;                        //相对湿度，0~100，单位为百分比
String wind_direction_0;                  //风向文字
String wind_scale_0;                      //风力等级
//明天天气
String weather_day_1;                     //白天天气现象文字
String weather_night_1;                   //晚间天气现象文字
String tempH_1;                           //当天最高温度
String tempL_1;                           //当天最低温度
//后天天气
String weather_day_2;                     //白天天气现象文字
String weather_night_2;                   //晚间天气现象文字
String tempH_2;                           //当天最高温度
String tempL_2;                           //当天最低温度

const int httpPort = 80;                  //端口号
const char* host = "api.seniverse.com";   //服务器地址
String reqUserKey = "SVnyz4LZkdYpoTav5";  //知心天气API私钥
String reqLocation = "佛山";              //地址
String reqUnit = "c";                     //摄氏度
//-------------------http请求-----------------------------//
String reqRes_now = "/v3/weather/now.json?key=" + reqUserKey + +"&location=" + reqLocation + "&language=zh-Hans&unit=" + reqUnit;
String httprequest_now = String("GET ") + reqRes_now + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n";

String reqRes_daily = "/v3/weather/daily.json?key=" + reqUserKey + +"&location=" + reqLocation + "&language=zh-Hans&unit=" + reqUnit + "&start=0&days=5";
String httprequest_daily = String("GET ") + reqRes_daily + " HTTP/1.1\r\n" + "Host: " + host + "\r\n" + "Connection: close\r\n\r\n";
//--------------------------------------------------------//
// API接口地址 实时天气 https://api.seniverse.com/v3/weather/now.json?key=SVnyz4LZkdYpoTav5&location=foshan&language=zh-Hans&unit=c
// 未来天气 https://api.seniverse.com/v3/weather/daily.json?key=SVnyz4LZkdYpoTav5&location=foshan&language=zh-Hans&unit=c&start=0&days=5


/***********获取网络时间**************/
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;
const char* ntpServer = "pool.ntp.org";
struct tm timeinfo;

/**************WIFI*************/
const char* ssid     = "YemKayan";
const char* password = "20191447";

WiFiServer server(80);

/******************链接WIFI*********************/
void wifi_connect() {
  //提示信息
  Serial.print("Wifi connecting");

  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setFontDirection(0);

  u8g2.clearBuffer();
  u8g2.setCursor(0, 16);
  u8g2.print("Wifi connecting ...");
  u8g2.sendBuffer();

  //连接Wifi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("\nWifi Connected!");

  u8g2.clearBuffer();
  u8g2.setCursor(0, 16);
  u8g2.print("Wifi connected!!!");
  u8g2.sendBuffer();
  delay(1000);
}

/***************访问网站 解析数据******************/
// Allocate the JSON document
// Use https://arduinojson.org/v6/assistant to compute the capacity.
void httpRequest() {

  WiFiClient client_now;
  WiFiClient client_daily;
  
  const size_t capacity_now = 512;
  const size_t capacity_daily = 1536;
  
  DynamicJsonDocument doc_now(capacity_now);
  DynamicJsonDocument doc_daily(capacity_daily);
  
  // 获取实时天气
  if (client_now.connect(host, httpPort)) {
    Serial.println("连接成功，接下来发送请求");
    client_now.print(httprequest_now);  //访问API接口
    String response_status = client_now.readStringUntil('\n');
    Serial.println(response_status);

    if (client_now.find("\r\n\r\n")) {
      Serial.println("响应报文体找到，开始解析");
    }
    
    deserializeJson(doc_now, client_now);
    JsonObject obj_now = doc_now["results"][0];

    if (obj_now["location"]["name"].as<String>() != "null") {
      cityName = obj_now["location"]["name"].as<String>();
      weather = obj_now["now"]["text"].as<String>();
      temperature = obj_now["now"]["temperature"].as<String>();
      LOGO_CODE = obj_now["now"]["code"].as<int>();

      // Serial.println(obj_now["last_update"].as<String>());
        
    }    
  } else {
    Serial.println("实时天气获取失败");
  }
  client_now.stop();
  
  // 获取未来天气
  if (client_daily.connect(host, httpPort)) {
    Serial.println("连接成功，接下来发送请求");
    client_daily.print(httprequest_daily);  //访问API接口
    String response_status = client_daily.readStringUntil('\n');
    Serial.println(response_status);

    if (client_daily.find("\r\n\r\n")) {
      Serial.println("响应报文体找到，开始解析");
    }
    
    deserializeJson(doc_daily, client_daily);
    JsonObject obj_daily = doc_daily["results"][0];
    JsonObject obj_daily_0 = obj_daily["daily"][0];
    JsonObject obj_daily_1 = obj_daily["daily"][1];
    JsonObject obj_daily_2 = obj_daily["daily"][2];
  
    if (obj_daily["location"]["name"].as<String>() != "null") {
      
      //今天天气
      weather_day_0 = obj_daily_0["text_day"].as<String>();                 //白天天气现象文字
      weather_night_0 = obj_daily_0["text_night"].as<String>();             //晚间天气现象文字
      tempH_0 = obj_daily_0["high"].as<String>();                           //当天最高温度
      tempL_0 = obj_daily_0["low"].as<String>();                            //当天最低温度
      humidity_0 = obj_daily_0["humidity"].as<String>();                    //相对湿度，0~100，单位为百分比
      wind_direction_0 = obj_daily_0["wind_direction"].as<String>();        //风向文字
      wind_scale_0 = obj_daily_0["wind_scale"].as<String>();                //风力等级
      //明天天气
      weather_day_1 = obj_daily_1["text_day"].as<String>();                 //白天天气现象文字
      weather_night_1 = obj_daily_1["text_night"].as<String>();             //晚间天气现象文字
      tempH_1 = obj_daily_1["high"].as<String>();                           //当天最高温度
      tempL_1 = obj_daily_1["low"].as<String>();                            //当天最低温度
      //后天天气
      weather_day_2 = obj_daily_2["text_day"].as<String>();                 //白天天气现象文字
      weather_night_2 = obj_daily_2["text_night"].as<String>();             //晚间天气现象文字
      tempH_2 = obj_daily_2["high"].as<String>();                           //当天最高温度
      tempL_2 = obj_daily_2["low"].as<String>();                            //当天最低温度
      
      // Serial.println(obj_daily["last_update"].as<String>());
    }    
  } else {
    Serial.println("未来天气获取失败");
  }
  client_daily.stop();
  
}

/*****************按键中断*****************/
void KEY_interrupt(){
  portENTER_CRITICAL_ISR(&mux);         // 进入临界区
  // display_delay10s = 0;              // 返回主页计数重置
  if(display_page < 2)
  {
    display_page+=1;
  }
  else
  {
    display_page = 0;
  }
  portEXIT_CRITICAL_ISR(&mux);          // 退出临界区
  // Serial.println("KEY_interrupt"); 
}

/****************页面更新*********************/
void Homepage() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  
  switch(LOGO_CODE)
  {
    case(0):
    case(1):
      u8g2.drawXBMP(10, 3, 32, 32, qingtianbaitian);
      u8g2.setCursor(14, 48);
      u8g2.print("晴天");
      break;    
    case(3):
    case(2):
      u8g2.drawXBMP(10, 3, 32, 32, qingtianyewan);
      u8g2.setCursor(14, 48);
      u8g2.print("晴天");
      break;    
    case(4):
      u8g2.drawXBMP(10, 3, 32, 32, yintian);
      u8g2.setCursor(14, 48);
      u8g2.print("多云");
      break;    
    case(5):
      u8g2.drawXBMP(10, 3, 32, 32, duoyun);
      u8g2.setCursor(2, 48);
      u8g2.print("晴间多云");
      break;
    case(6):
      u8g2.drawXBMP(10, 3, 32, 32, yejianduoyun);
      u8g2.setCursor(2, 48);
      u8g2.print("晴间多云");
      break;
    case(7):
      u8g2.drawXBMP(10, 3, 32, 32, duoyun);
      u8g2.setCursor(2, 48);
      u8g2.print("大部多云");
      break;
    case(8):
      u8g2.drawXBMP(10, 3, 32, 32, yejianduoyun);
      u8g2.setCursor(2, 48);
      u8g2.print("大部多云");
      break;
    case(9):
      u8g2.drawXBMP(10, 3, 32, 32, duoyun);
      u8g2.setCursor(14, 48);
      u8g2.print("阴天");
      break;
    case(10):
      u8g2.drawXBMP(10, 3, 32, 32, zhenyu);
      u8g2.setCursor(14, 48);
      u8g2.print("阵雨");
      break;
    case(11):
      u8g2.drawXBMP(10, 3, 32, 32, leizhenyu);
      u8g2.setCursor(8, 48);
      u8g2.print("雷阵雨");
      break;
    case(12):
      u8g2.drawXBMP(10, 3, 32, 32, leizhenyu);
      u8g2.setCursor(0, 48);
      u8g2.print("雷阵雨有冰雹");
      break;  
    case(13):
      u8g2.drawXBMP(10, 3, 32, 32, xiaoyu);
      u8g2.setCursor(14, 48);
      u8g2.print("小雨");
      break;  
    case(14):
      u8g2.drawXBMP(10, 3, 32, 32, zhongyu);
      u8g2.setCursor(14, 48);
      u8g2.print("中雨");
      break;  
      u8g2.drawXBMP(10, 3, 32, 32, zhongyu);
      u8g2.setCursor(14, 48);
      u8g2.print("中雨");
      break;  
    case(15):
      u8g2.drawXBMP(10, 3, 32, 32, dayu);
      u8g2.setCursor(14, 48);
      u8g2.print("大雨");
      break;  
    case(16):
      u8g2.drawXBMP(10, 3, 32, 32, baoyu);
      u8g2.setCursor(14, 48);
      u8g2.print("暴雨");
      break;  
    case(17):
      u8g2.drawXBMP(10, 3, 32, 32, baoyu);
      u8g2.setCursor(14, 48);
      u8g2.print("大暴雨");
      break;  
    case(18):
      u8g2.drawXBMP(10, 3, 32, 32, baoyu);
      u8g2.setCursor(2, 48);
      u8g2.print("特大暴雨");
      break;  
    case(19):
      u8g2.drawXBMP(10, 3, 32, 32, baoyu);
      u8g2.setCursor(14, 48);
      u8g2.print("冻雨");
      break;  
    case(20):
      u8g2.drawXBMP(10, 3, 32, 32, yujiaxue);
      u8g2.setCursor(8, 48);
      u8g2.print("雨夹雪");
      break;  
    case(21):
      u8g2.drawXBMP(10, 3, 32, 32, zhenxue);
      u8g2.setCursor(14, 48);
      u8g2.print("阵雪");
      break;  
    case(22):
      u8g2.drawXBMP(10, 3, 32, 32, xiaoxue);
      u8g2.setCursor(14, 48);
      u8g2.print("小雪");
      break;  
    case(23):
      u8g2.drawXBMP(10, 3, 32, 32, zhongxue);
      u8g2.setCursor(14, 48);
      u8g2.print("中雪");
      break;  
    case(24):
      u8g2.drawXBMP(10, 3, 32, 32, daxue);
      u8g2.setCursor(14, 48);
      u8g2.print("大雪");
      break;  
    case(25):
      u8g2.drawXBMP(10, 3, 32, 32, baoxue);
      u8g2.setCursor(14, 48);
      u8g2.print("暴雪");
      break;  
    case(26):
      u8g2.drawXBMP(10, 3, 32, 32, yangsha);
      u8g2.setCursor(14, 48);
      u8g2.print("浮尘");
      break;  
    case(27):
      u8g2.drawXBMP(10, 3, 32, 32, shachenbao);
      u8g2.setCursor(8, 48);
      u8g2.print("沙尘暴");
      break;  
    case(28):
    case(29):
      u8g2.drawXBMP(10, 3, 32, 32, shachenbao);
      u8g2.setCursor(2, 48);
      u8g2.print("强沙尘暴");
      break;  
    case(30):
      u8g2.drawXBMP(10, 3, 32, 32, wu);
      u8g2.setCursor(20, 48);
      u8g2.print("雾");
      break;  
    case(31):
      u8g2.drawXBMP(10, 3, 32, 32, wumai);
      u8g2.setCursor(20, 48);
      u8g2.print("霾");
      break;  
    case(32):
    case(33):
    case(34):
    case(35):
    case(36):
    case(37):
    case(38):
    default:
      u8g2.drawXBMP(10, 3, 32, 32, N_A);
      u8g2.setCursor(16, 48);
      u8g2.print("N_A");
      break;
  }

  //更新天气
  u8g2.setCursor(0, 63);
  u8g2.print(cityName);
  u8g2.print(" ");
  u8g2.print(temperature);
  u8g2.print("℃");

  //更新时间 时间已从ledclock获取
  u8g2.setCursor(72, 47);
  u8g2.print(&timeinfo, "%b %d");
  u8g2.setCursor(64, 63);
  u8g2.print(&timeinfo, "%A");
  
  u8g2.setFont(u8g2_font_logisoso18_tr);
  u8g2.setCursor(63, 27);
  u8g2.print(&timeinfo, "%R");
  u8g2.sendBuffer();
}
void ClockPage() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_logisoso24_tr);
  //更新时间 时间已从ledclock获取
  u8g2.setCursor(8, 40);
  u8g2.print(&timeinfo, "%T");
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  u8g2.setCursor(8, 60);
  u8g2.print(&timeinfo, "%F");
  u8g2.setCursor(75, 60);
  u8g2.print(&timeinfo, "%A");
  u8g2.sendBuffer();
}
void WeatherPage() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_wqy12_t_gb2312);
  
  u8g2.drawBox(63,24,64,1);
  u8g2.drawBox(63,49,64,1);

  // 今天天气
  u8g2.setCursor(0, 11);
  u8g2.print("今 ");
  u8g2.print(tempL_0);
  u8g2.print("~");
  u8g2.print(tempH_0);
  u8g2.print("℃");
  
  u8g2.setCursor(0, 24);
  u8g2.print("日");
  u8g2.setCursor(15, 24);
  u8g2.print(weather_day_0);
  u8g2.setCursor(0, 36);
  u8g2.print("夜");
  u8g2.setCursor(15, 36);
  u8g2.print(weather_night_0);
  
  u8g2.setCursor(0, 49);
  u8g2.print("hum:  ");
  u8g2.print(humidity_0);
  u8g2.print("%");
  
  u8g2.setCursor(0, 63);
  u8g2.print(wind_direction_0);
  u8g2.print(wind_scale_0);
  u8g2.print("级");

  // 明天天气
  u8g2.setCursor(64, 11);
  u8g2.print("明 ");
  u8g2.print(tempL_1);
  u8g2.print("~");
  u8g2.print(tempH_1);
  u8g2.print("℃");
  
  u8g2.setCursor(64, 23);
  u8g2.print(weather_day_1);
  u8g2.setCursor(96, 23);
  u8g2.print("/");
  u8g2.print(weather_night_1);
  
  // 后天天气
  u8g2.setCursor(64, 36);
  u8g2.print("后 ");
  u8g2.print(tempL_2);
  u8g2.print("~");
  u8g2.print(tempH_2);
  u8g2.print("℃");
  
  u8g2.setCursor(64, 48);
  u8g2.print(weather_day_2);
  u8g2.setCursor(96, 48);
  u8g2.print("/");
  u8g2.print(weather_night_2);
  
  u8g2.setFont(u8g2_font_4x6_tr);
  u8g2.setCursor(100, 63);
  u8g2.print("by Yem");
  // u8g2.setCursor0, 63);
  // u8g2.print("WeatherPage");(
  u8g2.sendBuffer();
}


/****************************************
* LED时钟更新
* 执行周期：200ms
* 优先度1
*****************************************/
void LEDclock(void *pvParameters){
  while(true)
  {
    rgb sec_rgb = {150,50,50};
    
    getLocalTime(&timeinfo);
    Serial.println(&timeinfo, "%b %d %a %T");  // 格式化输出
    
    tmhour = timeinfo.tm_hour % 12;   // 12小时制
    tmmin = timeinfo.tm_min / 5;      // 60分钟分为12格
    tmsec = timeinfo.tm_sec / 5;      // 60秒分为12格
    sec_color = timeinfo.tm_sec % 5;  // 一格5分钟，改变颜色深浅

    // Serial.println("tmhour\t tmmin\t tmsec");  // 格式化输出
    // Serial.print(tmhour);  // 格式化输出
    // Serial.print(tmmin);  // 格式化输出
    // Serial.println(tmsec);  // 格式化输出

    hour.clear();
    minsec.clear();

    if(timeinfo.tm_hour >= 5 && timeinfo.tm_hour < 18){
      hour.setPixelColor(tmhour, hour.Color(255, 255, 255));
    }
    else{
      hour.setPixelColor(tmhour, hour.Color(100, 100, 255));
    }
    
    if(sec_color == 0){
      minsec.setPixelColor( tmsec*2-2,  minsec.Color(0,0,0));    
    }
    if(sec_color < 3){
      minsec.setPixelColor( tmsec*2-1,  minsec.Color((uint8_t)(sec_rgb.r*(0.5-0.2*sec_color)),  (uint8_t)(sec_rgb.g*(0.5-0.2*sec_color)),   (uint8_t)(sec_rgb.b*(0.5-0.2*sec_color))));    
    }    
    else{
      minsec.setPixelColor( tmsec*2-1,  minsec.Color(0,0,0));
      minsec.setPixelColor( tmsec*2+3,  minsec.Color((uint8_t)(sec_rgb.r*(0.2*sec_color-0.5)),  (uint8_t)(sec_rgb.g*(0.2*sec_color-0.5)),   (uint8_t)(sec_rgb.b*(0.2*sec_color-0.5))));      
    }
    minsec.setPixelColor( tmsec*2,      minsec.Color((uint8_t)(sec_rgb.r*(1-0.2*sec_color)),    (uint8_t)(sec_rgb.g*(1-0.2*sec_color)),     (uint8_t)(sec_rgb.b*(1-0.2*sec_color))));
    minsec.setPixelColor( tmsec*2+1,    minsec.Color((uint8_t)(sec_rgb.r*(0.5+0.2*sec_color)),  (uint8_t)(sec_rgb.g*(0.5+0.2*sec_color)),   (uint8_t)(sec_rgb.b*(0.5+0.2*sec_color))));
    minsec.setPixelColor( (tmsec*2+2),  minsec.Color((uint8_t)(sec_rgb.r*(0.2*sec_color)),      (uint8_t)(sec_rgb.g*(0.2*sec_color)),       (uint8_t)(sec_rgb.b*(0.2*sec_color))));

    minsec.setPixelColor(tmmin*2, minsec.Color(255, 255, 50));
    hour.show();
    minsec.show();
    vTaskDelay(200/portTICK_PERIOD_MS); //等待200ms
  }
}

/****************************************
* OLED更新
* 执行周期：500ms/200ms/500ms
* 优先度1
*****************************************/
void OLEDupdate(void *pvParameters) {
  while(true)
  {
    // 5s返回主页    
    // display_delay10s+=1;
    // if(display_delay10s >= 10){
    //   display_delay10s = 0;
    //   display_page = 0;
    // }
    
    if(display_page == 0){
      Homepage();
      Serial.println("OLED homepage");      // 主页
      vTaskDelay(500/portTICK_PERIOD_MS);   // 等待500ms
    }
    if(display_page == 1){
      ClockPage();
      Serial.println("OLED ClockPage");     // 时钟页面 
      vTaskDelay(200/portTICK_PERIOD_MS);   // 等待200ms
    }
    if(display_page == 2){
      WeatherPage();
      Serial.println("OLED WeatherPage");   // 天气页面
      vTaskDelay(500/portTICK_PERIOD_MS);   // 等待500ms
    }
  }
}

/****************************************
* 获取天气数据更新
* 执行周期：15s
* 优先度1
*****************************************/
void WeatherUpdate(void *pvParameters){
  while(true)
  {
    httpRequest();
    Serial.println("WeatherUpdate runing........");
    vTaskDelay(1000*15/portTICK_PERIOD_MS);         //等待15s
  }
}

/****************************************
* 初始化
* 执行周期：0xffff
* 优先度
*****************************************/
void ProjInit(void *pvParameters){
  while(true){
    //WS2812 init
    hour.begin();
    minsec.begin();

    //OLED init
    u8g2.begin();
    u8g2.enableUTF8Print();  // enable UTF8 support for the Arduino print() function
    delay(10);

    // WIFI连接
    wifi_connect();

    // 获取时间
    // 从网络时间服务器上获取并设置时间
    // 获取成功后芯片会使用RTC时钟保持时间的更新
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    delay(500);
    
    portENTER_CRITICAL(&mux);           //进入临界区
    
    //按键中断初始化
    pinMode(GPIO_KEY, INPUT_PULLUP);
    attachInterrupt(digitalPinToInterrupt(GPIO_KEY), KEY_interrupt, FALLING); 

    Serial.println("ready"); 
    vTaskDelete(NULL);
    
    portEXIT_CRITICAL(&mux);            //退出临界区
  }
}

void setup() {
  Serial.begin(115200);
  
  // 创建任务
  xTaskCreate(ProjInit, "ProjInit", ProjInit_STK_SIZE, NULL, ProjInit_TASK_PRIO, &ProjInit_TaskHandle);
  vTaskDelay(1000*5/portTICK_PERIOD_MS); //等待5s

  xTaskCreate(WeatherUpdate, "WeatherUpdate", WeatherUpdate_STK_SIZE, NULL, WeatherUpdate_TASK_PRIO, &WeatherUpdate_TaskHandle);
  xTaskCreate(LEDclock, "LEDclock", LEDclock_STK_SIZE, NULL, LEDclock_TASK_PRIO, &LEDclock_TaskHandle); 
  xTaskCreate(OLEDupdate, "OLEDupdate", OLEDupdate_STK_SIZE, NULL, OLEDupdate_TASK_PRIO, &OLEDupdate_TaskHandle);
}

void loop() {

}