#include <SPI.h>
#include "Adafruit_MAX31855.h"

#define MAXDO   12
#define MAXCS   10
#define MAXCLK  13

#define HTR_ZC  2
#define HTR_PWM 3

#define USE_SERIAL Serial

#define DEBUG (false)
#define DBG_PRINT if(DEBUG)USE_SERIAL


// ********************
// PID controller
// ********************

typedef struct
{
  float measurement_prev; // Last measurement input

  float prop_gain;        // proportional gain
  float integ_gain;       // integral gain
  float deriv_gain;       // derivative gain

  float integ_accum;      // accumulator for integrator value
  float integ_max;        // maximum allowable integrator value
  float integ_min;        // minimum allowable integrator value
} PID_s;

float PID_Update(PID_s * pid, float error, float measurement)
{
  float pTerm, dTerm, iTerm;

  // calculate the proportional term
  pTerm = pid->prop_gain * error;

  // calculate the integral state with appropriate limiting
  pid->integ_accum += error;
  if (pid->integ_accum > pid->integ_max)
  {
    pid->integ_accum = pid->integ_max;
  }
  else if (pid->integ_accum < pid->integ_min)
  {
    pid->integ_accum = pid->integ_min;
  }
  // calculate the integral term
  iTerm = pid->integ_gain * pid->integ_accum;

  // calculate the derivative term
  // (based on position, rather than error, to smooth out setpoint changes)
  dTerm = pid->deriv_gain * (measurement - pid->measurement_prev);
  pid->measurement_prev = measurement;

  float mv = pTerm + iTerm - dTerm;

  return mv;
}


// ********************
// Heater controller
// ********************

const float MV_MIN = 0;
const float MV_MAX = 10;
const float SP_MAX = 250;

const uint16_t HTR_CYCLES_TOTAL = 100;
volatile uint16_t HTR_cycle_curr = 0;
volatile uint16_t HTR_cycle_trig = 0;

void HTR_SetMv(uint16_t mv_pct)
{
    if (mv_pct > HTR_CYCLES_TOTAL)
    {
        mv_pct = HTR_CYCLES_TOTAL;
    }
    else if (mv_pct < 0)
    {
        mv_pct = 0;
    }
    HTR_cycle_trig = mv_pct;
}

void HTR_TriggerZeroCross() {
  bool is_end_of_half_cycle = digitalRead(HTR_ZC);
  bool is_last_cycle = HTR_cycle_curr == 0;

  bool enable_output = (HTR_cycle_curr < HTR_cycle_trig);
  enable_output &= !(is_last_cycle && is_end_of_half_cycle);
  if (enable_output)
  {
      digitalWrite(HTR_PWM, true);
  }
  else
  {
      digitalWrite(HTR_PWM, false);
  }

  if (is_end_of_half_cycle)
  {
      if (is_last_cycle || HTR_cycle_curr > HTR_CYCLES_TOTAL)
      {
          HTR_cycle_curr = HTR_CYCLES_TOTAL;
      }
      HTR_cycle_curr--;
  }
}


// ********************
// Main bits
// ********************

Adafruit_MAX31855 thermocouple(MAXCLK, MAXCS, MAXDO);
PID_s pid_params;

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    // Wait for serial port to be ready
    delay(1);
  }

  Serial.println("Starting Oven Temperature Controller...");

  // setup heater input and output pins
  pinMode(HTR_PWM, OUTPUT);
  pinMode(HTR_ZC, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HTR_ZC), HTR_TriggerZeroCross, RISING);
  
  // HACK: these should be initialized at declaration, but Arduino doesn't support that apparently
  pid_params.prop_gain = 8;
  pid_params.integ_gain = 0.040;
  pid_params.deriv_gain = 50;
  pid_params.integ_max = 1000;
  pid_params.integ_min = -1000;
  pid_params.integ_accum = 0;

  // wait for MAX31855 chip to stabilize
  delay(500);

  Serial.println("Controller Initialized.");
}

float time_sec = 0;
int sample_rate_msec = 500;
void loop()
{
  DBG_PRINT.print("MAX31855 Temp = ");
  DBG_PRINT.print(thermocouple.readInternal());
  DBG_PRINT.println(" C");

  float tc_pv = thermocouple.readCelsius();  // thermocouple.readFarenheit()
  if (isnan(tc_pv))
  {
    USE_SERIAL.println("TC Error!");
  }


  float error = 50 - tc_pv;
  float mv = 0;

  mv = PID_Update(&pid_params, error, tc_pv);

  // constrain MV to min/max values
  if (mv > MV_MAX)
  {
    mv = MV_MAX;
  }
  else if (mv < MV_MIN) 
  {
    mv = MV_MIN;
  }

  // prevent heater from exceeding max temp setpoint
  // NOTE: threshold is slightly above setpoint to allow a little overshoot
  // without affecting any control algorithms
  if (tc_pv > (SP_MAX + 5))
  {
    mv = 0;
  }

  HTR_SetMv(mv);


  USE_SERIAL.print("Elapsed: ");
  USE_SERIAL.print(time_sec);
  USE_SERIAL.print("sec, ");
  USE_SERIAL.print("TC Temp: ");
  USE_SERIAL.print(tc_pv);
  USE_SERIAL.print("C, ");
  USE_SERIAL.print("Heater Mv = ");
  USE_SERIAL.print(mv);
  USE_SERIAL.print("%");
  USE_SERIAL.println();
  
  delay(sample_rate_msec);
  time_sec = time_sec + ((float)sample_rate_msec / 1000);
}
