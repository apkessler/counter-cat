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

#define BLENDER_PIN 3
#define SENSOR_PIN 2

//Note that the PIR sensor needs at least 40 seconds to start up. 
#define WARM_UP_TIME_MS ((unsigned long) 41000)
#define COOLDOWN_TIME_MS 5000
#define ACTIVE_TIME_MS 2000

enum State
{
 S_WARMING_UP,
 S_IDLE,
 S_ACTIVE,
 S_COOLDOWN,
};

State currentState;

String state2String[] =
{
 "warming up",
 "idle",
 "active",
 "cooldown"
};


unsigned long timerStart = 0;
unsigned long now = 0;

void setup(void)
{
  
    pinMode(BLENDER_PIN, OUTPUT);
    pinMode(SENSOR_PIN, INPUT);  
  
    digitalWrite(BLENDER_PIN, LOW);
  
    Serial.begin(9600);
    Serial.println("Starting CounterCat...");
     
    Serial.println("Warming up...");
    timerStart = millis();
    
    currentState = S_WARMING_UP;
}

void loop(void)
{
  now = millis();
  
   switch (currentState)
    {
  
    case S_WARMING_UP:
        if (now % 1000 == 0)
        {
          Serial.println(String(now*100/WARM_UP_TIME_MS, DEC) + "%");
        }
        
        if (now > (timerStart + WARM_UP_TIME_MS))
        {
            Serial.println("Warmed up!");
            currentState = S_IDLE;
        }
        break;
   
  
    case S_IDLE:
          if (digitalRead(SENSOR_PIN))
          {
              Serial.println("Movement!");
              currentState = S_ACTIVE; 
              digitalWrite(BLENDER_PIN, HIGH);
              timerStart = now; 
          }
        break;
     
   case S_ACTIVE:
    
        if (now > (timerStart + ACTIVE_TIME_MS))
        {
            Serial.println("Done being active!");
            digitalWrite(BLENDER_PIN, LOW);
            timerStart = now;
            currentState = S_COOLDOWN; 
        }
        break;
     
   case S_COOLDOWN:
        if (now > (timerStart + COOLDOWN_TIME_MS))
        {
            Serial.println("Done cooling down.");
            currentState = S_IDLE;  
        }
        break;
    }
    
  delay(1);

}
