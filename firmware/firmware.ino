
#include "Heater.h"
#include "PID.h"
#include "RollingStats.h"
#include "Adafruit_MAX31856.h"
#include <SPI.h>

const float MV_MIN = 0;
const float MV_MAX = 50;
const float SP_MAX = 220;

#define SPI_DO        12
#define SPI_DI        11
#define SPI_CLK       13
#define MAX31856_CS   10
#define HTR_ZC        2
#define HTR_PWM       3

#define DEBUG (false)
#define USE_SERIAL Serial
#define DBG_PRINT if(DEBUG)USE_SERIAL

// ********************
// Main bits
// ********************

Adafruit_MAX31856 TC = Adafruit_MAX31856(MAX31856_CS, SPI_DI, SPI_DO, SPI_CLK);

PID_t pid_params;
RollingStats_t stats;

float get_TC_temp()
{
  DBG_PRINT.print("MAX31855 CJ Temp = ");
  DBG_PRINT.print(TC.readCJTemperature());
  DBG_PRINT.println(" C");

  float tc_temp = TC.readThermocoupleTemperature();
  if (isnan(tc_temp))
  {
    USE_SERIAL.println("TC Error!");
  }

  // Check and print any faults
  uint8_t fault = TC.readFault();
  if (fault) {
    if (fault & MAX31856_FAULT_CJRANGE) USE_SERIAL.println("Cold Junction Range Fault");
    if (fault & MAX31856_FAULT_TCRANGE) USE_SERIAL.println("Thermocouple Range Fault");
    if (fault & MAX31856_FAULT_CJHIGH)  USE_SERIAL.println("Cold Junction High Fault");
    if (fault & MAX31856_FAULT_CJLOW)   USE_SERIAL.println("Cold Junction Low Fault");
    if (fault & MAX31856_FAULT_TCHIGH)  USE_SERIAL.println("Thermocouple High Fault");
    if (fault & MAX31856_FAULT_TCLOW)   USE_SERIAL.println("Thermocouple Low Fault");
    if (fault & MAX31856_FAULT_OVUV)    USE_SERIAL.println("Over/Under Voltage Fault");
    if (fault & MAX31856_FAULT_OPEN)    USE_SERIAL.println("Thermocouple Open Fault");
  }

  return tc_temp;
}

void setup()
{
  Serial.begin(9600);
  while (!Serial)
  {
    // Wait for serial port to be ready
    delay(1);
  }

  Serial.println("Starting Temperature Controller...");

  Heater_Init(HTR_PWM, HTR_ZC);
  Heater_SetMvLimits(MV_MIN, MV_MAX);

  // initialize PID parameters
  // HACK: these should be initialized at declaration, but Arduino doesn't support that apparently
  pid_params.prop_gain = 8;
  pid_params.integ_gain = 0.040;
  pid_params.deriv_gain = 50;
  pid_params.integ_max = 1000;
  pid_params.integ_min = -1000;
  pid_params.integ_accum = 0;

  // initialize MAX31856 chip and wait for it to stabilize
  TC.begin();
  TC.setThermocoupleType(MAX31856_TCTYPE_K);
  delay(500);

  // initialize rolling stats
  stats.window_size = 20;
  stats.value_prev = get_TC_temp();;
  stats.average = get_TC_temp();;
  stats.variance = 0;

  Serial.println("Controller Initialized.");
}

float interp1(float x0, float x1, float y0, float y1, float x)
{
  if (abs(x1 - x0) < 1e-5)
  {
    return (y0 + y1) / 2;
  }
  return y0 + (x - x0) * (y1 - y0) / (x1 - x0);
}

float time_sec = 0;
int sample_rate_msec = 500;
float temp_setpoint = 25;
void loop()
{
  float temp_curr = get_TC_temp();
  RollingStats_Update(&stats, temp_curr);

  // Update temperature controller setpoint
  switch (1)
  {
    // constant temperature
    case (0):
      {
        temp_setpoint = 25;
        break;
      }

    // setpoint temperature profile (for reflow soldering)
    case (1):
      {
        if (time_sec < 90)
        {
          temp_setpoint = interp1(0, 90, 25, 145, time_sec);
        }
        else if (time_sec < 180)
        {
          temp_setpoint = interp1(90, 180, 145, 180, time_sec);
        }
        else if (time_sec < 210)
        {
          temp_setpoint = interp1(180, 210, 180, 210, time_sec);
        }
        else if (time_sec < 240)
        {
          temp_setpoint = interp1(210, 240, 210, 180, time_sec);
        }
        else if (time_sec < 300)
        {
          temp_setpoint = interp1(210, 300, 210, 25, time_sec);
        }
        else
        {
          temp_setpoint = 25;
        }
        break;
      }
  }

  // Update temperature controller output
  float error = temp_setpoint - temp_curr;
  float mv = 0;
  static int temp_stable_count = 0;
  switch (2)
  {
    // Heater output OFF
    case (0):
      {
        mv = 0;
        break;
      }

    // thermostat controller
    case (1):
      {
        const float error_thresh = 0.5;
        if (error > error_thresh)
        {
          mv = MV_MAX;
        }
        else if (error < -error_thresh)
        {
          mv = MV_MIN;
        }

        break;
      }

    // PID controller
    case (2):
      {
        mv = PID_Update(&pid_params, error, temp_curr);
        break;
      }

    // step response data collection
    case (3):
      {
        bool temp_stable = false;
        const float temp_stable_thresh = 0.5;
        const int temp_stable_count_thresh = 20;

        static float mv_prev = 0;
        static float mv_curr = 0;

        temp_stable = abs(stats.variance) < temp_stable_thresh;
        temp_stable_count = temp_stable ? temp_stable_count + 1 : 0;

        if (temp_stable_count > temp_stable_count_thresh)
        {
          temp_stable_count = 0;

          // let heater cool down to ambient before running next MV level
          if (mv_curr > 0)
          {
            mv_prev = mv_curr;
            mv_curr = 0;
          }
          else
          {
            mv_curr = mv_prev + 5;
          }
        }

        mv = mv_curr;
        break;
      }
  }


  // prevent heater from exceeding max temp setpoint
  // NOTE: threshold is slightly above setpoint to allow
  // for some overshoot without affecting any control algorithms
  if (temp_curr > (SP_MAX + 5))
  {
    mv = 0;
  }

  Heater_SetTargetMv(mv);
  uint16_t mv_cur = Heater_GetCurrentMv();


  USE_SERIAL.print("Time: ");
  USE_SERIAL.print(time_sec);
  USE_SERIAL.print("s, ");
  USE_SERIAL.print("SP: ");
  USE_SERIAL.print(temp_setpoint);
  USE_SERIAL.print("C, ");
  USE_SERIAL.print("PV: ");
  USE_SERIAL.print(temp_curr);
  USE_SERIAL.print("C, ");
  USE_SERIAL.print("MV = ");
  USE_SERIAL.print(mv_cur);
  USE_SERIAL.print("%, ");
  USE_SERIAL.print("Var = ");
  USE_SERIAL.print(stats.variance);
  USE_SERIAL.print(", StableCnt = ");
  USE_SERIAL.print(temp_stable_count);
  USE_SERIAL.println();

  delay(sample_rate_msec);
  time_sec = time_sec + ((float)sample_rate_msec / 1000);
}


