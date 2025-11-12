#include "Display.h"
#include "bitmaps_weather.h"
#include "bitmaps_icons.h"

#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>

#define MAX_ELEMENTS 64
#define PARTIAL_UPDATES_LIMIT 4000

String settingsModes[4] = {String("Back"), String("Reset Wi-Fi"), String("Calibrate Temperature"), String("Calibrate CO2")};

Display::Display()
{
  m_display = new GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>(GxEPD2_DRIVER_CLASS(/*CS=*/ 5, /*DC=*/ 17, /*RST=*/ 16, /*BUSY=*/ 4));

  m_display->init(115200, true, 2, false);
  m_display->setRotation(0);

  m_elements = new Element*[MAX_ELEMENTS];
  m_callbacks = new Callable*[MAX_ELEMENTS];

  for(int i=0; i<MAX_ELEMENTS; i++)
  {
    m_elements[i] = nullptr;
    m_callbacks[i] = nullptr;
  }

  initElements();
}

Display::~Display()
{
  deleteElements();
  delete[] m_elements;
  delete[] m_callbacks;
  delete m_display;
}

void innerTemperatureUpdateCallback(TextElement* element, const State& state)
{
  element->setText(String(state.roomWeather.temperature, 0));
}

void innerHumidityUpdateCallback(TextElement* element, const State& state)
{
  element->setText(String(state.roomWeather.humidity, 0));
}

void co2UpdateCallback(TextElement* element, const State& state)
{
  element->setText(String(state.roomWeather.co2));
}

void flowerUpdateCallback(ImageElement* element, const State& state)
{
  if(state.roomWeather.co2 > 1000)
  {
    element->setImage(bitmap_flower_dead);
  }
  else
  {
    element->setImage(bitmap_flower_alive);
  }
}

void timeUpdateCallback(TextElement* element, const State& state)
{
  char buffer[32];
  sprintf(buffer, "%02d:%02d", state.time.hours, state.time.minutes);
  element->setText(String(buffer));
}

void dateUpdateCallback(TextElement* element, const State& state)
{
  char buffer[32];
  sprintf(buffer, "%02d.%02d.%04d", state.time.day, state.time.month + 1, state.time.year + 1900);
  element->setText(String(buffer));
}

void temperatureUpdateCallback(TextElement* element, const State& state)
{
  element->setText(String(state.weather.temperature, 0));
}

void windSpeedUpdateCallback(TextElement* element, const State& state)
{
  element->setText(String(state.weather.wind_speed, 0));
}

void windDirectionUpdateCallback(ImageElement* element, const State& state)
{
  if (state.weather.wind_direction >= 337 || state.weather.wind_direction < 23)
  {
    element->setImage(bitmap_wind_south);
  }
  else if (state.weather.wind_direction < 67)
  {
    element->setImage(bitmap_wind_southwest);
  }
  else if (state.weather.wind_direction < 112)
  {
    element->setImage(bitmap_wind_west);
  }
  else if (state.weather.wind_direction < 157)
  {
    element->setImage(bitmap_wind_northwest);
  }
  else if (state.weather.wind_direction < 202)
  {
    element->setImage(bitmap_wind_north);
  }
  else if (state.weather.wind_direction < 247)
  {
    element->setImage(bitmap_wind_northeast);
  }
  else if (state.weather.wind_direction < 292)
  {
    element->setImage(bitmap_wind_east);
  }
  else
  {
    element->setImage(bitmap_wind_southeast);
  }
}

