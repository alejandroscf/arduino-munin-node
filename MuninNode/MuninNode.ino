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
#define FString(X) String(F(X))

#include <ESP8266WiFi.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// Change as your needs in credentials.h!
#include "credentials.h"

String nodename = NODE_NAME;
#define MAX_SRV_CLIENTS 1
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWD;

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

#define MAX_W1_DEVICES 32
// arrays to hold device addresses
DeviceAddress Thermometer[MAX_W1_DEVICES];
int w1devices;

uint64_t millis64() {
    static uint32_t low32, high32;
    uint32_t new_low32 = millis();
    if (new_low32 < low32) high32++;
    low32 = new_low32;
    return (uint64_t) high32 << 32 | low32;
}

float getTemp(DeviceAddress id) {
  float temp;
  do {
    DS18B20.requestTemperatures(); 
    temp = DS18B20.getTempC(id);
#if DEBUG
    Serial.print("Temperature");
    printAddress(id);
    Serial.print(": ");
    Serial.println(temp);
#endif
  } while (temp == 85.0 || temp == (-127.0));

  return temp;
  
}

// function to print a device address
void printAddress(DeviceAddress deviceAddress)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    // zero pad the address if necessary
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
}

// function to print the temperature for a device
void printTemperature(DeviceAddress deviceAddress)
{
  float tempC = DS18B20.getTempC(deviceAddress);
  Serial.print("Temp C: ");
  Serial.print(tempC);
  Serial.print(" Temp F: ");
  Serial.print(DallasTemperature::toFahrenheit(tempC));
}

// function to print a device's resolution
void printResolution(DeviceAddress deviceAddress)
{
  Serial.print("Resolution: ");
  Serial.print(DS18B20.getResolution(deviceAddress));
  Serial.println();    
}

void setup() {
#if DEBUG
  // Open serial communications and wait for port to open:
  Serial.begin(115200);
  Serial.println("Initializing...");
#endif
  // start the Ethernet connection and the server:
  WiFi.mode(WIFI_STA);
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


  // initialize 1-wire library
  DS18B20.begin();
  
  // locate devices on the bus
  w1devices = DS18B20.getDeviceCount();
#if DEBUG
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(w1devices, DEC);
  Serial.println(" devices.");

  // report parasite power requirements
  Serial.print("Parasite power is: "); 
  if (DS18B20.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");
#endif
  for (int dev = 0; dev < w1devices; dev++)
  {
    if (!DS18B20.getAddress(Thermometer[dev], dev)) {
#if DEBUG
      Serial.print("Unable to find address for Device ");
      Serial.println(dev);
#endif
    } else {
      Serial.print("Sensor ");
      Serial.print(dev);
      Serial.print(": ");
      printAddress(Thermometer[dev]);
      Serial.println();
    }
  }
  
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
    client.print(FString("# munin node at ") + nodename + '\n');

    while (client.connected()) {
      if (client.available()) {
#if DEBUG
        Serial.println("readString...");
#endif
        String command = client.readString();
#if DEBUG
        Serial.println(command);
#endif
        if (command.startsWith(FString("quit"))) break;
        if (command.startsWith(FString("version"))) {
          client.print(FString("munin node on ") + nodename + FString(" version: 1.0.0\n"));
          continue;
        }
        if (command.startsWith(FString("list"))) {
          client.print(FString("esp_w1_temp uptime\n"));
          continue;
        }
        if (command.startsWith(FString("config esp_w1_temp"))) {
          client.print(FString("graph_title ESP8266 OneWire Temperature\n"));
          client.print(FString("graph_vlabel Temperature in Celsius\n"));
          client.print(FString("graph_category Sensors\n"));
          for (int dev = 0; dev < w1devices; dev++) {
            client.print(FString("temp") + (dev + 1) + ".warning 30\n");
            client.print(FString("temp") + (dev + 1) + ".critical 40\n");
            client.print(FString("temp") + (dev + 1) + ".label "); 
            for (uint8_t i = 0; i < 8; i++)
            {
              // zero pad the address if necessary
              if (Thermometer[dev][i] < 16) client.print("0");
              client.print(Thermometer[dev][i], HEX);
            }
            client.print(FString("\n"));
          }
          client.print(FString(".\n"));
          continue;
        }
        if (command.startsWith(FString("fetch esp_w1_temp"))) {
          for (int dev = 0; dev < w1devices; dev++) {
            float temp = getTemp(Thermometer[dev]);
            String STemp = String(temp, 3);
            client.print(FString("temp") + (dev + 1) + ".value " + STemp + "\n");

          }
          client.print(FString(".\n"));
          continue;
        }
        if (command.startsWith(FString("config uptime"))) {
          client.print(FString("graph_title Uptime\n"));
          client.print(FString("graph_args --base 1000 -l 0\n"));
          client.print(FString("graph_scale no\n"));
          client.print(FString("graph_vlabel uptime in days\n"));
          client.print(FString("graph_category system\n"));
          client.print(FString("uptime.label uptime\n"));
          client.print(FString("uptime.draw AREA\n"));
          client.print(FString(".\n"));
          continue;
        }
        if (command.startsWith(FString("fetch uptime"))) {
          String Suptime = String((float)millis64() / (1000*60*60*24),3);
          client.print(FString("uptime.value ") + Suptime + "\n.\n");
          continue;
        }
        // no command catched
        client.print(FString("# Unknown command. Try list, config, fetch, version or quit\n"));
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
