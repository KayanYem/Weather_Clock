# 天气时钟

**硬件：** ESP-wroom-32，0.96寸OLED显示屏，WS2812。

##### RTOS
* ProjInit: 
  * 初始化数据
  * 执行周期：0xffff
  * 优先度1
* LEDclock: 
  * LED时钟更新
  * 执行周期：200ms
  * 优先度1
* OLEDupdate: 
  * OLED更新
  * 按键切换三个页面：homepage,clockpage,weatherpage
  * 执行周期：500ms/200ms/500ms
  * 优先度1
* WeatherUpdate: 
  * 获取天气数据更新
  * 执行周期：15s
  * 优先度1