void weatherUpdateCallback(ImageElement* element, const State& state)
{
  switch(state.weather.weather_code)
  {
    case WeatherCode::CLEAR:
    {
      if(state.weather.is_day == 1)
      {
        element->setImage(bitmap_clear_day);
      }
      else
      {
        element->setImage(bitmap_clear_night);
      }
      break;
    }
    case WeatherCode::PARLY_CLOUDY:
    {
      if(state.weather.is_day == 1)
      {
        element->setImage(bitmap_partly_day);
      }
      else
      {
        element->setImage(bitmap_partly_night);
      }
      break;
    }
    case WeatherCode::CLOUDY:
    {
      element->setImage(bitmap_cloudy);
      break;
    }
    case WeatherCode::OVERCAST:
    {
      element->setImage(bitmap_overcast);
      break;
    }
    case WeatherCode::THUNDER:
    {
      element->setImage(bitmap_thunder);
      break;
    }
    case WeatherCode::MIST:
    {
      element->setImage(bitmap_fog);
      break;
    }
    case WeatherCode::PARTLY_RAIN:
    {
      if(state.weather.is_day == 1)
      {
        element->setImage(bitmap_rain_partly_day);
      }
      else
      {
        element->setImage(bitmap_rain_partly_night);
      }
      break;
    }
    case WeatherCode::RAIN:
    {
      element->setImage(bitmap_rain);
      break;
    }
    case WeatherCode::HEAVY_RAIN:
    {
      element->setImage(bitmap_heavy_rain);
      break;
    }
    case WeatherCode::PARTLY_SNOW:
    {
      if(state.weather.is_day == 1)
      {
        element->setImage(bitmap_snow_partly_day);
      }
      else
      {
        element->setImage(bitmap_snow_partly_night);
      }
      break;
    }
    case WeatherCode::SNOW:
    {
      element->setImage(bitmap_snow);
      break;
    }
    case WeatherCode::SLEET:
    {
      element->setImage(bitmap_sleet);
      break;
    }
    case WeatherCode::THUNDER_RAIN:
    {
      element->setImage(bitmap_thunder_rain);
      break;
    }
  }
}


bounds_t Display::createTextElement(const coordinates_t& coords, const String& text, const GFXfont* font, void (*updateCallback) (TextElement*, const State&))
{
  TextElement* e = new TextElement(m_display, coords, text, font);
  m_elements[m_numElements++] = e;
  if(updateCallback){
    m_callbacks[m_numCallbacks++] = new UpdateCallback<TextElement>(e, updateCallback);
  }
  return e->getBounds();
}

bounds_t Display::createImageElement(const coordinates_t& coords, const uint8_t* bitmap, void (*updateCallback) (ImageElement*, const State&))
{
  ImageElement* e = new ImageElement(m_display, coords, bitmap);
  m_elements[m_numElements++] = e;
  if(updateCallback){
    m_callbacks[m_numCallbacks++] = new UpdateCallback<ImageElement>(e, updateCallback);
  }
  return e->getBounds();
}

void Display::initElements()
{
  bounds_t houseBounds = createImageElement({12, 10, 203, 278}, bitmap_house);

  bounds_t innerTemperatureBounds = createTextElement({houseBounds.left + 20, houseBounds.top + 95, 40, 32}, "00", &FreeSansBold18pt7b, innerTemperatureUpdateCallback);
  createImageElement({innerTemperatureBounds.right, innerTemperatureBounds.top + 4, 22, 22}, bitmap_degree);

  bounds_t innerHumidityBounds = createTextElement({houseBounds.left + 116, houseBounds.top + 95, 40, 32}, "00", &FreeSansBold18pt7b, innerHumidityUpdateCallback);
  createImageElement({innerHumidityBounds.right, innerHumidityBounds.top + 7, 18, 18}, bitmap_percent);

  bounds_t innerCo2Bounds = createTextElement({houseBounds.left + 124, houseBounds.top + 180, 40, 24}, "000", &FreeSansBold12pt7b, co2UpdateCallback);
  createImageElement({houseBounds.left + 36, houseBounds.top + 200, 32, 44}, bitmap_flower_alive, flowerUpdateCallback);

  createImageElement({houseBounds.right + 45, houseBounds.top, 112, 112}, bitmap_clear_day, weatherUpdateCallback);

  bounds_t timeBounds = createTextElement({houseBounds.right + 30, houseBounds.top + 110, 136, 40}, "00:00", &FreeSansBold24pt7b, timeUpdateCallback);
  bounds_t dateBounds = createTextElement({timeBounds.left, timeBounds.bottom + 2, 136, 24}, "00.00.0000", &FreeSans12pt7b, dateUpdateCallback);

  bounds_t windIconBounds = createImageElement({houseBounds.right + 30, houseBounds.bottom - 40, 34, 34}, bitmap_wind_northeast, windDirectionUpdateCallback);
  bounds_t windBounds = createTextElement({windIconBounds.right + 9, windIconBounds.top, 56, 32}, "0", &FreeSansBold18pt7b, windSpeedUpdateCallback);
  createImageElement({windBounds.right, windBounds.top + 7, 40, 19}, bitmap_windspeed);

  bounds_t temperatureIconBounds = createImageElement({houseBounds.right + 40, windIconBounds.top - 40, 16, 34}, bitmap_temperature);
  bounds_t temperatureBounds = createTextElement({temperatureIconBounds.right + 17, temperatureIconBounds.top, 56, 32}, "0", &FreeSansBold18pt7b, temperatureUpdateCallback);
  createImageElement({temperatureBounds.right, temperatureBounds.top + 4, 22, 22}, bitmap_degree);

}

