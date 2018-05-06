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

float last_pot_push = 0;

float i=0;
int last_pot_error = 0;

unsigned long framestart = 0;

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


float pot = 0;
const int POT_SAMPLES=10;

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
  pot = last_pot_push = analogRead(1);
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
  
  
  float dt = float(millis() - framestart)/1000.0;
  if( dt >= 0.05) {
    framestart = millis();

    pot = (pot * (POT_SAMPLES-1) + analogRead(1)) / POT_SAMPLES;
    float p = pot - last_pot_push;
    i += p*dt;
    float d = double(p - last_pot_error)/dt;
    last_pot_error = p;
    
    last_pot_push += p*.0005 + i*.0001 + d*.0001;
    
    if( abs(p) > 5 && abs(i) > 10 && abs(d) < 10 ) {
      last_pot_push = pot = analogRead(1);
      i = last_pot_error = 0;
      Serial.print("pushing: ");
      Serial.println(pot);
      uint16_t buf = (uint16_t)pot;
      client.publish("spaceprobe/knob",(uint8_t*)&buf,2);
    }
  }
  
}

