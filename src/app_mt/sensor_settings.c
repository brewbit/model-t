
#include "sensor_settings.h"

float
sensor_settings_get_current_setpoint(const sensor_settings_t* settings)
{
  if (settings->setpoint_type == SP_STATIC)
    return settings->static_setpoint.value;
  else
    return temp_profile_get_current_setpoint(&settings->temp_profile);
}
