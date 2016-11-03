#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>

byte mac[]    = {  0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xE3 };
IPAddress server(10, 42, 0, 1);
EthernetClient ethClient;
PubSubClient client(ethClient);

const unsigned char led_r = 6;
const unsigned char led_g = 5;
const unsigned char led_b = 3;

int last_pot = 0;
int last_pot_push = 0; 
unsigned long pot;

unsigned long last_move = 0;
unsigned long last_send = 0;

struct led_colour {
  unsigned char r;
  unsigned char g;
  unsigned char b;
} led_colour;

struct led_blink {
  struct led_colour colour;
  float frequency;
  unsigned long duration;
} led_blink;

struct buzz {
  unsigned int frequency;
  unsigned long duration;
};

struct volts {
  unsigned char pwm;
};

void callback(char* topic, byte* payload, unsigned int length) {
  if(strcmp(topic,"spaceprobe/led")==0 && length == sizeof(led_colour)) {
    memcpy(&led_colour,payload,sizeof(led_colour));
  }
  
  if(strcmp(topic,"spaceprobe/led")==0 && length == sizeof(led_blink)) {
    memcpy(&led_blink,payload,sizeof(led_blink));
    led_blink.duration += millis();
  }
  
  if(strcmp(topic,"spaceprobe/tone")==0 && length == sizeof(buzz)) {
    struct buzz buzz;
    memcpy(&buzz,payload,sizeof(buzz));
    tone(8,buzz.frequency,buzz.duration);
  }

  if(strcmp(topic,"spaceprobe/volts")==0 && length == sizeof(volts) ) {
    struct volts volts;
    memcpy(&volts,payload,sizeof(volts));
    analogWrite(9,volts.pwm);
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
  memset(&led_blink,0,sizeof(led_blink));
  // Allow the hardware to sort itself out
  delay(1500);
}


void loop()
{
  if (!client.connected()) {
    reconnect();
  }
  
  client.loop();
  
  if( led_blink.duration > millis() ) {
    if( millis() % (int)(led_blink.frequency *1000) >= led_blink.frequency * 500 ) {
      analogWrite(led_r,led_blink.colour.r);
      analogWrite(led_g,led_blink.colour.g);
      analogWrite(led_b,led_blink.colour.b);
    } else {
      analogWrite(led_r,0);
      analogWrite(led_g,0);
      analogWrite(led_b,0);
    }
  } else {
    analogWrite(led_r,led_colour.r);
    analogWrite(led_g,led_colour.g);
    analogWrite(led_b,led_colour.b);
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

