#include "mbed.h"

// Example setup for echo and trigger using D2 and D3
// YOUR JOB: set up echo and trigger for a second sensor
DigitalIn echoR(ARDUINO_UNO_D2);
DigitalIn echoB(ARDUINO_UNO_D11);
DigitalOut triggerR(ARDUINO_UNO_D3);
DigitalOut triggerB(ARDUINO_UNO_D10);


// Timer for echo code
Timer echoDurationR;
Timer echoDurationB;
Timer travelTime;

using namespace std::chrono; // namespace for timers

// Example setup for light1 using D4, D5, D6
// YOUR JOB: set up bus for light2
BusOut redLight(ARDUINO_UNO_D6,ARDUINO_UNO_D5,ARDUINO_UNO_D4);
BusOut blueLight(ARDUINO_UNO_D9,ARDUINO_UNO_D8,ARDUINO_UNO_D7);

// Defines to make our lives easier when using busses
// We can use the same defines for light1 and light2 as long as wiring is consistent
#define RED 0b100 // top bit / pin is red. Corresponds to the last pin in light1 declaration.
#define YELLOW 0b010 // middle bit / pin is yellow. Corresponds to the middle pin in light1 declaration. 
#define GREEN 0b001 // lowest bit / pin is green. Corresponds to the first pin in light1 declaration.

enum SM1_States { SM1_Intermediate, SM1_TurnOffRed, SM1_TurnOffBlue, SM1_NoBlue, SM1_OnlyBlue, SM1_RedTwoMin, SM1_BlueOneMin } SM1_State;

int Tick1 = 0;
int BCTick = 0;

bool blueCar = false;
bool redCar = true;

unsigned char SM1_Clk;

Ticker flipper;

void GetDistance(void){
    
    // Trigger pulse
    triggerR = 1;
    wait_us(10);
    triggerR = 0;

    // Red Car
    travelTime.start();
    while (echoR != 1 && duration_cast<milliseconds>(travelTime.elapsed_time()).count() < 200); // wait for echo to go high
    travelTime.stop();
    travelTime.reset();
    
    echoDurationR.start(); // start timer
    while (echoR == 1 && duration_cast<milliseconds>(echoDurationR.elapsed_time()).count() < .2); // wait for echo to go low
    echoDurationR.stop(); // stop timer

    // Blue Car
    triggerB = 1;
    wait_us(10);
    triggerB = 0;

    travelTime.start();
    while (echoB != 1 && duration_cast<milliseconds>(travelTime.elapsed_time()).count() < 200); // wait for echo to go high
    travelTime.stop();
    travelTime.reset();

    echoDurationB.start(); // start timer
    while (echoB == 1 && duration_cast<milliseconds>(echoDurationB.elapsed_time()).count() < .5); // wait for echo to go low
    echoDurationB.stop(); // stop timer

    // print distance in cm (switch to divison by 148 for inches)
    float redDist = (float)duration_cast<microseconds>(echoDurationR.elapsed_time()).count()/58.0;
    float blueDist = (float)duration_cast<microseconds>(echoDurationB.elapsed_time()).count()/58.0;

    redCar = redDist < 2.5;
    blueCar = blueDist < 8;

    // printf("%f cm (R)\n",redDist);
    // printf("%f cm (B)\n",blueDist);

    printf("Red: %d", redCar);
    printf("Blue: %d\n", blueCar);
    
    echoDurationR.reset(); // need to reset timer (doesn't happen automatically)
    echoDurationB.reset();

    //thread_sleep_for(100); // use 100 ms measurement cycle for this example
}


void TickFct_State_machine_1() {
   switch(SM1_State) { // Transitions
      case SM1_Intermediate: 
         if ((!blueCar)) {
            SM1_State = SM1_NoBlue;
         }
         else if ((!redCar && blueCar)) {
            SM1_State = SM1_OnlyBlue;
         }
         else if ((redCar && blueCar &&BCTick <= 120)) {
            SM1_State = SM1_RedTwoMin;
            BCTick = 0;
         }
         else if ((redCar && blueCar &&BCTick > 120)) {
            SM1_State = SM1_BlueOneMin;
         }
         break;
      case SM1_TurnOffRed: 
         if ((Tick1 > 10)) {
            SM1_State = SM1_Intermediate;
         }
         break;
      case SM1_TurnOffBlue: 
         if ((Tick1 > 10)) {
            SM1_State = SM1_Intermediate;
         }
         break;
      case SM1_NoBlue: 
         if ((!redCar && blueCar)) {
            SM1_State = SM1_TurnOffRed;
         }
         else if (!blueCar) {
            SM1_State = SM1_NoBlue;
         }
         else if ((redCar && blueCar)) {
            SM1_State = SM1_RedTwoMin;
            BCTick = 0;
         }
         break;
      case SM1_OnlyBlue: 
         if ((redCar||!blueCar)) {
            SM1_State = SM1_TurnOffBlue;
         }
         else if ((!redCar&&blueCar)) {
            SM1_State = SM1_OnlyBlue;
         }
         break;
      case SM1_RedTwoMin: 
         if (((BCTick > 120) || (!redCar))) {
            SM1_State = SM1_TurnOffRed;
         }
         else if ((!blueCar)) {
            SM1_State = SM1_NoBlue;
            BCTick = 0;
         }
         break;
      case SM1_BlueOneMin: 
         if ((!redCar)) {
            SM1_State = SM1_OnlyBlue;
            BCTick = 0;
         }
         else if ((!blueCar || BCTick >= 180)) {
            SM1_State = SM1_TurnOffBlue;
            BCTick = 0;
         }
         break;
      default:
         SM1_State = SM1_Intermediate;
   } // Transitions

   switch(SM1_State) { // State actions
      case SM1_Intermediate:
         Tick1 = 0;
         
         break;
      case SM1_TurnOffRed:
         Tick1++;
         if (Tick1 <= 5) { // RL = Yellow, BL = Red (5 seconds)
                    redLight = YELLOW;
                    blueLight = RED;
                 } else if (Tick1 <= 10) { // RL = Red, BL = Red (5 seconds)
                    redLight = RED;
                 } 
         break;
      case SM1_TurnOffBlue:
         Tick1++;
         if (Tick1 <= 5) { // RL = Red, BL = Yellow (5 seconds)
                    redLight = RED;
                    blueLight = YELLOW;
                 } else if (Tick1 <= 10) { // RL = Red, BL = Red (5 seconds)
                     blueLight = RED;
                 }
         break;
      case SM1_NoBlue:
         redLight = GREEN;
         blueLight = RED;
         break; 
      case SM1_OnlyBlue:
         redLight = RED;
         blueLight = GREEN;
         break;
      case SM1_RedTwoMin:
         BCTick++;
         redLight = GREEN;
         blueLight = RED;
         break; 
      case SM1_BlueOneMin:
         BCTick++;
         redLight = RED;
         blueLight = GREEN;
         break;

      default: // ADD default behaviour below
      break;
   } // State actions

}

void bigSM(void) {
    GetDistance();
    TickFct_State_machine_1();
}

int main()
{
    // needed to use thread_sleep_for in debugger
    // your board will get stuck without it :(
    #if defined(MBED_DEBUG) && DEVICE_SLEEP
        HAL_DBGMCU_EnableDBGSleepMode();
    #endif

    flipper.attach(&bigSM, 1s);

    SM1_State = SM1_Intermediate; // Initial state
    //B = 0; // Init outputs

    while(1) {
    }
}
