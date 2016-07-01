
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
//#define MY_DEBUG 
//#define NO_PHONY
#define NO_BUTTON
//#define NO_DHT

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enabled repeater feature for this node
#define MY_REPEATER_FEATURE

#define MY_NODE_ID 13

#include <SPI.h>
#include <MyConfig.h>
#include <MySensors.h>
#ifndef NO_PHONY
  #include <Bounce2.h>
#endif
#ifndef NO_DTH
  #include <DHT.h>
#endif


//
#define RADIO_RESET_DELAY_TIME 20
//
#ifndef NO_PHONY
  byte RGB_PINS[3] = {3, 5, 6};
  #ifndef NO_BUTTON
    #define BUTTON_PIN  2  // Arduino Digital I/O pin number for button 
  #endif
  #define CHILD_ID_PHONY 1   // 
#endif
#ifndef NO_DHT
  #define CHILD_ID_HUM 2
  #define CHILD_ID_TEMP 3
  #define CHILD_ID_DIMMER 4
  //
  // DHT_REPEAT_INTERVAL is the interval for reporting the same temperature/
  // humidity value to the gateway. The value is reported when changes or
  // every DHT_REPEAT_INTERVAL milliseconds.
  #define DHT_REPEAT_INTERVAL 900000 // 15 minutes
  //
  // DHT_INTERVAL is the interval for temperature/humidity measurements
  // Every DHT_INTERVAL milliseconds either temperature or humidity is
  // read from the DHT sensor.
  #define DHT_INTERVAL         6000 // 1 minute
#endif

//
#define DEBUG
#ifdef DEBUG
#define DEBUG_PRINT(x)   Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINT2(x, y) Serial.print(x, y)
#define DEBUG_WRITE(x)   Serial.write(x)
#define DEBUG_BEGIN      Serial.begin(115200)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINT2(x, y)
#define DEBUG_WRITE
#define DEBUG_BEGIN
#endif

//
#ifndef NO_PHONY
  MyMessage msg(CHILD_ID_PHONY, V_LIGHT);
#endif

//
//
#define DHT_PIN 4
// totaal: 
// red   3 = (3) + (4) + (8)
// green 3 = (3) + (7) + (8)
// blue  4 = (3) + (5) + (6) + (8)
//
#ifndef NO_PHONY
  #ifndef NO_BUTTON
    Bounce debouncer = Bounce();
  #endif
  byte oldValue = 0;
  boolean active = false;
  boolean oldActive = false;
  int dipInterval = 10;
  int darkTime = 250;
  unsigned long currentDipTime;
  unsigned long dipStartTime;
  unsigned long previousMillis = 0UL;
  unsigned long interval = 2000UL;
  int twitch = 50;
  int dipCount = 0;
  int analogLevel = 100;
  boolean timeToDip = false;
  boolean gotAck=false;
#endif
//
#ifndef NO_DHT
  DHT dht;
  float lastTemp;
  unsigned long lastTempMillis = 0UL;
  float lastHum;
  unsigned long lastHumMillis = 0UL;
  unsigned long dhtMillis = 0UL;
  boolean tempNext = true;
#endif

void setup()
{
  DEBUG_BEGIN;
  #ifndef NO_PHONY
    // Set the PWM pins for Red, Green and Blue to output mode.
    for (byte color = 0; color < 3; color++)
    {
      pinMode(RGB_PINS[color], OUTPUT);
    }
    for (byte color = 0; color < 3; color++)
    {
      digitalWrite(RGB_PINS[color], HIGH);
      wait(1000);
      digitalWrite(RGB_PINS[color], LOW);
    }
    wait(1000);
    for (byte color = 0; color < 3; color++)
    {
      analogWrite(RGB_PINS[color], HIGH);
      wait(1000);
      analogWrite(RGB_PINS[color], LOW);
    }
    #ifndef NO_BUTTON
      pinMode(BUTTON_PIN, INPUT_PULLUP);
      // Initialize the debouncer for the button
      debouncer.attach(BUTTON_PIN);
      debouncer.interval(50);
    #endif
  #endif
  #ifndef NO_DHT
    //
    dht.setup(DHT_PIN/*, DHT::DHT11*/); 
    dhtMillis = millis();
  #endif
  
}

