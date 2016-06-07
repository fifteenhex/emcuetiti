Embeddable MQTT broker

What

A really basic MQTT broker that fits into a few K of text (code and readonly data) and
requires very little run time memory wherever you put it.

Why

For many IoT applications you want to talk to a remote broker but also allow some level
of local control. So instead of an MQTT client you really want a broker that can talk
to local clients while also acting as client to a remote broker.

How

There are 3 main parts: The broker, clients and ports.

The broker works like a really basic MQTT broker. It handles setting up connections from clients,
taking publishes from them and sending publishes to them. Unlike a normal broker like mosquitto 
routing between connected clients doesn't happen by default. When a publish comes into the broker
it's topic is resolved and the publish is handed off to any ports that are listening to the topic.
It's then up to the port to decide how to handle the publish. 

Ports could be used to implement:

-	An interface to drive your local application logic from publishes coming into the broker
-	An MQTT client that passes messages local messages to a remote broker
-	client->client routing like a standard broker.


Limitations

-	+ wildcard is not supported.