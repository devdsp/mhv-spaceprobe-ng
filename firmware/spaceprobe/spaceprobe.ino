#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xE3 };
IPAddress server(10, 42, 0, 1);
EthernetClient ethClient;
PubSubClient client(ethClient);

float led_period = 0;
unsigned long led_flashing = 0;
unsigned char led_colour[] = {0,0,0};

const unsigned char led_r = 6;
const unsigned char led_g = 5;
const unsigned char led_b = 3;

int last_pot = 0;
int last_pot_push = 0; 
unsigned long pot;

unsigned long last_move = 0;
unsigned long last_send = 0;


void callback(char* topic, byte* payload, unsigned int length) {

  if(strcmp(topic,"spaceprobe/led")==0 && length == 3) {
    analogWrite(led_r,payload[0]);
    analogWrite(led_g,payload[1]);
    analogWrite(led_b,payload[2]);
    led_period = 0;
  }
  
  if(strcmp(topic,"spaceprobe/led")==0 && length == 8) {
    memcpy(led_colour,payload,3);
    memcpy(&led_period,payload+4,4);\
    led_period *= 1000;
  }
  
  if(strcmp(topic,"spaceprobe/tone")==0 && length == 2) {
    tone(8,payload[0],payload[1]);
  }

  if(strcmp(topic,"spaceprobe/volts")==0 && length == 1 ) {
    analogWrite(9,payload[0]);
  }
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("SpaceProbe")) {
      Serial.println("connected");
      client.subscribe("spaceprobe/led");
      client.subscribe("spaceprobe/tone");
      client.subscribe("spaceprobe/volts");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  delay(50);
  Serial.begin(57600);
  Serial.println("Starting The Space Probe");
  while (Ethernet.begin(mac) != 1)
  {
    Serial.println("Waiting on DHCP");
  }
  Serial.println("Have IP");
  client.setServer(server, 1883);
  client.setCallback(callback);
  // Allow the hardware to sort itself out
  delay(1500);
}


void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();
  
  if( led_period ) {
    if( millis() % (int)led_period >= led_period /2 ) {
      analogWrite(led_r,led_colour[0]);
      analogWrite(led_g,led_colour[1]);
      analogWrite(led_b,led_colour[2]);
    } else {
      analogWrite(led_r,0);
      analogWrite(led_g,0);
      analogWrite(led_b,0);
    }
  }
  
  pot = ((pot * 9) + analogRead(1)) / 10;
  
  if (abs(last_pot - pot)) {
    last_pot = pot;
    last_move = millis();
  } else if( millis() > last_move + 500) {
    if(abs(last_pot_push - pot) > 100.0/(last_send - millis()) +5 ) {
    last_pot_push = pot;
    last_send = millis();
    Serial.println(pot);
    client.publish("spaceprobe/knob",(uint8_t*)&pot,2);
    }
  }
  
}

