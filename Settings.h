#ifndef SETTINGS_H
#define SETTINGS_H

#include <EEPROM.h>


typedef struct Settings
{
  struct Wifi
  {
    bool initialized;
    char ssid[33];
    char password[64];
  };

  struct Temperature
  {
    bool needs_calibration;
    int calibration_coefficient;
  };

  Wifi wifi;
  Temperature temperature;

  SemaphoreHandle_t mutex;

public:

  static void load(Settings& settings)
  {
    EEPROM.get(0, settings);
  }

  static void save(const Settings& settings)
  {
    EEPROM.put(0, settings);
    EEPROM.commit();
  }

} Settings;

#endif
