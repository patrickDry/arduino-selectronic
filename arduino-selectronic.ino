/*

A simple program that reads values from a Selectronic inverter over Modbus on a RS485 RTU physical connection.
Values are then packed into a char array and sent over UDP to another micro controller which shows values on a display on a wall and also controls relays to achieve load dumping easily.

I used an Arduino MKR with the 485 MKR Shield, connecting Y/Z ports on the Shield to the RS485 port on the inverter (I had an older inverter and so needed to upgrade the comms card on the inverter, purchased directly from Selectronic).

Good luck!

*/

#include <ArduinoModbus.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

const unsigned long REPORT_INTERVAL = 2000;
unsigned long lastMillis = 0;
byte mac[] = {
  0xA8, 0x61, 0x0A, 0xAE, 0x44, 0xC7
};
IPAddress localIP(10, 0, 0, 51);
const char destinationIP[] = "10.0.0.52";
unsigned int udpPort = 3333;
EthernetUDP Udp;
char finalBuffer[14];

short unpackShort(char buffer[], int index)
{
  // Not used in this program but used in my case on the other end to unpack the char array back into a bunch of shorts
  return buffer[index] << 8 | buffer[index + 1];
}

void packShort(short value, char buffer[], int index)
{
  // Bit shifting to pack a 2-byte short into a char array
  buffer[index] = value >> 8;
  buffer[index + 1] = value & 0xff;
}

void setup() {
  Serial.begin(9600);
  
  if (!ModbusRTUClient.begin(9600, SERIAL_8N1)) {
    Serial.println("Failed to start Modbus RTU Client!");
    while (1);
  }
  else
  {
    Serial.println("Modbus RTU Client initialized!");
  }

  Ethernet.init(5); // Pin for the Arduino MKR ETH Shield, might be a different point on other shields
  Ethernet.begin(mac, localIP);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    while (1);
  }
  if (Ethernet.linkStatus() == LinkOFF) {
    Serial.println("Ethernet cable is not connected.");
  }

  // Start UDP
  Udp.begin(udpPort);

  lastMillis = millis();
}

void loop() {
  if (millis() - lastMillis > REPORT_INTERVAL) {
    lastMillis = millis();
    int count = ModbusRTUClient.requestFrom(1, HOLDING_REGISTERS, 8000, 90);
    if (!count) {
      Serial.print("ERROR: ");
      Serial.println(ModbusRTUClient.lastError());
    }
    else {

      // These are the values I was intereste in, refer to the technical document by Selectronic if there are different values you want to read
      short batteryVoltage;
      short batterySoC;
      short batteryPower;
      short acLoadPower;
      short acSourcePower;
      short hydroPower;
      short solarPower;

      for(int i = 0; i < count; i++)
      {
        short value = ModbusRTUClient.read();
        switch(i)
        {
          case 0: batteryVoltage = value; break;
          case 1: batterySoC = value; break;
          case 2: batteryPower = value; break;
          case 4: acLoadPower = value; break;
          case 8: acSourcePower = value; break;
          case 84: hydroPower = 65535 - value; break; // Weird case where Shunt 1 is reported back inverted to the max value of ushort, might be hardware issue on my end (wires crossed on inverter side?)
          case 89: solarPower = value; break;
        }
      }
    
      /*
      Serial.println("--- DEBUG VALUES: ");
      
      Serial.print("Battery Voltage: ");
      Serial.println(batteryVoltage / 10.0);
      
      Serial.print("Battery SoC: ");
      Serial.println(batterySoC / 10.0);

      Serial.print("Battery Power: ");
      Serial.println(batteryPower / 100.0);

      Serial.print("AC Load: ");
      Serial.println(acLoadPower / 100.0);

      Serial.print("AC Source Power: ");
      Serial.println(acSourcePower / 100.0);

      Serial.print("Hydro Power: ");
      Serial.println(hydroPower / 100.0);

      Serial.print("Solar Power: ");
      Serial.println(solarPower / 100.0);
      */

      // Pack all values into a char array to send over UDP
      int i = 0;
      packShort(batteryVoltage, finalBuffer, i);
      i+=2;
      packShort(batterySoC, finalBuffer, i);
      i+=2;
      packShort(batteryPower, finalBuffer, i);
      i+=2;
      packShort(acLoadPower, finalBuffer, i);
      i+=2;
      packShort(acSourcePower, finalBuffer, i);
      i+=2;
      packShort(hydroPower, finalBuffer, i);
      i+=2;
      packShort(solarPower, finalBuffer, i);

      // Send UDP Packet to master controller
      int beginPacket = Udp.beginPacket(destinationIP, udpPort);
      Udp.write(finalBuffer, sizeof(finalBuffer));
      int endPacket = Udp.endPacket();
    }
    
    delay(100);
  }
}
