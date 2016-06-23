/*
 * PhoneyTV v3.1.1
 *
 * This Sketch illuminates 6 sets of LED's in a random fashion as to mimic the
 * light eminating from a television.  It is intended to make an empty home,
 * or an empty section of a home, appear to be occupied by someone watching
 * TV.  As an alternative to a real television left on, this uses less than 1%
 * of the electrical energy.
 *
 * With the use of the MySensors plugin and gateway, PhoneyTV is intended to
 * be used with a controller (e.g. Vera or Raspberry PI).
 *
 * Sketch does not use any delays to create the random blinking as a way to
 * assure that communication back to the gateway is as unaffected as possible.
 *
 * You can adjust the length of the blink interval and its "twitchyness" by
 * modifying the random number generators, if you prefer more/less 'motion' in
 * in your unit.  The lines are highlighted in the code, play around to create the
 * random effect you like.
 *
 * Sketch takes advantage of available PWM on pins 3, 5 & 6 using the white/blue LEDs
 * to allow fluctuations in the intensity of the light, enhancing the PhoneyTV's
 * realistic light effects.
 *
 * Created 12-APR-2014
 * Free for distrubution
 * Credit should be given to MySensors.org for their base code for relay control
 * and for the radio configuration.  Thanks Guys.
 *
 * 29-May-2014
 * Version 2:  Simplified the code, removing all redundant relay setup from original
 * code.  Added an on/off momentary pushputton option to be set up on pin 2.  Inproved
 * the dark dips for longer duration (can be configured) at intervals.
 *
 * 6-Jun-2015
 * Version 3.1
 * Updated for MySensors V1.4.1
 * Contributed by Jim (BulldogLowell@gmail.com) Inspired by Josh >> Deltanu1142@gmail.com
 *
 * How to video: https://youtu.be/p37qnl8Kjfc
 */
//
// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enabled repeater feature for this node
//#define MY_REPEATER_FEATURE

#define MY_NODE_ID 13

#include <SPI.h>
#include <MyConfig.h>
#include <MySensors.h>
#include <Bounce2.h>

//
#define RADIO_RESET_DELAY_TIME 20
//
#define BUTTON_PIN  2  // Arduino Digital I/O pin number for button 
#define CHILD_ID 1   // 
#define RADIO_ID 5  //AUTO
//
#ifdef MY_DEBUG
#define DEBUG_PRINT(x)   Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

//
MyMessage msg(CHILD_ID, V_LIGHT);

//
byte RGB_PINS[6] = {3, 6, 5};
// totaal: 
// red   3 = (3) + (4) + (8)
// green 3 = (3) + (7) + (8)
// blue  4 = (3) + (5) + (6) + (8)
//
Bounce debouncer = Bounce();
byte oldValue = 0;
boolean active = false;
boolean oldActive = false;
int dipInterval = 10;
int darkTime = 250;
unsigned long currentDipTime;
unsigned long dipStartTime;
unsigned long currentMillis;
unsigned long previousMillis = 0UL;
unsigned long interval = 2000UL;
int twitch = 50;
int dipCount = 0;
int analogLevel = 100;
boolean timeToDip = false;
boolean gotAck=false;
//

void setup()
{
  // Set the PWM pins for Red, Green and Blue to output mode.
  for (byte color = 0; color < 3; color++)
  {
    pinMode(RGB_PINS[color], OUTPUT);
  }
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // Initialize the debouncer for the button
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(50);
}

void presentation()  
{  
  //Send the sensor node sketch version information to the gateway
  sendSketchInfo("PhonyTV", __DATE__);
  present(CHILD_ID, S_LIGHT);
  // Send the current state to the controller
  send(msg.set(active), true);
}


//
void loop()
{
  debouncer.update();
  byte value = debouncer.read();
  if (value != oldValue && value == 0)
  {
    active = !active;
    while(!gotAck)
    {
      send(msg.set(active), true);
      wait(RADIO_RESET_DELAY_TIME);
    }
    gotAck = false;
    DEBUG_PRINT(F("State changed to: PhoneyTV "));
    DEBUG_PRINTLN(active? F("ON") : F("OFF"));
  }
  oldValue = value;
  
  if (active)
  {
    if (timeToDip == false)
    {
      currentMillis = millis();
      if (currentMillis - previousMillis > interval)
      {
        previousMillis = currentMillis;
        interval = random(750, 4001); //Adjusts the interval for more/less frequent random light changes
        twitch = random(40, 100); // Twitch provides motion effect but can be a bit much if too high
        dipCount = dipCount++;
      }
      if (currentMillis - previousMillis < twitch)
      {
        byte color = random(0, 3);
        switch (color) //for the three PWM pins
        {
          case 0: // RED
            analogWrite(RGB_PINS[color], random(0, 3 * 255 / 4));
            break;
          case 1: // GREEN
            analogWrite(RGB_PINS[color], random(0, 3 * 255 / 4));
            break;
          case 2: // BLUE
            analogWrite(RGB_PINS[color], random(0, 255));
            break;
        }
        if (dipCount > dipInterval)
        {
          timeToDip = true;
          dipCount = 0;
          dipStartTime = millis();
          darkTime = random(50, 150);
          dipInterval = random(5, 250); // cycles of flicker
        }
      }
    }
    else
    {
      DEBUG_PRINTLN(F("Dip Time"));
      currentDipTime = millis();
      if (currentDipTime - dipStartTime < darkTime)
      {
        for (int i = 0; i < 3; i++)
        {
          digitalWrite(RGB_PINS[i], LOW);
        }
      }
      else
      {
        timeToDip = false;
      }
    }
  }
  else
  {
    if (active != oldActive)
    {
      for (int i = 0; i < 3; i++)
      {
        digitalWrite(RGB_PINS[i], LOW);
      }
    }
  }
  oldActive = active;
}

//
void receive(const MyMessage & message)
{
  if (message.isAck())
  {
    DEBUG_PRINTLN(F("This is an ack from gateway"));
    gotAck = true;
  }
  if (message.type == V_LIGHT)
  {
    active = message.getBool();
    DEBUG_PRINT(F("Incoming change for sensor... New State = "));
    DEBUG_PRINTLN(active ? F("ON") : F("OFF"));
  }
}


