#!/usr/bin/env python

import paho.mqtt.client as mqtt
import sys
import json
import requests
import base64

import weather_pb2


rxmsg = weather_pb2.weatherProto()


# The callback for when the client receives a CONNACK response from the server.

def on_connect(client, userdata, flags, rc):
    print("Connected with result code "+str(rc))

    # Subscribing in on_connect() means that if we lose the connection and
    # reconnect then subscriptions will be renewed.
    client.subscribe("application/1/node/70b3d58ff0031de5/rx")

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    print(msg.payload["data"])
    decodedData = str(base64.b64decode(json.loads(str(msg.payload))["data"]))
    print (decodedData)
    rxmsg.ParseFromString(decodedData)
    '''print ('')
    print ('Wind Speed in MPH '  + str(rxmsg.MPH))
    print ('Wind Speed in KmPH '  + str(rxmsg.KmPH))
    print ('Wind Direction ' + str(rxmsg.calDirection))
    print ('')'''

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set("loraserver","loraserver")

client.connect("gateways.rbccps.org", 1883, 60)
client.loop_forever()

