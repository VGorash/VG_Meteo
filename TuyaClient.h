#ifndef TUYA_CLIENT_H
#define TUYA_CLIENT_H

#include <WiFiClientSecure.h>
#include <ArduinoMqttClient.h>

#include "State.h"
#include "Settings.h"

class TuyaClient
{
public:
  TuyaClient();
  ~TuyaClient();

  bool begin(const State& state, const Settings& settings);
  bool sync(const State& state, const Settings& settings);

private:
  WiFiClientSecure m_wifiClient;
  MqttClient* m_client;
  uint64_t m_messageCounter;
};

#endif