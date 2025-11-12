#define ENABLE_TUYA

#include <EEPROM.h>
#include <time.h>
#include <EncButton.h>
#include <esp_task_wdt.h>

#include "Display.h"
#include "State.h"
#include "RoomSensors.h"
#include "WifiHelper.h"
#include "MeteoAPI.h"
#ifdef ENABLE_TUYA
#include "TuyaClient.h"
#endif

#define WDT_TIMEOUT_SECONDS 60

#define DISPLAY_UPDATE_SECONDS 1
#define SENSORS_UPDATE_SECONDS 30
#define WEATHER_UPDATE_SECONDS 60 * 15
#define TEMPERATURE_CALIBRATION_SECONDS 60 * 20

#ifdef ENABLE_TUYA
#define TUYA_UPDATE_SECONDS 30
#endif

// My objects
Display display;
State state;
Settings settings;
RoomSensors sensors;
WifiHelper wifiHelper;
MeteoAPI meteoApi;
Button btn(32, INPUT_PULLUP);
#ifdef ENABLE_TUYA
TuyaClient tuyaClient;
#endif


void setup() {
  Serial.begin(9600);
  delay(1000);

  EEPROM.begin(512);
  Settings::load(settings);

  sensors.calibrateTemperature(settings.temperature.calibration_coefficient);

  pinMode(2, OUTPUT);

  state.mutex = xSemaphoreCreateMutex();
  settings.mutex = xSemaphoreCreateMutex();

  Serial1.begin(9600, SERIAL_8N1, 25, 26);
  sensors.begin(Serial1);

  display.showWifiIcon(!settings.wifi.initialized);
  wifiHelper.begin(state, settings);

  configTime(3600*3, 0, "ntp0.ntp-servers.net", "0.pool.ntp.org", "1.pool.ntp.org");

  btn.attach(buttonCallback);

  esp_task_wdt_deinit();
  esp_task_wdt_config_t wdt_config{WDT_TIMEOUT_SECONDS * 1000, (1 << portNUM_PROCESSORS) - 1, true};
  esp_task_wdt_init(&wdt_config);
  esp_task_wdt_add(NULL);

  static int task_number0 = 0;
  TaskHandle_t task_handle0;
  xTaskCreate(
    sensorUpdateTask, "Sensor Update",
    2048,
    (void *)&task_number0,
    1,  // priority
    &task_handle0
  );
  esp_task_wdt_add(task_handle0);
  delay(100);

  static int task_number1 = 1;
  TaskHandle_t task_handle1;
  xTaskCreate(
    displayUpdateTask, "Display Update",
    4096,
    (void *)&task_number1,
    1,  // priority
    &task_handle1
  );
  esp_task_wdt_add(task_handle1);
  delay(100);

  static int task_number2 = 2;
  TaskHandle_t task_handle2;
  xTaskCreate(
    wifiUpdateTask, "Wifi Update",
    2048,
    (void *)&task_number2,
    1,  // priority
    &task_handle2
  );
  esp_task_wdt_add(task_handle2);
  delay(100);

  static int task_number3 = 3;
  TaskHandle_t task_handle3;
  xTaskCreate(
    timeUpdateTask, "Time Update",
    2048,
    (void *)&task_number3,
    1,  // priority
    &task_handle3
  );
  esp_task_wdt_add(task_handle3);
  delay(100);

  static int task_number4 = 4;
  TaskHandle_t task_handle4;
  xTaskCreate(
    weatherUpdateTask, "Weather Update",
    4096,
    (void *)&task_number4,
    1,  // priority
    &task_handle4
  );
  esp_task_wdt_add(task_handle4);
  delay(100);

  if(settings.temperature.needs_calibration){
    static int task_number5 = 5;
    xTaskCreate(
      temperatureCalibrateTask, "Calibrate Temperature",
      2048,
      (void *)&task_number5,
      1,  // priority
      NULL
    );
    delay(100);
  }

#ifdef ENABLE_TUYA
  TaskHandle_t task_handle6;
  static int task_number6 = 6;
  xTaskCreate(
    tuyaTask, "Send Data To Tuya",
    8192,
    (void *)&task_number6,
    1,  // priority
    &task_handle6
  );
  esp_task_wdt_add(task_handle6);
  delay(100);
#endif
}

void loop() {
  btn.tick();
  esp_task_wdt_reset();
  delay(10);
}

