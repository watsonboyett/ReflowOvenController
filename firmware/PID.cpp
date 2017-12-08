#include "PID.h"

float PID_Update(PID_t * pid, float error, float measurement)
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