void Display::deleteElements()
{
  for(int i=0; i<m_numElements; i++)
  {
    delete m_elements[i];
  }

  for(int i=0; i<m_numCallbacks; i++)
  {
    delete m_callbacks[i];
  }
}

uint16_t findNearEight(uint16_t x)
{
  if(x < 8)
  {
    return 8;
  }
  if(x % 8 == 0)
  {
    return x;
  }
  return (x / 8 + 1) * 8;
}

void Display::partialUpdate(Element* e)
{
  coordinates_t coords = e->getCoordinates();

  m_display->setPartialWindow(coords.x, coords.y, findNearEight(coords.w), coords.h);
  m_display->firstPage();
  do
  {
    e->show();
  }
  while (m_display->nextPage());
}

void Display::updateState(const State& newState)
{
  for(int i=0; i<m_numCallbacks; i++)
  {
    m_callbacks[i]->call(newState);
  }
}

void Display::updateFull()
{
  m_display->setFullWindow();
  m_display->firstPage();
  do
  {
    m_display->fillScreen(GxEPD_WHITE);
    for(int i=0; i<m_numElements; i++)
    {
      m_elements[i]->show();
    }
  }
  while (m_display->nextPage());
  m_display->hibernate();
}

void Display::updatePartial()
{
  for(int i=0; i<m_numElements; i++)
  {
    if(m_elements[i]->isDirty())
    {
      partialUpdate(m_elements[i]);
      m_elements[i]->clearDirty();
    }
  }
  m_display->hibernate();
}

void Display::showSettingsIcon(const String& message)
{
  m_display->setFullWindow();
  m_display->firstPage();
  ImageElement icon(m_display, {72, 22, 256, 256}, bitmap_settings);
  TextElement messageBox(m_display, {10, 240, 380, 0}, message, &FreeSansBold18pt7b);
  do
  {
    icon.show();
    messageBox.show();
  }
  while (m_display->nextPage());
  m_display->hibernate();
}

void Display::showWifiIcon(bool serverMode)
{
  m_display->setFullWindow();
  m_display->firstPage();
  const uint8_t* bitmap_info = serverMode ? bitmap_wifi_server : bitmap_wifi_connecting;
  ImageElement icon(m_display, {72, 22, 256, 256}, bitmap_wifi);
  ImageElement info(m_display, {50, 240, 300, 30}, bitmap_info);
  do
  {
    m_display->fillScreen(GxEPD_WHITE);
    icon.show();
    info.show();
  }
  while (m_display->nextPage());
  m_display->hibernate();
}

void Display::sync(State& state, Settings& settings)
{
  DisplayMode oldMode = m_mode;
  bool settingsDirty;
  while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
  updateState(state);
  if(state.app.settings_opened)
  {
    m_mode = DisplayMode::SETTINGS;
    settingsDirty = state.app.settings_dirty;
    state.app.settings_dirty = false;
  }
  else if (!state.app.wifi_available)
  {
    m_mode = DisplayMode::WIFI;
  }
  else if (!state.roomWeather.initialized || !state.weather.initialized || !state.time.initialized)
  {
    m_mode = DisplayMode::WIFI;
  }
  else
  {
    m_mode = DisplayMode::NORMAL;
  }
  xSemaphoreGive(state.mutex);
  if(m_mode==DisplayMode::NORMAL)
  {
    if(oldMode == DisplayMode::NORMAL && m_numPartialUpdates < PARTIAL_UPDATES_LIMIT)
    {
      updatePartial();
      m_numPartialUpdates++;
    }
    else
    {
      updateFull();
      m_numPartialUpdates = 0;
    }
  }
  else
  {
    if(m_mode == DisplayMode::WIFI)
    {
      if(oldMode == DisplayMode::WIFI)
      {
        return;
      }
      showWifiIcon(false);
    }
    else if(m_mode == DisplayMode::SETTINGS)
    {
      if(oldMode == DisplayMode::SETTINGS && !settingsDirty)
      {
        return;
      }
      showSettingsIcon(settingsModes[state.app.settings_mode]);
    }
  }
}