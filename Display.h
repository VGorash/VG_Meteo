#ifndef DISPLAY_H
#define DISPLAY_H

#include <GxEPD2_BW.h>

#include "graphics.h"
#include "State.h"
#include "Settings.h"

#define GxEPD2_DISPLAY_CLASS GxEPD2_BW
#define GxEPD2_DRIVER_CLASS GxEPD2_420_GDEY042T81

#ifndef EPD_CS
#define EPD_CS SS
#endif

#define MAX_DISPLAY_BUFFER_SIZE 65536ul
#define MAX_HEIGHT(EPD) (EPD::HEIGHT <= MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8) ? EPD::HEIGHT : MAX_DISPLAY_BUFFER_SIZE / (EPD::WIDTH / 8))

enum DisplayMode
{
  NORMAL,
  SETTINGS,
  WIFI
};

class Callable
{
public:
  virtual void call(const State& state) = 0;
};

template<typename E>
class UpdateCallback : public Callable
{
public:
  UpdateCallback(E* element, void (*updateCallback) (E*, const State&)) : m_element(element), m_updateCallback(updateCallback){}

  void call(const State& state) override
  {
    m_updateCallback(m_element, state);
  }

private:
  E* m_element;
  void (*m_updateCallback) (E*, const State&);
};

class Display
{
public:
  Display();
  ~Display();

  void sync(State& state, Settings& settings);

  void updateState(const State& newState);
  void updateFull();
  void updatePartial();

  void showSettingsIcon(const String& message);
  void showWifiIcon(bool serverMode);

private:
  void initElements();
  void deleteElements();

  bounds_t createTextElement(const coordinates_t& coords, const String& text, const GFXfont* font, void (*updateCallback) (TextElement*, const State&)=nullptr);
  bounds_t createImageElement(const coordinates_t& coords, const uint8_t* bitmap, void (*updateCallback) (ImageElement*, const State&)=nullptr);

  void partialUpdate(Element* e);

private:
  GxEPD2_DISPLAY_CLASS<GxEPD2_DRIVER_CLASS, MAX_HEIGHT(GxEPD2_DRIVER_CLASS)>* m_display;

  Element** m_elements;

  Callable** m_callbacks;

  uint16_t m_numElements = 0;
  uint16_t m_numCallbacks = 0;

  DisplayMode m_mode = DisplayMode::WIFI;

  uint16_t m_numPartialUpdates = 0;

};

#endif