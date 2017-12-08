#include "RollingStats.h"

// NOTE: this implementation comes from: http://jonisalonen.com/2014/efficient-and-accurate-rolling-standard-deviation/
void RollingStats_Update(RollingStats_t * stats, float new_value)
{
  float old_avg = stats->average;
  float new_avg = old_avg + (new_value - stats->value_prev) / stats->window_size;
  stats->average = new_avg;
  float new_var = (new_value - stats->value_prev) * (new_value - new_avg + stats->value_prev - old_avg) / (stats->window_size - 1);
  stats->variance += new_var;
  stats->value_prev = new_value;
}

