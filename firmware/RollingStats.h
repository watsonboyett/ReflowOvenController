#pragma once

typedef struct
{
  int window_size;
  float value_prev;
  float average;
  float variance;
} RollingStats_t;

void RollingStats_Update(RollingStats_t * stats, float new_value);

