#!/bin/python3

"""
/**
 * @file fanju_mqtt.py
 * @author Andreas Daasch
 * @brief MQTT Adapter for FanJu Decoder (MIT Licence)
 * 
 * @date 2022-01-10
 * 
 * @copyright Copyright (c) 2022
 * 
 */
"""

import ctypes as c
import pathlib
import paho.mqtt.client as mqtt
import DHT
import time
import threading

exit_thread = False


def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")


@c.CFUNCTYPE(None, c.c_float, c.c_uint8, c.c_bool, c.c_bool, c.c_uint8)
def cb(temp, hum, bat_ok, tx_req, chan):
    print("{:.1f},{},{},{},{}".format(temp, hum, bat_ok, tx_req, chan))

    client.publish('pi/fanju/temp',
                   payload="{:.1f}".format(temp), qos=0, retain=True)
    client.publish('pi/fanju/hum', payload=hum, qos=0, retain=True)
    client.publish('pi/fanju/bat_ok', payload=bat_ok, qos=0, retain=True)


def read_dht11():
    sensor = DHT.sensor(gpio=4)
    while not exit_thread:
        res = sensor.read()
        print("{},{},{}".format(res[3], res[4], res[2]))
        client.publish('pi/dht11/temp',
                       payload="{:.1f}".format(res[3]), qos=0, retain=True)
        client.publish('pi/dht11/hum', payload=res[4], qos=0, retain=True)
        time.sleep(20)


if __name__ == "__main__":
    client = mqtt.Client()
    client.on_connect = on_connect
    client.connect("openwrt", 1883, 60)

    threading.Thread(None, read_dht11).start()

    # Load the shared library into ctypes
    libname = pathlib.Path().absolute() / "../build/libfanju.so"
    c_lib = c.CDLL(libname)

    ret = c_lib.fanju_setup(cb)
    while ret == 0:
        c_lib.fanju_loop()
    exit_thread = True
