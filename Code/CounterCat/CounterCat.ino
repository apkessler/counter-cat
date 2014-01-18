/////////////////////////////////////////////
// CounterCat                          
//
// System Components:
//   - 1x Parallax Passive Infrared Sensor
//        http://parallax.com/product/555-28027
//
//   - 1x Beefcake Relay Board
//        https://www.sparkfun.com/products/11042      
//
//      uC   |  PIR
//    -------|------
//     D2 <<---- Out
//     +5v|----- Vcc 
//     GND|----- Gnd    
//
//     uC    |  Relay
//    -------|---------
//     +5v|----- Vcc 
//     GND|----- Gnd 
//     D3<<----- Cntrl
//     
/////////////////////////////////////////////


#define SENSOR_1_PIN   13
#define SENSOR_2_PIN   7
#define BLENDER_PIN    12
#define GREEN_LED_PIN  5
#define RED_LED_PIN    4
#define BUTTON_PIN     2

//The DIP switches!
#define SWITCH_1_PIN  11
#define SWITCH_2_PIN  10
#define SWITCH_3_PIN  9
#define SWITCH_4_PIN  8


//Note that the PIR sensor needs at least 40 seconds to start up. 
#define WARM_UP_TIME_MS ((unsigned long) 41000)
#define COOLDOWN_TIME_MS 5000
#define ACTIVE_TIME_MS 2000
#define WARM_UP_FLICKER 200
#define ACTIVE_FLICKER 100
#define COOLDOWN_FLICKER  200
#define BUTTON_DEBOUNCE 500

#define USE_PIR2 true

//These are the states for our state machine. 
enum State
{
  S_WARMING_UP,
  S_IDLE,
  S_ACTIVE,
  S_COOLDOWN,
  S_PAUSED,
};



///MODULE VARIABLES////////////////////////////////////////////////////////
State currentState;

String state2String[] =
{
  "warming up",
  "idle",
  "active",
  "cooldown",
  "paused"
};


unsigned long timerStart = 0;
unsigned long now = 0;

int buttonThen;
int sensorThen;
int safetyThen;
boolean wasHigh = false;

///SETUP//////////////////////////////////////////////////////////////////
void setup(void)
{

  pinMode(BLENDER_PIN, OUTPUT);
  pinMode(SENSOR_1_PIN, INPUT);  
  pinMode(SENSOR_2_PIN, INPUT);  
  pinMode(BUTTON_PIN, INPUT);
  pinMode(GREEN_LED_PIN, OUTPUT);
  pinMode(RED_LED_PIN, OUTPUT);
  
  pinMode(SWITCH_1_PIN, INPUT);
  pinMode(SWITCH_2_PIN, INPUT);
  pinMode(SWITCH_3_PIN, INPUT);
  pinMode(SWITCH_4_PIN, INPUT);

  digitalWrite(BLENDER_PIN, LOW);
  digitalWrite(GREEN_LED_PIN, LOW);
  digitalWrite(RED_LED_PIN, LOW);

  Serial.begin(9600);
  Serial.println("Starting CounterCat2...");

  Serial.println("Warming up...");
  timerStart = millis();

  currentState = S_WARMING_UP;
  buttonThen = 0;
  sensorThen = 0;
  safetyThen =0;
}


///LOOP/////////////////////////////////////////////////////////////////////////
void loop(void)
{
  now = millis();

  /*
   * Check the state of the button.
   */
  boolean buttonPushed = false;
  int buttonNow = digitalRead(BUTTON_PIN);
  if (buttonNow != buttonThen && buttonNow)
  {
    //Button pushed!
     buttonPushed = true;
  }
  buttonThen = buttonNow;
  
  
  /*
   * Check for a rising edge on the sensor.
   */
  boolean sensorRising = false;
  int sensorNow = digitalRead(SENSOR_1_PIN);
  if (sensorNow != sensorThen && sensorNow)
  {
    //Sensor rising edge!
    sensorRising = true;
  }  
  sensorThen = sensorNow;
  
   /*
   * Check for a rising edge on the safety sensor.
   */
  boolean safetyRising = false;
  int safetyNow = digitalRead(SENSOR_2_PIN);
  if (safetyNow != safetyThen && safetyNow)
  {
    //Sensor rising edge!
    Serial.println("Safety rising edge.");
    safetyRising = true;
  }  
  safetyThen = safetyNow;


  /*
   * Act based on current state.
   */
  switch (currentState)
  {

  case S_WARMING_UP:
    if (now % 1000 == 0)
    {
      Serial.println(String(now*100/WARM_UP_TIME_MS, DEC) + "%");
    }

    if (now % WARM_UP_FLICKER == 0)
    {
      if (wasHigh)
      {
        digitalWrite(RED_LED_PIN, LOW);
        wasHigh = false;
      } 
      else
      {
        digitalWrite(RED_LED_PIN, HIGH); 
        wasHigh = true;
      }
    }

    if (now > (timerStart + WARM_UP_TIME_MS))
    {
      Serial.println("Warmed up!");
      currentState = S_IDLE;
    }
    break;


  case S_IDLE:

    //Leave the red light on while we're armed
    digitalWrite(RED_LED_PIN, HIGH);
    digitalWrite(GREEN_LED_PIN, LOW);

    //If there's a rising edge on the sensor pin
    if (sensorRising)
    {
      Serial.println("Movement!");
      currentState = S_ACTIVE; 
      digitalWrite(BLENDER_PIN, HIGH);
      timerStart = now; 
    }
    else if (buttonPushed || (safetyRising && USE_PIR2))
    {
      Serial.println("Pausing...");
      currentState = S_PAUSED;
      timerStart = now;
    }
    break;

  case S_ACTIVE:

    if (now % ACTIVE_FLICKER == 0)
    {
      if (wasHigh)
      {
        digitalWrite(RED_LED_PIN, LOW);
        digitalWrite(GREEN_LED_PIN, LOW);
        wasHigh = false;
      } 
      else
      {
        digitalWrite(RED_LED_PIN, HIGH); 
        digitalWrite(GREEN_LED_PIN, HIGH);
        wasHigh = true; 
      }
    }


    if (now > (timerStart + ACTIVE_TIME_MS))
    {
      Serial.println("Done being active!");
      digitalWrite(BLENDER_PIN, LOW);
      timerStart = now;
      currentState = S_COOLDOWN; 
    }
    break;

  case S_COOLDOWN:

    digitalWrite(RED_LED_PIN, LOW);
    
    if (now % COOLDOWN_FLICKER == 0)
    {
      if (wasHigh)
      {
        digitalWrite(GREEN_LED_PIN, LOW);
        wasHigh = false;
      }
      else
      {
        digitalWrite(GREEN_LED_PIN, HIGH);
        wasHigh = true; 
      }

    }   

    if (now > (timerStart + COOLDOWN_TIME_MS))
    {
      Serial.println("Done cooling down.");
      currentState = S_IDLE;  
    }
    break;

  case S_PAUSED:

    digitalWrite(GREEN_LED_PIN, HIGH);
    digitalWrite(RED_LED_PIN, LOW);
    digitalWrite(BLENDER_PIN, LOW);
    
    if (now > (timerStart + BUTTON_DEBOUNCE))
    {
      if (buttonPushed)
      {
        Serial.println("Unpaused!");
        currentState = S_IDLE;          
      }
    }
    break;
  }

  delay(1);

}