void presentation()  
{  
  //Send the sensor node sketch version information to the gateway
  sendSketchInfo("PhonyTV+DHT", __DATE__);
  #ifndef NO_PHONY
    present(CHILD_ID_PHONY, S_LIGHT);
  #endif
  #ifndef NO_DHT
    present(CHILD_ID_HUM, S_HUM);
    present(CHILD_ID_TEMP, S_TEMP);
  #endif
  #ifndef NO_PHONY
    // Send the current state to the controller
    send(msg.set(active), true);
  #endif
}

void before()
{
  
}
//
void loop()
{
  unsigned long currentMillis = millis();
  #ifndef NO_PHONY
    #ifndef NO_BUTTON
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
    #endif
    
    if (active)
    {
      if (timeToDip == false)
      {
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
              analogWrite(RGB_PINS[color], random(128, 3 * 255 / 4));
              break;
            case 1: // GREEN
              analogWrite(RGB_PINS[color], random(128, 3 * 255 / 4));
              break;
            case 2: // BLUE
              analogWrite(RGB_PINS[color], random(128, 255));
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
  #endif
  #ifndef NO_DHT
    if (currentMillis - dhtMillis > DHT_INTERVAL)
    {
      if (tempNext)
      {
        DEBUG_PRINT(F("DHT reading temperature - "));
        float temperature = dht.getTemperature();
        DEBUG_PRINT(dht.getStatusString());
        if (dht.getStatus() == DHT::ERROR_NONE)
        {
          DEBUG_PRINT(F(": "));
          DEBUG_PRINT2(temperature, dht.getNumberOfDecimalsTemperature());
          DEBUG_WRITE(176);
          DEBUG_PRINT(F("C"));
          bool doSend(false);
          if (isnan(temperature)) 
          {
            //DEBUG_PRINTLN(F("failed!"));
          }
          else if (temperature != lastTemp) 
          {
            lastTemp = temperature;
            doSend = true;
          }
          else
          {
            DEBUG_PRINT(" (no change)");
            if (currentMillis - lastTempMillis > DHT_REPEAT_INTERVAL)
            {
              doSend = true;
            }
          }
          if (doSend)
          {
            MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
            send(msgTemp.set(temperature, 1));
            lastTempMillis = currentMillis;
          }
        }
        DEBUG_PRINTLN(F(""));
      }
      else
      {
        DEBUG_PRINT(F("DHT reading humidity - "));
        float humidity = dht.getHumidity();
        DEBUG_PRINT(dht.getStatusString());
        if (dht.getStatus() == DHT::ERROR_NONE)
        {
          DEBUG_PRINT(F(": "));
          DEBUG_PRINT2(humidity, dht.getNumberOfDecimalsHumidity());
          DEBUG_PRINT(F("%"));
          bool doSend(false);
          if (isnan(humidity)) 
          {
            DEBUG_PRINTLN(F("failed!"));
          }
          else if (humidity != lastHum) 
          {
            lastHum = humidity;
            doSend = true;
          }
          else
          {
            DEBUG_PRINT(" (no change)");
            if (currentMillis - lastHumMillis > DHT_REPEAT_INTERVAL)
            {
              doSend = true;            
            }
          }
          if (doSend)
          {
            MyMessage msgHum(CHILD_ID_HUM, V_HUM);
            send(msgHum.set(humidity, 1));
            lastHumMillis = currentMillis;
          }
          else
          {
            sendHeartbeat();
          }
        }
        DEBUG_PRINTLN(F(""));
      }
      tempNext = !tempNext;
      dhtMillis = currentMillis;
    }
  #endif
}

//
void receive(const MyMessage & message)
{
  #ifndef NO_PHONY
    if (message.isAck())
    {
      DEBUG_PRINTLN(F("Receive - ack (from gateway)"));
      gotAck = true;
    }
    if (message.type == V_LIGHT)
    {
      active = message.getBool();
      DEBUG_PRINT(F("Receive - change for sensor: state="));
      DEBUG_PRINTLN(active ? F("ON") : F("OFF"));
    }
  #endif
}


