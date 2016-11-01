#!/usr/bin/python

import mosquitto
import tweepy
import struct
import humanize
from datetime import datetime, timedelta
from config import get_auth, get_knob_mapping

twitter = tweepy.API(get_auth())
mqtt = mosquitto.Mosquitto()

def scale(value, in_min, in_max, out_min, out_max):

    return out_min + (float(value - in_min) / float(in_max-in_min)) * (out_max-out_min)

def on_connect(mqtt, userdata, rc):
    print("Connected with result code "+str(rc))

    mqtt.subscribe("spaceprobe/knob")
    mqtt.subscribe("space/status/open")

first_open = None
estimated_close = None

def on_message(mqtt, userdata, msg):
    if msg.topic == 'spaceprobe/knob':
        mqtt.publish("spaceprobe/led",struct.pack("BBBf",255,0,0,.2))
        
        try:
            value = struct.unpack("h",msg.payload)[0]
        except:
            print "got a weird payload"

        knob_map = get_knob_mapping()
        scaled = None
        for i in range(0,len(knob_map)):
            if knob_map[i] > value:
                scaled = scale(value,knob_map[i-1],knob_map[i],i-1,i)
                scaled = round(scaled*4.0)/4.0 # 15 min intervals
                break

        if scaled > 0:
            print "%s: Space Open! %.2f" %(datetime.now(),scaled) 
            mqtt.publish("space/status/open",struct.pack("f",scaled),retain=True)
            mqtt.publish("spaceprobe/led",struct.pack("BBB",0,255,0))
        else:
            print "%s: Space Closed!" %(datetime.now())
            mqtt.publish("space/status/open",struct.pack("f",0),retain=True)
            mqtt.publish("spaceprobe/led",struct.pack("BBB",0,0,0))

    #TODO: Move this to another script because MicroServices
    if msg.topic == 'space/status/open':
        try:
            duration = struct.unpack("f",msg.payload)[0]
        except:
            print "got a weird payload"

        global first_open, estimated_close

        if duration:
            estimated_close = datetime.now() + timedelta(hours=duration)
            msg = "Space %s open until about %s%s." %( 
                "is" if not first_open else "staying", 
                estimated_close.strftime("%H:%M"), 
                " tomorrow" if estimated_close.date() > datetime.today().date() else "" 
            ) 
            if first_open is None:
                first_open = datetime.now()

            twitter.update_status(msg)
        else:
            if first_open:
                msg = "Space closed (opened %s)." %( humanize.naturaltime(datetime.now() - first_open))
            else:
                msg = "Space closed." 
            first_open = None
            estimated_close = None 
            twitter.update_status(msg)

mqtt.on_connect = on_connect
mqtt.on_message = on_message

mqtt.connect("10.42.0.1", 1883, 60)

while 1:
    mqtt.loop()
    # do other stuff! 

