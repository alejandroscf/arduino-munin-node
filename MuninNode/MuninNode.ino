/*
 ESP8266 1-wire Munin Node

 Copyright (c) 2017 Alejandro Suarez Cebrian <alejandro@asuarez.es>
 Copyright (c) 2015 Hristo Gochkov. 
 (CC) 2014, 'Aztec Eagle' Turbo <turbo@ier.unam.mx>
 (CC) 2014, Instituto de Energías Renovables, UNAM
  
 A simple munin node that shows the value of 1-wire divices conected to
 an ESP8266 board.

 Based in "Arduino Munin Node" from 'Aztec Eagle' Turbo and 
 WiFiTelnetToSerial - Example Transparent UART to Telnet Server for esp8266
 from Hristo Gochkov.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.


 */

#define DEBUG 1
#define FS(X) String(F(X))

#include <ESP8266WiFi.h>

#include <OneWire.h>
#include <DallasTemperature.h>


// Change as your needs!
String nodename = "esp8266";
#define MAX_SRV_CLIENTS 1
const char* ssid = "*******";
const char* password = "**********";

// Initialize the esp8266 server library
// with the IP address and port you want to use
// (port 4949 is default for munin):
WiFiServer server(4949);
WiFiClient serverClients[MAX_SRV_CLIENTS];

// Initialize the 1-wire library with the
// GPIO port

#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);

float getTemp(int id) {
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempCByIndex(id);
#if DEBUG
    Serial.print("Temperature");
    Serial.print(id);
    Serial.print(": ");
    Serial.println(temp);
#endif
  } while (temp == 85.0 || temp == (-127.0));

  return temp;
  
}

void setup() {
#if DEBUG
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  Serial.println("Initializing...");
#endif
  // start the Ethernet connection and the server:
  WiFi.begin(ssid, password);
#if DEBUG
  Serial.print("\nConnecting to "); Serial.println(ssid);
#endif
  uint8_t i = 0;
  while (WiFi.status() != WL_CONNECTED && i++ < 20) delay(500);
  if(i == 21){
#if DEBUG    
    Serial.print("Could not connect to"); Serial.println(ssid);
#endif
    while(1) delay(500);
  }

  // start the server:
  server.begin();
  server.setNoDelay(true);
  
#if DEBUG
  Serial.print("Server is at ");
  Serial.print(WiFi.localIP());
  Serial.println(":4949");
#endif
  // setup your stuff

}

void loop() {
  // listen for incoming clients
  WiFiClient client = server.available();
  if (client) {
    //digitalWrite(13, HIGH); // turn the LED on
#if DEBUG
    Serial.println("New client");
#endif
    client.print(FS("# munin node at ") + nodename + '\n');

    while (client.connected()) {
      if (client.available()) {
#if DEBUG
        Serial.println("readString...");
#endif
        String command = client.readString();
#if DEBUG
        Serial.println(command);
#endif
        if (command.startsWith(FS("quit"))) break;
        if (command.startsWith(FS("version"))) {
          client.print(FS("munin node on ") + nodename + FS(" version: 1.0.0\n"));
          continue;
        }
        if (command.startsWith(FS("list"))) {
          client.print(FS("a0 a1 a2 a3 a4 a5\n"));
          continue;
        }
        if (command.startsWith(FS("config a"))) {
          char ch = command.charAt(8);
          if (ch>='0' && ch<='5') {
              client.print(FS("graph_title Analog Input A") + String(ch - '0') + '\n');
              // client.print(FS("graph_args -l 0 --upper-limit 1024\n"));
              // client.print(FS("graph_scale no\n"));
              // client.print(FS("graph_category arduino\n"));
              client.print(FS("sensor.label Digital value\n.\n"));
          } else client.print(FS("# Unknown service\n.\n"));
          continue;
        }
        if (command.startsWith(FS("fetch a"))) {
          char ch = command.charAt(7);
          if (ch>='0' && ch<='5') {
            // read the input on analog pin 0:
            // int sensorValue = analogRead(A0 + ch - '0');
            // float voltage = 5.0 * sensorValue / 1024;
            float sensorValue = getTemp(ch - '0');
#if DEBUG
            Serial.print("ADC.value ");
            Serial.println(sensorValue);
#endif
            client.print(FS("sensor.value ")
                       + String(sensorValue) 
                       + FS("\n.\n"));
            continue;
          } else client.print(FS("# Unknown service\n.\n")); 
          continue;
        }
        // no command catched
        client.print(FS("# Unknown command. Try list, config, fetch, version or quit\n"));
      }
    }
    // give the web browser time to receive the data
    delay(1);
    // close the connection:
    client.stop();
#if DEBUG
    Serial.println("Client disconnected");
#endif
    //digitalWrite(13, LOW); // turn the LED off
  }
}

