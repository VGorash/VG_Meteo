#ifndef METEO_API_H
#define METEO_API_H

#define WEATHER_API_KEY "******"

#include "State.h"
#include "Settings.h"

#include <Arduino_JSON.h>
#include <HTTPClient.h>

class MeteoAPI
{
public:
  MeteoAPI(){}

  bool sync(State& state, Settings& settings)
  {
    while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
    bool wifiAvailable = state.app.wifi_available;
    xSemaphoreGive(state.mutex);

    if(!wifiAvailable)
    {
      return false;
    }

    String responce = httpGet("http://ip-api.com/json");
    if (responce == "")
    {
      return false;
    }
    JSONVar object = JSON.parse(responce);
    String lat = JSON.stringify(object["lat"]);
    String lon = JSON.stringify(object["lon"]);
    responce = httpGet(String("https://api.openweathermap.org/data/2.5/weather?units=metric&appid=") + String(WEATHER_API_KEY) + String("&lat=") + lat + String("&lon=") + lon);
    if (responce == "")
    {
      return false;
    }
    JSONVar weather = JSON.parse(responce);
    float temperature = JSON.stringify(weather["main"]["temp"]).toFloat();
    float windSpeed = JSON.stringify(weather["wind"]["speed"]).toFloat();
    int windDirection = JSON.stringify(weather["wind"]["deg"]).toInt();
    int clouds = JSON.stringify(weather["clouds"]["all"]).toInt();
    long dt = JSON.stringify(weather["dt"]).toInt();
    long sunrise = JSON.stringify(weather["sys"]["sunrise"]).toInt();
    long sunset = JSON.stringify(weather["sys"]["sunset"]).toInt();
    int isDay = (dt > sunrise) && (dt < sunset) ? 1 : 0;

    WeatherCode weatherCode;
    int weatherCodeRaw = JSON.stringify(weather["weather"][0]["id"]).toInt();
    switch(weatherCodeRaw)
    {
      //CLEAR,
      case 800:
      {
        weatherCode = WeatherCode::CLEAR;
        break;
      }
      //PARLY_CLOUDY,
      case 801:
      {
        weatherCode = WeatherCode::PARLY_CLOUDY;
        break;
      }
      //CLOUDY,
      case 802:
      case 803:
      {
        weatherCode = WeatherCode::CLOUDY;
        break;
      }
      //OVERCAST,
      case 804:
      {
        weatherCode = WeatherCode::OVERCAST;
        break;
      }
      //MIST,
      case 701:
      case 711:
      case 721:
      case 731:
      case 741:
      case 751:
      case 761:
      case 762:
      {
        weatherCode = WeatherCode::MIST;
        break;
      }
      //PARTLY_RAIN,
      //RAIN,
      case 300:
      case 301:
      case 302:
      case 310:
      case 311:
      case 312:
      case 313:
      case 321:
      case 520:
      case 521:
      case 500:
      case 501:
      {
        if(clouds <=30)
        {
          weatherCode = WeatherCode::PARTLY_RAIN;
        }
        else
        {
          weatherCode = WeatherCode::RAIN;
        }
        break;
      }
      //HEAVY_RAIN
      case 314:
      case 502:
      case 503:
      case 504:
      case 522:
      case 531:
      {
        weatherCode = WeatherCode::HEAVY_RAIN;
        break;
      }
      //PARTLY_SNOW,
      //SNOW,
      case 600:
      case 601:
      case 602:
      case 620:
      case 621:
      case 622:
      {
        if(clouds <=30)
        {
          weatherCode = WeatherCode::PARTLY_SNOW;
        }
        else
        {
          weatherCode = WeatherCode::SNOW;
        }
        break;
      }
      //SLEET,
      case 511:
      case 611:
      case 612:
      case 613:
      case 615:
      case 616:
      {
        weatherCode = WeatherCode::SLEET;
        break;
      }
      //THUNDER,
      case 221:
      case 210:
      case 211:
      case 212:
      case 771:
      case 781:
      {
        weatherCode = WeatherCode::THUNDER;
        break;
      }
      //THUNDER_RAIN
      case 200:
      case 201:
      case 202:
      case 230:
      case 231:
      case 232:
      {
        weatherCode = WeatherCode::THUNDER_RAIN;
        break;
      }
    }

    while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
    state.weather.temperature = temperature;
    state.weather.weather_code = weatherCode;
    state.weather.wind_speed = windSpeed;
    state.weather.wind_direction = windDirection;
    state.weather.is_day = isDay;
    state.weather.initialized = true;
    xSemaphoreGive(state.mutex);
    return true;
  }

private:
  String httpGet(String url)
  {
    Serial.println(url);
    HTTPClient http;
    http.begin(url.c_str());
    int httpCode = http.GET();
    if(httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_MOVED_PERMANENTLY)
    {
      Serial.print("Error code: ");
      Serial.println(httpCode);
      return "";
    }
    String payload = http.getString();
    http.end();
    return payload;
  }
};

#endif