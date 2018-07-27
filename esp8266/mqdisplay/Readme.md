## ESP8266 MQTT JSON TFT Display with Home Assistant Auto Discovery


No need to alter the code for config, it is all done via browser.  Just flash, connect to AP and configure.

After configuration, all the mqtt config info is displayed on screen.
![coolpic](https://raw.githubusercontent.com/packetinspector/IoT-Stuff/master/esp8266/mqdisplay/images/display_start.jpg)

It has many functions and works with Home Assistant.

You can control it via JSON.

Example Payloads:
```json
// Set Brightness
{"command": "brightness", "command_data": "50" }
// Display text
{"command": "message", "command_data": "My Cool Message" }
// HA Template
{"command": "message", "command_data": " {{ states.sensor.dark_sky_daily_summary.state }} "}
// Whats playing?
{"command": "message", "command_data": " {{ states.media_player.kodi.attributes.media_series_title }} "}
// Show things
{"command": "message", "command_data": "High: {{ states.sensor.dark_sky_daily_high_temperature.state}} \nLow:  {{ states.sensor.dark_sky_daily_low_temperature.state}} \n"}
// Will use new line chars to break
{"command": "message", "command_data": "DarkSky\nTodays Temp\nCurrent: {{ states.sensor.dark_sky_temperature.state }}\nHigh:    {{ states.sensor.dark_sky_daily_high_temperature.state}}\nLow:     {{ states.sensor.dark_sky_daily_low_temperature.state}}"}
// Reconnect to mqtt - You could use this "rediscover" the light component
{"command": "mqreconnect"}
// Reset your device
{"command": "reset"}
```

Some attempt made to autosize font for display.  

This will use the MQTT Auto Discovery feature in Home Assistant and add itself as a light component. From there you can also turn the display on/off and alter the brightness.

I used the TFT_eSPI lib which is compatible with many TFT displays
##### Parts
- NodeMCU
- ILI9341

##### Other Notes
OTA is enabled
