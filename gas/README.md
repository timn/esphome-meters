Gas Meter via Inductive Sensor
==============================

This is an ESPHome configuration to read out a gas meter using an inductive
sensor. Gas meters like the Pipersberg G4 RF1 c come with a (somewhat hidden)
metal cog that can be detected with such a sensor.

In my setup I'm using an Abilkeen LJ18A3-10-Z/BY (PNP, normally open) mounted
with a [custom holder](https://www.printables.com/model/465960-gas-meter-inductive-sensor-holder-g4-rf1-c).
It has been rocksolid and not missed an impulse and not counted any doubles.
I use an ESP8266 based D1 mini in a
[magnet-held case](https://www.printables.com/model/466027-esp8266-d1-mini-case-with-perma-proto-small-mint-t).

This configuration uses only built-in ESPHome capabilities.

<p align="center">
  <img src="gas_meter.jpg?raw=true" alt="Gas meter with inductive sensor and D1 Mini"/>
</p>

Board Schematic
---------------

Here is a simple schematic of the board layout I used. No active components,
just wiring up the connector. I have used a angled pin header soldered into
the second row of holes.

<p align="center">
  <img src="mini_tin_3pin_connector.png?raw=true" alt="Small mint tin perma-proto connections"/>
</p>

