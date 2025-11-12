#ifndef STATE_H
#define STATE_H

enum WeatherCode{
  CLEAR,
  PARLY_CLOUDY,
  CLOUDY,
  OVERCAST,
  MIST,
  PARTLY_RAIN,
  PARTLY_SNOW,
  RAIN,
  HEAVY_RAIN,
  SNOW,
  SLEET,
  THUNDER,
  THUNDER_RAIN
};

typedef struct
{
  struct Time
  {
    uint16_t hours;
    uint16_t minutes;
    uint16_t day;
    uint16_t month;
    uint16_t year;
    int64_t  timestamp;
    bool initialized;
  };

  struct RoomWeather
  {
    int co2;
    float temperature;
    float humidity;
    bool initialized;
  };

  struct Weather
  {
    float temperature;
    float humidity;
    WeatherCode weather_code;
    float pressure;
    float wind_speed;
    int wind_direction;
    int is_day;
    bool initialized;
  };

  struct App
  {
    bool wifi_available;
    bool settings_opened;
    int settings_mode;
    bool settings_dirty;
    bool screenForceUpdate;
#ifdef ENABLE_TUYA
    bool tuya_initialized;
#endif
  };

  RoomWeather roomWeather;
  Weather weather;
  Time time;
  App app;

  SemaphoreHandle_t mutex;

} State;

#endif