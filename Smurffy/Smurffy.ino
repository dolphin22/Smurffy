#include <stdlio.h>
#include <stdlib.h>
#include <Adafruit_CC3000.h>
#include <SPI.h>
#include <avr/wdt.h>
#include <Wire.h>
#include <LibHumidity.h>

LibHumidity sht25 = LibHumidity(0);

// Define CC3000 chip pins
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

// Create CC3000 instances
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIV2); // you can change this clock speed

// WLAN parameters
#define WLAN_SSID       "Dolphin"
#define WLAN_PASS       "BaDo2206@"
#define WLAN_SECURITY   WLAN_SEC_WPA2

uint32_t ip = 0;

void setup() {
  Serial.begin(115200);
  
  if (!cc3000.begin()) {
    while(1); 
  }
  
  Serial.println("Connecting to AP");
  cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY);
  
  Serial.println("Checking DHCP");
  while (!cc3000.checkDHCP()) {
    delay(100); 
  }
  
  Serial.println(F("Connected to AP"));
  
  Serial.println(F("Resolving IP"));
  while  (ip  ==  0)  {
    if  (!cc3000.getHostByName("128.199.246.241", &ip))  {
      Serial.println(F("Couldn't resolve!"));
      while(1){}
    }
    delay(500);
  }  
  cc3000.printIPdotsRev(ip);
  Serial.println(F(""));
}


void loop() {
  // Enable watchdog
  wdt_enable(WDTO_8S);
  
  /*
  char temp[5], humi[5]; 
  dtostrf(sht25.GetHumidity(),5,2,humi);
//  Serial.println(humi);
  dtostrf(sht25.GetTemperatureC(),5,2,temp);
//  Serial.println(temp);
  */
  
  int temp = (int)sht25.GetTemperatureC();
  int humi = (int)sht25.GetHumidity();
  
  String data = "";
  data = data + "\n" + "{\"temperature\": \"" + String(temp) + "\", \"humidity\": \"" + String(humi) + "\"}";
  Serial.println(data);
  
  int length = 0;
  length = data.length();
  
  // Reset watchdog
  wdt_reset();
  
  Serial.println("Connecting to server");
  Adafruit_CC3000_Client client = cc3000.connectTCP(ip, 3000);
  if (client.connected()) {
    Serial.println(F("Host connected"));
    // send header
    client.fastrprintln(F("POST /api/sensors HTTP/1.1"));
    Serial.print(F("."));
    client.fastrprintln(F("Host: 128.199.246.241"));
    Serial.print(F("."));
    client.fastrprintln(F("Content-Type: application/json"));
    Serial.println(F("."));
    client.fastrprint(F("Content-Length: "));
    client.println(length);
    Serial.print(F("."));
    client.fastrprintln(F("Connection: close"));
    Serial.println(F("Header sent"));
    
    // Reset watchdog
    wdt_reset();
    
    // send data
    int buffer_size = 20;
    client.fastrprintln(F(""));    
    sendData(client,data,buffer_size);  
    client.fastrprintln(F(""));
    Serial.println(F("Data sent"));
    // Reset watchdog
    wdt_reset();
  } else {
    Serial.println(F("Host connection failed"));    
    return;
  }
  
  // Reset watchdog
  wdt_reset();
    
  Serial.println(F("Respond: "));
  while (client.connected()) {
    while (client.available()) {
      char c = client.read();
      Serial.print(c);
    }
  }
  
  // Reset watchdog
  wdt_reset();
  
  // Close connection and disconnect
  client.close();
  
  // Reset watchdog & disable
  wdt_reset();
  wdt_disable();
  
  // Wait 5 seconds until next update
  wait(10000);
}

void sendData(Adafruit_CC3000_Client& client, String input, int chunkSize) {
  
  // Get String length
  int length = input.length();
  int max_iteration = (int)(length/chunkSize);
  
  for (int i = 0; i < length; i++) {
    client.print(input.substring(i*chunkSize, (i+1)*chunkSize));
    wdt_reset();
  }  
}

void wait(int total_delay) {
  
  int number_steps = (int)(total_delay/5000);
  wdt_enable(WDTO_8S);
  for (int i = 0; i < number_steps; i++){
    delay(5000);
    wdt_reset();
  }
  wdt_disable();
}
