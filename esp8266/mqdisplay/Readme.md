## ESP8266 MQTT JSON TFT Display with Home Assistant Auto Discovery


No need to alter the code for config, it is all done via browser.  Just flash, connect to AP and configure.

After configuration, all the mqtt config info is displayed on screen.

<img src="https://raw.githubusercontent.com/packetinspector/IoT-Stuff/master/esp8266/mqdisplay/images/display_start.jpg" width="30%" height="30%">

It has many functions and works with Home Assistant.

<img src="https://raw.githubusercontent.com/packetinspector/IoT-Stuff/master/esp8266/mqdisplay/images/ha_command.png" width="35%" height="35%" align="top"><img src="https://raw.githubusercontent.com/packetinspector/IoT-Stuff/master/esp8266/mqdisplay/images/display_mqtt.jpg" width="30%" height="30%"><img src="https://raw.githubusercontent.com/packetinspector/IoT-Stuff/master/esp8266/mqdisplay/images/ha_light.png" width="35%" height="35%" align="top">

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
// Draw text  ([x,y,text])
{"command": "drawstring", "command_data": [50,150, "Test"]}
// Fill Screen ([r,g,b])
{"command": "fillscreen", "command_data": [200,0,200]}
```

Some attempt made to autosize font for display.  

This will use the MQTT Auto Discovery feature in Home Assistant and add itself as a light component. From there you can also turn the display on/off and alter the brightness.

I used the TFT_eSPI lib which is compatible with many TFT displays.

#### Sample HA Automation
```yaml
- id: '1532730381532'
  alias: Refresh Weather Display
  trigger:
  - entity_id: sensor.dark_sky_temperature
    platform: state
  condition: []
  action:
  - data_template:
      topic: 'mqdisplay/command'
      payload: >
        {"command": "message", "command_data": "DarkSky\n
        Todays Temp\nCurrent: {{ states.sensor.dark_sky_temperature.state }}\n
        High:   {{ states.sensor.dark_sky_daily_high_temperature.state }}\n
        Low:    {{ states.sensor.dark_sky_daily_low_temperature.state }}"}
    service: mqtt.publish
```


##### Parts
- NodeMCU
- ILI9341

##### Other Notes
- OTA is enabled
- I added brightness by varying the voltage on D0 to pin LED on the screen, if you want to stick to eSPI just connect it to 3.3v
- Edit PubSub.h -> MQTT_MAX_PACKET_SIZE 2048
