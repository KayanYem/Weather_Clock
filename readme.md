# 天气时钟

以ESP32作为主控芯片，0.96寸OLED显示屏显示信息，WS2812时钟显示。

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
  * 执行周期：500ms
  * 优先度1
* WeatherUpdate: 
  * 获取天气数据更新
  * 执行周期：13s
  * 优先度1
