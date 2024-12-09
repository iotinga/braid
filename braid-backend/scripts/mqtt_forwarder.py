import argparse
import logging
import time
from argparse import Namespace

import paho.mqtt.client as mqtt

parser = argparse.ArgumentParser(description="MQTT forwarder")
parser.add_argument(
    "-b",
    "--broker",
    default="localhost",
    help="The address of the MQTT broker",
)
parser.add_argument(
    "--port",
    type=int,
    default=1883,
    help="The port to connect to the MQTT broker (default: 1883)",
)
parser.add_argument("-u", "--username", help="Username for MQTT broker authentication")
parser.add_argument("-p", "--password", help="Password for MQTT broker authentication")
parser.add_argument(
    "-f", "--from", required=True, dest="topic_from", help="Topic to subscribe to"
)
parser.add_argument(
    "-t", "--to", required=True, dest="topic_to", help="Topic to forward to"
)


class CustomFormatter(logging.Formatter):
    """Custom formatter for Python logging with aligned log messages"""

    # Fixed width for log level (left-aligned)
    max_level_width = len(
        "CRITICAL"
    )  # Adjust based on the length of the longest log level
    _format = (
        "%(asctime)s.%(msecs)03d %(levelname)-" + str(max_level_width) + "s %(message)s"
    )

    def __init__(self):
        super().__init__(fmt=self._format, datefmt="%H:%M:%S")


# Callback when the client connects to the broker
def create_on_connect(topic_from: str) -> mqtt.CallbackOnConnect:
    def on_connect(
        client: mqtt.Client,
        userdata,
        flags: mqtt.ConnectFlags,
        rc: mqtt.ReasonCode,
        properties: mqtt.Properties,
    ):
        if rc == 0:
            logging.info("Connected to broker")
            client.subscribe(topic_from, qos=1)
        else:
            logging.error(f"Failed to connect, return code {rc}")

    return on_connect


# Callback when a message is received from the broker
def create_on_message(topic_to: str) -> mqtt.CallbackOnMessage:
    def on_message(client: mqtt.Client, userdata, msg: mqtt.MQTTMessage):
        logging.info(f"{msg.topic} --- {len(msg.payload)} --> {topic_to}")
        # Forward the message
        client.publish(topic_to, msg.payload, qos=1, retain=True)

    return on_message


# Initialize the MQTT client
def connect_to_broker(client: mqtt.Client, broker: str, port: int):
    while True:
        try:
            client.connect(broker, port)
            client.loop_start()
            break
        except Exception as e:
            logging.warn(f"Error connecting to broker: {e}. Retrying in 5 seconds...")
            time.sleep(5)


def main(args: Namespace):
    # Initialize MQTT client
    client = mqtt.Client(
        callback_api_version=mqtt.CallbackAPIVersion.VERSION2,
        client_id="fake_entity_plugin",
        protocol=mqtt.MQTTv5,
    )

    # Set MQTT username and password if provided
    if args.username and args.password:
        client.username_pw_set(args.username, args.password)

    # Assign callback functions
    client.on_connect = create_on_connect(args.topic_from)
    client.on_message = create_on_message(args.topic_to)

    # Connect to the broker
    connect_to_broker(client, args.broker, args.port)

    # Keep the script running
    try:
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        client.disconnect()


if __name__ == "__main__":
    args = parser.parse_args()

    root_logger = logging.getLogger()
    root_logger.setLevel(logging.DEBUG)

    ch = logging.StreamHandler()
    ch.setLevel(logging.DEBUG)
    ch.setFormatter(CustomFormatter())
    root_logger.addHandler(ch)

    main(args)
