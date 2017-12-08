#pragma once

#include "Arduino.h"

void Heater_Init(int pwm_output_pin, int zc_input_pin);

void Heater_SetMvLimits(int16_t mv_min, int16_t mv_max);

int16_t Heater_GetTargetMv();

void Heater_SetTargetMv(int16_t mv_pct);

int16_t Heater_GetCurrentMv();

