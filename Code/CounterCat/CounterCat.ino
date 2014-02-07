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


//These are on the "analog" arduino ports
#define BUTTON_PIN   A0 //Port C0
#define BLENDER_PIN  A1 //Port C1
#define SENSOR_1_PIN A2 //Port C2
#define SENSOR_2_PIN A3 //Port C3

//The LEDs
#define RED_LED_PIN  A4 //Port C4
#define GRN_LED_PIN  A5 //Port C5
#define BLU_LED_PIN 2 //Port D2
#define YLW_LED_PIN 3 //Port D3


#define COOLDOWN_LED_PIN BLU_LED_PIN
#define WARMUP_LED_PIN YLW_LED_PIN
#define ARMED_LED_PIN RED_LED_PIN
#define PAUSED_LED_PIN GRN_LED_PIN

//The Decoder - 74HC238
#define DECODER_LINE0 4 //Port D4
#define DECODER_LINE1 5 //Port D5
#define DECODER_LINE2 6 //Port D6
#define DECODER_ENABLE 7 //Port D7


//Note that the PIR sensor needs at least 40 seconds to start up. 
#define WARM_UP_TIME_MS ((unsigned long) 41000)
#define COOLDOWN_TIME_MS 5000
#define ACTIVE_TIME_MS 2000
#define WARM_UP_FLICKER 200
#define ACTIVE_FLICKER 100
#define COOLDOWN_FLICKER  200
#define BUTTON_DEBOUNCE 500

#define EYE_SCANNING_RATE 10 

#define USE_PIR2 true

#define BIT0HI 0x01
#define BIT1HI 0x02
#define BIT2HI 0x04
#define BIT3HI 0x08
#define BIT4HI 0x0F
#define BIT5HI 0x20
#define BIT6HI 0x40
#define BIT7HI 0x80



//These are the states for our state machine. 
enum State
{
  S_WARMING_UP,
  S_ARMED,
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
  //Since this controls the relay, best to set it low first
  pinMode(BLENDER_PIN, OUTPUT);
  digitalWrite(BLENDER_PIN, LOW);
  
  //All the inputs
  pinMode(SENSOR_1_PIN, INPUT);  
  pinMode(SENSOR_2_PIN, INPUT);  
  pinMode(BUTTON_PIN, INPUT);

  //Set all the LEDs as outputs, and let low to start
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);  
  
  pinMode(GRN_LED_PIN, OUTPUT);
  digitalWrite(GRN_LED_PIN, LOW);  
  
  pinMode(YLW_LED_PIN, OUTPUT);
  digitalWrite(YLW_LED_PIN, LOW);  
  
  pinMode(BLU_LED_PIN, OUTPUT);
  digitalWrite(BLU_LED_PIN, LOW);  
  
  pinMode(DECODER_LINE0,OUTPUT);
  pinMode(DECODER_LINE1,OUTPUT);
  pinMode(DECODER_LINE2,OUTPUT);
  
  pinMode(DECODER_ENABLE, OUTPUT);
  digitalWrite(DECODER_ENABLE, LOW);
  
  Serial.begin(9600);
  Serial.println("Starting CounterCat3...");
  if (USE_PIR2)
  {
    Serial.println("Using PIR2 as safety.");
  } 
  else
  {
    Serial.println("Ignoring PIR2.");
  }
  
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

  //Step the scanning eye.
  if (now % EYE_SCANNING_RATE == 0)
  {
      stepScanningEye();
   } 
  
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
  if (USE_PIR2)
  {
    int safetyNow = digitalRead(SENSOR_2_PIN);
    if (safetyNow != safetyThen && safetyNow)
    {
      //Sensor rising edge!
      Serial.println("Safety rising edge.");
      safetyRising = true;
    }  
    safetyThen = safetyNow;
  }


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
        digitalWrite(WARMUP_LED_PIN, LOW);
        wasHigh = false;
      } 
      else
      {
        digitalWrite(WARMUP_LED_PIN, HIGH); 
        wasHigh = true;
      }
    }

    if (now > (timerStart + WARM_UP_TIME_MS))
    {
      Serial.println("Warmed up!");
      currentState = S_ARMED;
      digitalWrite(WARMUP_LED_PIN,LOW); //make sure this pin goes off
    }
    break;


  case S_ARMED:

    //Leave the red light on while we're armed
    turnOffAllLEDs();
    digitalWrite(ARMED_LED_PIN, HIGH);

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
        turnOffAllLEDs();
        wasHigh = false;
      } 
      else
      {
        digitalWrite(RED_LED_PIN, HIGH); 
        digitalWrite(GRN_LED_PIN, HIGH);
        digitalWrite(YLW_LED_PIN, HIGH); 
        digitalWrite(BLU_LED_PIN, HIGH);
        wasHigh = true; 
      }
    }


    if (now > (timerStart + ACTIVE_TIME_MS))
    {
      Serial.println("Done being active!");
      digitalWrite(BLENDER_PIN, LOW);
      timerStart = now;
      currentState = S_COOLDOWN; 
      turnOffAllLEDs();
      
    }
    break;

  case S_COOLDOWN:

    turnOffAllLEDs();
    
    if (now % COOLDOWN_FLICKER == 0)
    {
      if (wasHigh)
      {
        digitalWrite(COOLDOWN_LED_PIN, LOW);
        wasHigh = false;
      }
      else
      {
        digitalWrite(COOLDOWN_LED_PIN, HIGH);
        wasHigh = true; 
      }

    }   

    if (now > (timerStart + COOLDOWN_TIME_MS))
    {
      Serial.println("Done cooling down.");
      currentState = S_ARMED;  
    }
    break;

  case S_PAUSED:

   turnOffAllLEDs();
   digitalWrite(PAUSED_LED_PIN, HIGH);
   
    if (now > (timerStart + BUTTON_DEBOUNCE))
    {
      if (buttonPushed)
      {
        Serial.println("Unpaused!");
        currentState = S_ARMED;          
        turnOffAllLEDs();
      }
    }
    break;
  }

  delay(1);

}




void turnOffAllLEDs()
{
     digitalWrite(RED_LED_PIN, LOW);
     digitalWrite(GRN_LED_PIN, LOW);
     digitalWrite(YLW_LED_PIN, LOW);
     digitalWrite(BLU_LED_PIN, LOW); 
}

//Turn on the selected output Y0-Y7. Outoff bounds entry turns all off. 
void setDecoderLED(int i)
{
  if ( i >= 0 && i < 8)
  {
    digitalWrite(DECODER_ENABLE, HIGH);
    digitalWrite(DECODER_LINE0, i & BIT0HI);
    digitalWrite(DECODER_LINE1, i & BIT1HI);
    digitalWrite(DECODER_LINE2, i & BIT2HI);
 
  }
  else
  {
    digitalWrite(DECODER_ENABLE, LOW); 
  }
  
}


void stepScanningEye()
{
 static char eyeIndex = 0;
 setDecoderLED(eyeIndex++); 
}
