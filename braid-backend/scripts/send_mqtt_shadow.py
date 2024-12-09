import paho.mqtt.client as mqtt
import cbor2
import time
import random


# MQTT Broker details
broker = "localhost"
port = 1883
topic = "braid/agent/TEST/shadow/reported/"
username = "demo-service"
password = "abram.space"



ntc1 = random.randrange(190, 240)
ntc2 = random.randrange(190, 240)

# JSON data to be sent
shadow = {"ntc1": ntc1, "ntc2": ntc2, "status": "ok"}

message = {
    "TIMESTAMP": int(time.time() * 1000),
    "VERSION": 1,
    "PROTOCOL": 1,
    "ACTION": 4,
    "BODY": shadow,
}

# Encode JSON data to CBOR
cbor_data = b'\xBF'

for [key, value] in message.items():
    cbor_data += cbor2.dumps(key)
    cbor_data += cbor2.dumps(value)

cbor_data  += b'\xFF'


# Define the MQTT client callbacks
def on_connect(client, userdata, flags, rc):
    print(f"Connected with result code {rc}")
    # Publish the CBOR encoded data
    properties = mqtt.Properties(mqtt.PacketTypes.PUBLISH)
    properties.UserProperty = ("content-type", "application/cbor")
    client.publish(topic, cbor_data)
    print("Data published")


def on_disconnect(client, userdata, rc):
    if rc != 0:
        print("Unexpected disconnection.")


def on_publish(client, userdata, mid):
    print(f"Message {mid} published.")


# Create an MQTT client instance
client = mqtt.Client()
client.username_pw_set(username, password)

# Assign the callback functions
client.on_connect = on_connect
client.on_disconnect = on_disconnect
client.on_publish = on_publish

# Connect to the broker
client.connect(broker, port, 60)

# Start the loop
client.loop_start()

# Give some time to ensure the message is sent
import time

time.sleep(2)

# Disconnect from the broker
client.loop_stop()
client.disconnect()
