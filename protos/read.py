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


def get_heading(direction):
    global heading
    if(direction < 48):
        heading = 'East'
    elif (direction < 80):
        heading = 'South-East'
    elif (direction < 116):
        heading = 'South'
    elif (direction < 177):
        heading = 'North-East'
    elif (direction < 237):
        heading = 'South-West'
    elif (direction < 290):
        heading = 'North'
    elif (direction < 327):
        heading = 'North-West'
    elif (direction < 348):
        heading = 'West'
    else:
        heading = 'East'
    return heading

# The callback for when a PUBLISH message is received from the server.
def on_message(client, userdata, msg):
    #print(str(msg.payload))
    # print(json.loads(str(msg.payload))["data"])
    decodedData = base64.b64decode(json.loads(msg.payload)["data"])
    #print (decodedData)
    parseData = rxmsg.ParseFromString(decodedData)
    #print(parseData)
    print ('')
    #print ('Wind Speed in mph - {0:.3f}'.format(rxmsg.MPH))
    print ('Wind Speed in km/h - {0:.3f}'.format(rxmsg.KmPH))
    #print ('Wind Direction ' + str(rxmsg.calDirection))
    print ('Wind Direction - ' + get_heading(rxmsg.calDirection))
    print ('Rain Fall in mm - {0:.3f}'.format(rxmsg.rainFall))
    print ('')

client = mqtt.Client()
client.on_connect = on_connect
client.on_message = on_message
client.username_pw_set("loraserver","loraserver")

client.connect("gateways.rbccps.org", 1883, 60)
client.loop_forever()
