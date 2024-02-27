# arduino-selectronic
 
A simple program that reads values from a Selectronic inverter over Modbus on a RS485 RTU physical connection.
Values are then packed into a char array and sent over UDP to another micro controller which shows values on a display on a wall and also controls relays to achieve load dumping easily.

I used an Arduino MKR with the 485 MKR Shield, connecting Y/Z ports on the Shield to the RS485 port on the inverter (I had an older inverter and so needed to upgrade the comms card on the inverter, purchased directly from Selectronic).

Good luck!