#include "TuyaClient.h"

#include <Crypto.h>
#include <SHA256.h>

#define DEVICE_ID "*****"
#define DEVICE_SECRET "*****"

String getClientId()
{
  return String("tuyalink_") + String(DEVICE_ID);
}

String calculateUsername(int64_t timestamp)
{
  return String(DEVICE_ID) + String("|signMethod=hmacSha256,timestamp=") + String(timestamp) + String(",secureMode=1,accessType=1");
}

void arrayToString(byte* array, unsigned int len, char* buffer)
{
  for (unsigned int i = 0; i < len; i++)
  {
    byte nib1 = (array[i] >> 4) & 0x0F;
    byte nib2 = (array[i] >> 0) & 0x0F;
    buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'a' + nib1  - 0xA;
    buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'a' + nib2  - 0xA;
  }
  buffer[len*2] = '\0';
}

String calculatePassword(int64_t timestamp)
{
  byte password_bytes[32];
  char password[65];

  Serial.print(timestamp);
  Serial.print("\n");

  const char* secret = DEVICE_SECRET;
  const char* content = (String("deviceId=") + String(DEVICE_ID) + String(",timestamp=") + String(timestamp) + String(",secureMode=1,accessType=1")).c_str();

  SHA256 hasher;
  hasher.resetHMAC(secret, strlen(secret));
  hasher.update(content, strlen(content));
  hasher.finalizeHMAC(secret, strlen(secret), password_bytes, 32);

  arrayToString(password_bytes, 32, password);
    
  return String(password);
}

TuyaClient::TuyaClient()
{
  m_wifiClient.setInsecure();
}

TuyaClient::~TuyaClient()
{
  delete m_client;
}

bool TuyaClient::begin(const State& state, const Settings& settings)
{
  if(m_client)
  {
    delete m_client;
  }
  m_client = new MqttClient(m_wifiClient);

  while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
  bool wifiAvailable = state.app.wifi_available;
  bool timeAvailable = state.time.initialized;
  int64_t timestamp = state.time.timestamp;
  xSemaphoreGive(state.mutex);

  if(!wifiAvailable || !timeAvailable)
  {
    return false;
  }

  String username = calculateUsername(timestamp);
  String password = calculatePassword(timestamp);
  String clientId = getClientId();
  Serial.print(clientId);
  Serial.print("\n");
  Serial.print(username);
  Serial.print("\n");
  Serial.print(password);
  Serial.print("\n");
  m_client->setId(clientId.c_str());
  m_client->setUsernamePassword(username.c_str(), password.c_str());
  if (!m_client->connect("m1.tuyaeu.com", 8883))
  {
    Serial.print("MQTT connection failed! Error code = ");
    Serial.println(m_client->connectError());

    return false;
  }
  return true;
}

bool TuyaClient::sync(const State& state, const Settings& settings)
{
  while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
  bool wifiAvailable = state.app.wifi_available;
  bool dataAvailable = state.roomWeather.initialized;
  bool timeAvailable = state.time.initialized;
  float temperature = state.roomWeather.temperature;
  float humidity = state.roomWeather.humidity;
  int co2 = state.roomWeather.co2;
  int64_t timestamp = state.time.timestamp;
  xSemaphoreGive(state.mutex);

  if(!wifiAvailable || !dataAvailable || !timeAvailable)
  {
    return false;
  }
  if(!m_client || !m_client->connected()){
    begin(state, settings);
    return false;
  }

  String topic = String("tylink/") + String(DEVICE_ID) + String("/thing/property/report");
  char buffer[512];
  sprintf(
    buffer,
    "{\"msgId\":\"%llu\", \"time\":%llu, \"data\":{\"temperature\":{\"value\":%d,\"time\":%llu},\"humidity\":{\"value\":%d,\"time\":%llu},\"co2\":{\"value\":%d,\"time\":%llu}}}",
    m_messageCounter++,
    timestamp, 
    (int)(temperature), 
    timestamp, 
    (int)(humidity), 
    timestamp, 
    co2, 
    timestamp
  );

  m_client->beginMessage(topic.c_str());
  m_client->print(buffer);
  m_client->endMessage();

  return true;
}
