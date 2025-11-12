#include "WifiHelper.h"

void WifiHelper::begin(State& state, Settings& settings)
{
  if(settings.wifi.initialized)
  {
    Serial.println("Wifi initialized");
    WiFi.begin(settings.wifi.ssid, settings.wifi.password);
  }
  else
  {
    Serial.println("Wifi not initialized");
    IPAddress AP_LOCAL_IP(192, 168, 4, 1);
    IPAddress AP_GATEWAY_IP(192, 168, 1, 254);
    IPAddress AP_NETWORK_MASK(255, 255, 255, 0);

    WiFi.mode(WIFI_MODE_APSTA);
    WiFi.softAP("VG_Meteo", "12345678");
    Serial.println("AP started");
    WiFiServer server(80);
    server.begin();
    Serial.println("Server started");
    while (1)
    {
      WiFiClient client = server.available();
      if (handleClient(client, settings))
      {
        break;
      }
    }
    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_MODE_STA);
  }
}

void WifiHelper::sync(State& state, Settings& settings)
{
  if(WiFi.status() != WL_CONNECTED) // wifi not available
  {
    if(state.app.wifi_available)
    {
      while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
      state.app.wifi_available = false;
      xSemaphoreGive(state.mutex);
    }
    WiFi.mode(WIFI_MODE_NULL);
    WiFi.mode(WIFI_MODE_STA);
    WiFi.reconnect();
    waitForConnect(10);
  }
  else // wifi available
  {
    if(!state.app.wifi_available)
    {
      while(xSemaphoreTake(state.mutex, portMAX_DELAY) != pdTRUE){}
      state.app.wifi_available = true;
      xSemaphoreGive(state.mutex);
    }
  }
}

bool WifiHelper::waitForConnect(int seconds)
{
  int numTries = 0;
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    numTries++;
    if(numTries == seconds)
    {
      return false;
    }
  }
  return true;
}

//// AP mode ////
void success(NetworkClient& client)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html; charset:utf-8");
  client.println("Connection: close");
  client.println();
  client.println("<!DOCTYPE html><html>");

  client.println("<head>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("</head>");

  client.println("<body>");
  client.println("<h1>Wi-Fi connected!</h1>");
  client.println("</body>");
  client.println("</html>");
  client.println();
}

void form(NetworkClient& client, bool error)
{
  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html; charset:utf-8");
  client.println("Connection: close");
  client.println();

  // Main HTML code
  client.println("<!DOCTYPE html><html>");

  client.println("<head>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("</head>");

  client.println("<body>");
  client.println("<h1>Wi-Fi</h1>");
  if(error)
  {
    client.println("<h3>Failed to connect. Please try again</h3>");
  }
  client.println("<form method=\"get\">");
  client.println("<div>");
  client.println("<label for=\"ssid\"> SSID </label>");
  client.println("<input name=\"ssid\" id=\"ssid\"/>");
  client.println("</div>");
  client.println("<div>");
  client.println("<label for=\"pass\"> Password </label>");
  client.println("<input name=\"pass\" id=\"pass\"/>");
  client.println("</div>");
  client.println("<div>");
  client.println("<button type=\"submit\"> Connect </button>");
  client.println("</div>");
  client.println("</form>");
  client.println("</body>");

  client.println("</html>");

  // The HTTP response ends with another blank line
  client.println();
}

void notFound(NetworkClient& client)
{
  client.println("HTTP/1.1 404 Not Found");
  client.println("Connection: close");
  client.println();
}

bool WifiHelper::handleClient(NetworkClient& client, Settings& settings){
  if (!client)
  {
    return false;
  }
  Serial.println("Handle");
  String header = "";
  String currentLine = "";
  while (client.connected())
  {
    if (!client.available())
    {
      continue;
    }
    char c = client.read();
    header += c;
    if (c == '\n')
    {
      // if the current line is blank, you got two newline characters in a row.
      // that's the end of the client HTTP request, so send a response:
      if (currentLine.length() == 0)
      {
        if (header.indexOf("GET /?") >= 0)
        {
          Serial.println("GET /?");
          int idx = header.indexOf("GET /?");
          int idx2 = header.indexOf(" HTTP/", idx);
          String key = "";
          String value = "";
          String temp = "";
          String ssid = "";
          String pass = "";
          for (int i = idx + 6; i <= idx2; i++)
          {
            char c = header.c_str()[i];
            if(c == '&' || i == idx2)
            {
              value = temp;
              temp = "";
              if(key == "ssid")
              {
                ssid = value;
              }
              if(key == "pass")
              {
                pass = value;
              }
            }
            else if(c == '=')
            {
              key = temp;
              temp = "";
            }
            else
            {
              temp += c;
            }
          }
          Serial.print("SSID: ");
          Serial.println(ssid);
          Serial.print("PASSWORD: ");
          Serial.println(pass);
          if (ssid == "" || pass == "")
          {
            form(client, true);
            return false;
          }
          WiFi.begin(ssid, pass);
          if (!waitForConnect(10))
          {
            form(client, true);
            WiFi.mode(WIFI_MODE_AP);
            WiFi.mode(WIFI_MODE_APSTA);
            return false;
          }
          else
          {
            success(client);
            client.stop();
            delay(1000);
            strcpy(settings.wifi.ssid, ssid.c_str());
            strcpy(settings.wifi.password, pass.c_str());
            settings.wifi.initialized = true;
            Settings::save(settings);
            return true;
          }
        }
        if (header.indexOf("GET / ") >= 0)
        {
          Serial.println("GET /");
          form(client, false);
          return false;
        }
        Serial.println("404");
        notFound(client);
        return false;
      }
      else
      {
        // if you got a newline, then clear currentLine
        currentLine = "";
      }
    }
    else if (c != '\r') 
    {
      // if you got anything else but a carriage return character,
      currentLine += c;
    }
  }
  return false;
}
