#!/usr/bin/env python

import paho.mqtt.client as mqtt
import sys
import json
import requests
import base64

import weather_pb2

rxmsg = weather_pb2.weatherProto()
heading = 'Waiting for wind vane'

# The callback for when the client receives a CONNACK response from the server.

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("application/1/node/70b3d58ff0031de5/rx")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(str(msg.payload))
    print(json.loads(str(msg.payload))["data"])
    decodedData = str(base64.b64decode(json.loads(str(msg.payload))["data"]))
    print (decodedData)
    rxmsg.ParseFromString(decodedData)
    print ('Wind Speed in MPH '  + str(rxmsg.MPH))
    print ('Wind Speed in KmPH '  + str(rxmsg.KmPH))
#    print ('Wind Direction ' + str(rxmsg.calDirection))
    print ('Wind Direction ' + getHeading(rxmsg.calDirection))
    print ('Rain Fall ' + str(rxmsg.rainFall))

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set("loraserver","loraserver")

client.connect("gateways.rbccps.org", 1883, 60)
client.loop_forever()

def getHeading(direction):
        global heading
        if(direction < 22.5):
            heading = 'North'
        elif (direction < 67.5):
            heading = 'North-East'
        elif (direction < 112.5):
            heading = 'East'
        elif (direction < 157.5):
            heading = 'South-East'
        elif (direction < 212.5):
            heading = 'South'
        elif (direction < 247.5):
            heading = 'South-West'
        elif (direction < 292.5):
            heading = 'West'
        elif (direction < 337.5):
            heading = 'North-West'
        else:
            heading = 'North'
        return heading