void displayUpdateTask(void* pvParameters)
{
  int seconds = DISPLAY_UPDATE_SECONDS - 1;
  while(1)
  {
    seconds++;
    if(seconds == DISPLAY_UPDATE_SECONDS)
    {
      seconds = 0;
      display.sync(state, settings);
    }
    esp_task_wdt_reset();
    delay(1000);
  }
}

void sensorUpdateTask(void* pvParameters)
{
  int seconds = SENSORS_UPDATE_SECONDS - 1;
  while(1)
  {
    seconds++;
    if(seconds == SENSORS_UPDATE_SECONDS)
    {
      seconds = 0;
      sensors.sync(state, settings);
    }
    esp_task_wdt_reset();
    delay(1000);
  }
}

void wifiUpdateTask(void* pvParameters)
{
  while(1)
  {
    wifiHelper.sync(state, settings);
    esp_task_wdt_reset();
    delay(1000);
  }
}

void timeUpdateTask(void* pvParameters)
{
  while(1)
  {
    int64_t  timestamp;
    struct tm timeinfo;
    bool initialized = getLocalTime(&timeinfo);
    time(&timestamp);
    while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
    state.time.timestamp = timestamp;
    state.time.hours = timeinfo.tm_hour;
    state.time.minutes = timeinfo.tm_min;
    state.time.day = timeinfo.tm_mday;
    state.time.month = timeinfo.tm_mon;
    state.time.year = timeinfo.tm_year;
    state.time.initialized = initialized;
    xSemaphoreGive(state.mutex);
    esp_task_wdt_reset();
    delay(1000);
  }
}

void weatherUpdateTask(void* pvParameters)
{
  int seconds = WEATHER_UPDATE_SECONDS - 1;
  bool meteoUpdatedSuccess = false;
  while(1)
  {
    seconds++;
    if(!meteoUpdatedSuccess || seconds == WEATHER_UPDATE_SECONDS)
    {
      seconds = 0;
      meteoUpdatedSuccess = meteoApi.sync(state, settings);
    }
    esp_task_wdt_reset();
    delay(1000);
  }
}

void temperatureCalibrateTask(void* pvParameters)
{

  sensors.sync(state, settings);
  float oldTemperature = state.roomWeather.temperature;

  delay(1000 * TEMPERATURE_CALIBRATION_SECONDS);

  sensors.sync(state, settings);
  float newTemperature = state.roomWeather.temperature; 
  float calibrationCoefficient = oldTemperature - newTemperature;
  sensors.calibrateTemperature(calibrationCoefficient);

  while(xSemaphoreTake(settings.mutex, portMAX_DELAY) != pdTRUE){}
  settings.temperature.needs_calibration = false;
  settings.temperature.calibration_coefficient = calibrationCoefficient;
  Settings::save(settings);
  xSemaphoreGive(settings.mutex);

  vTaskDelete(NULL);
}

void buttonCallback()
{
  if(btn.hold())
  {
    digitalWrite(2, 1);
    delay(100);
    digitalWrite(2, 0);
    while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}

    if(!state.app.settings_opened)
    {
      state.app.settings_opened = true;
      state.app.settings_mode = 0;
      state.app.settings_dirty = true;
    }
    else
    {
      if(state.app.settings_mode == 1)
      {
        while(xSemaphoreTake(settings.mutex, portMAX_DELAY) != pdTRUE){}
        settings.wifi.initialized = false;
        Settings::save(settings);
        xSemaphoreGive(settings.mutex);          
      }
      else if(state.app.settings_mode == 2)
      {
        while(xSemaphoreTake(settings.mutex, portMAX_DELAY) != pdTRUE){}
        settings.temperature.needs_calibration = true;
        Settings::save(settings);
        xSemaphoreGive(settings.mutex);
      }
      else if(state.app.settings_mode == 3)
      {
        sensors.calibrateCo2() ;
      }
      state.app.settings_opened = false;
    }
    xSemaphoreGive(state.mutex);
  }
  else if(btn.click())
  {
    if(!state.app.settings_opened)
    {
      return;
    }
    while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
    state.app.settings_mode = (state.app.settings_mode + 1) % 4;
    state.app.settings_dirty = true;
    xSemaphoreGive(state.mutex);
  }
}

#ifdef ENABLE_TUYA
void tuyaTask(void* pvParameters)
{
  bool success = false;
  int seconds = TUYA_UPDATE_SECONDS - 1;
  while(1)
  {
    seconds++;
    if(!success || seconds == TUYA_UPDATE_SECONDS)
    {
      seconds = 0;
      success = tuyaClient.sync(state, settings);
    }
    esp_task_wdt_reset();
    delay(1000);
  }
}
#endif
