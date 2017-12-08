#pragma once

typedef struct
{
  float measurement_prev; // Last measurement input

  float prop_gain;        // proportional gain
  float integ_gain;       // integral gain
  float deriv_gain;       // derivative gain

  float integ_accum;      // accumulator for integrator value
  float integ_max;        // maximum allowable integrator value
  float integ_min;        // minimum allowable integrator value
} PID_t;

float PID_Update(PID_t * pid, float error, float measurement);

