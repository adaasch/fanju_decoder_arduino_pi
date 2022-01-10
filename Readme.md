# Decoder for FanJu temperature/humidity remote sensor

This repo contains a decoder for the RF protocol, which is used by FanJu (https://de.aliexpress.com/item/32858575543.html).  
There a 2 implementations, one for Arduino and one for the Pi.  
To recieve the signal, you can either use a TI CC1101 or the cheap XY-MK-5v.
Just connect the GD0/GD2 line or DATA line to D2 on the Arduino or GPIO25 on the Pi.

The Pi version contains also a MQTT forwarder and the ability to read a local DHT11.

## References

https://manual.pilight.org/protocols/433.92/weather/fanju.html

https://www.chaosgeordend.nl/wordpress/2021/02/18/fanju-temperature-sensor-transmission-protocol/#more-828