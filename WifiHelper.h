#ifndef WIFI_HELPER_H
#define WIFI_HELPER_H

#include <WiFi.h>

#include "State.h"
#include "Settings.h"

class WifiHelper
{
public:
  WifiHelper(){};

  void begin(State& state, Settings& settings);

  void sync(State& state, Settings& settings);

private:
  bool waitForConnect(int seconds);
  bool handleClient(NetworkClient& client, Settings& settings);
};

#endif