#include "Heater.h"

int heater_pwm_pin = -1;
int heater_zc_pin = -1;

int16_t heater_mv_min = 0;
int16_t heater_mv_max = 100;

const uint16_t HTR_CYCLES_TOTAL = 100;
volatile uint16_t HTR_cycle_curr = 0;
volatile uint16_t HTR_cycle_trig = 0;

void Heater_TriggerZeroCross() {
  bool is_end_of_half_cycle = digitalRead(heater_zc_pin);
  bool is_last_cycle = HTR_cycle_curr == 0;

  bool enable_output = (HTR_cycle_curr < HTR_cycle_trig);
  enable_output &= !(is_last_cycle && is_end_of_half_cycle);
  if (enable_output)
  {
    digitalWrite(heater_pwm_pin, true);
  }
  else
  {
    digitalWrite(heater_pwm_pin, false);
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

void Heater_Init(int pwm_output_pin, int zc_input_pin)
{
  heater_pwm_pin = pwm_output_pin;
  heater_zc_pin = zc_input_pin;

  pinMode(heater_pwm_pin, OUTPUT);
  pinMode(heater_zc_pin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(heater_zc_pin), Heater_TriggerZeroCross, RISING);
}

void Heater_SetMvLimits(int16_t mv_min, int16_t mv_max)
{
  heater_mv_min = mv_min;
  heater_mv_max = mv_max;
}

int16_t heater_mv_target_pct = 0;

int16_t Heater_GetTargetMv()
{
  return heater_mv_target_pct;
}

void Heater_SetTargetMv(int16_t mv_pct)
{
  heater_mv_target_pct = mv_pct;

  // constrain MV to min/max values
  if (heater_mv_target_pct > heater_mv_max)
  {
    heater_mv_target_pct = heater_mv_max;
  }
  else if (heater_mv_target_pct < heater_mv_min)
  {
    heater_mv_target_pct = heater_mv_min;
  }

  // constrain to PWM limits
  if (heater_mv_target_pct > HTR_CYCLES_TOTAL)
  {
    heater_mv_target_pct = HTR_CYCLES_TOTAL;
  }
  else if (heater_mv_target_pct < 0)
  {
    heater_mv_target_pct = 0;
  }

  HTR_cycle_trig = heater_mv_target_pct;
}

int16_t Heater_GetCurrentMv()
{
  return HTR_cycle_trig;
}

