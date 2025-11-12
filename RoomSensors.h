#ifndef ROOM_SENSORS_H
#define ROOM_SENSORS_H

#include <HardwareSerial.h>
#include <Adafruit_SHT4x.h>
#include <MHZ19.h>

#include "State.h"
#include "Settings.h"

class RoomSensors
{
public:
  RoomSensors()
  {
    m_mutex = xSemaphoreCreateMutex();
  }

  void begin(Stream& serial)
  {
    m_mhz19.begin(serial);
    m_mhz19.autoCalibration(false);

    m_sht4.begin();
    m_sht4.setPrecision(SHT4X_HIGH_PRECISION);
    m_sht4.setHeater(SHT4X_NO_HEATER);
  }

  void sync(State& state, Settings& settings)
  {
    while(xSemaphoreTake(m_mutex, portMAX_DELAY) != pdTRUE){}
    sensors_event_t event_humidity, event_temperature;
    m_sht4.getEvent(&event_humidity, &event_temperature);

    float temperature = event_temperature.temperature + m_temperatureCalibrationCoefficient;
    float humidity = event_humidity.relative_humidity;
    int co2 = m_mhz19.getCO2();
    xSemaphoreGive(m_mutex);

    while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
    state.roomWeather.temperature = temperature;
    state.roomWeather.humidity = humidity;
    state.roomWeather.co2 = co2;
    state.roomWeather.initialized = true;
    xSemaphoreGive(state.mutex);
  }

  void calibrateCo2()
  {
    while(xSemaphoreTake(m_mutex, portMAX_DELAY) != pdTRUE){}
    m_mhz19.calibrate();
    xSemaphoreGive(m_mutex);
  }

  void calibrateTemperature(float coefficient)
  {
    while(xSemaphoreTake(m_mutex, portMAX_DELAY) != pdTRUE){}
    m_temperatureCalibrationCoefficient = coefficient;
    xSemaphoreGive(m_mutex);
  }

private:
  Adafruit_SHT4x m_sht4;
  MHZ19 m_mhz19;

  float m_temperatureCalibrationCoefficient;

  SemaphoreHandle_t m_mutex;
};

#endif