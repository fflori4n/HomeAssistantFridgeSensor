#include "secrets.h"
#include "setup.h"
#include <HTTPClient.h>
#include "haRestSensor.h"
#include <WiFi.h>
#define DHT_DEBUG 1
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>

/* sensor uses DS18b20 sensor for measuring fridge temp. */
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
DeviceAddress insideFridge = {0x28, 0xBF, 0x89, 0x80, 0xE3, 0xE1, 0x3C, 0x53};

/* HA entities for sensors */
HAentity tempSensor_0("freezer temp", "°C", "mdi:thermometer", "temperature", true);
HAentity tempSensor_1("fridge temp", "°C", "mdi:thermometer", "temperature", true);
HAentity humiSensor_0("freezer humidity", "%%", "mdi:water-percent", "humidity", true);

/* wifi SSID and PASS */
const char* ssid = SECR_WIFI_SSID;
const char* password = SECR_WIFI_PASS;

WiFiClient Client;
HTTPClient http;

/* define DHT sensor */
DHT dht(DHT_SENS_PIN, DHTTYPE);

uint16_t sensorUpdatesSent = 0;
uint16_t sensReadTimer = (UPDATESENSORS_INTERV / MAIN_CYCLE_MS);
uint16_t postTimer = (POST_TO_SERVER_INTERV / MAIN_CYCLE_MS);
/*uint16_t lightSensReadTimer = (UPDATELIGHTSENS_INTERV / MAIN_CYCLE_MS);
*/
uint16_t retryDelay = 0;

void setup() {

  DBG_SER.begin(115200);
  dht.begin();

  /* init DS18B20 sensors */
  sensors.begin();

  /* print DS18B20 sensors that are present on bus */
  Serial.print("Locating devices...");
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");
  
  Serial.print("Parasite power is: ");
  if (sensors.isParasitePowerMode()) Serial.println("ON");
  else Serial.println("OFF");

  
  if (!sensors.getAddress(insideFridge, 0)) Serial.println("Unable to find address for Device 0");
  /* set precision of DS18B20 */
  sensors.setResolution(insideFridge, TEMPERATURE_PRECISION);

  Serial.print("Device 0 Resolution: ");
  Serial.print(sensors.getResolution(insideFridge), DEC);
  Serial.println();



  /* HA server endpoints are exposed in firewall settings to be accessible from outside network on 8123 */
  tempSensor_0.APIendpointUrl = "http://dummyHomeAssistantServerURL.net:8123/api/states/sensor.rest_tempSensor0";
  tempSensor_1.APIendpointUrl = "http://dummyHomeAssistantServerURL.net:8123/api/states/sensor.rest_tempSensor1";
  humiSensor_0.APIendpointUrl = "http://dummyHomeAssistantServerURL.net:8123/api/states/sensor.rest_humiSensor0";

  /* wait for WIFI Connected */
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("CONNECTED");
}

int8_t readDht(double &temperatureVar, double &humidityVar) {

  double newHumidity = 0, newTemperature = 0;

#define IN_RANGE(VAL, MIN, MAX) ((VAL >= MIN) && (VAL <= MAX))
#define FILTER_COMBINE_READINGS (1.0/6)
#define MAX_DHT_READ_TRY 5

  uint8_t readOKFlag = 0x00;  /* | HUMIDITY READ | TEMP READ |*/

  for (int retry = 0; ((retry < MAX_DHT_READ_TRY) && (readOKFlag != 0x03)); retry++) {

    if ((readOKFlag & 0x01) == 0) {
      newTemperature = dht.readTemperature();
      Serial.println(newTemperature);
      /*newTemperature = ~(newTemperature & 0x7FFF);*/
      Serial.println(newTemperature);
      if ((!isnan(newTemperature)) && IN_RANGE(newTemperature, -40.0, 80.0)) {
        Serial.println("setting temp.");

        if (temperatureVar == -999.99) {
          temperatureVar = newTemperature;
        }
        else {
          temperatureVar = (((1.0 - FILTER_COMBINE_READINGS) * temperatureVar) + ((FILTER_COMBINE_READINGS) * newTemperature));
        }
        readOKFlag |= 0x01;
      }
    }

    if ((readOKFlag & 0x02) == 0) {
      newHumidity = dht.readHumidity();
      if ((!isnan(newHumidity)) && IN_RANGE(newHumidity, 0.0, 100.0)) {

        if (humidityVar == -999.99) {
          humidityVar = newHumidity;
        }
        else {
          humidityVar = (((1.0 - FILTER_COMBINE_READINGS) * humidityVar) + ((FILTER_COMBINE_READINGS) * newHumidity));
        }
        readOKFlag |= 0x02;
      }
    }

  }

#define DBG_DHT_VERBOSE
#ifdef DBG_DHT_VERBOSE
  if (readOKFlag == 0x03) {
    Serial.print(F("DHT| [ OK ]"));
  }
  else {
    Serial.print(F("DHT| [ ER ]"));
    (readOKFlag == 0) ? Serial.println(F(" Temp & humidity not read.")) : ((readOKFlag == 1) ? Serial.println(F(" Humidity not read.")) : Serial.println(F(" Temp not read.")));
  }
  Serial.print(F("Temperature: "));
  Serial.print(temperatureVar);
  Serial.print(F("°C "));
  Serial.print(F("Humidity: "));
  Serial.print(humidityVar);
  Serial.println(F(" %"));
#endif

  if (readOKFlag == 0x03) {
    return 0;
  }
  return -1;
}

int8_t readDS18B20(double &temperatureVar){
  delay(1000);
  sensors.requestTemperatures();
    float newTemperature = sensors.getTempC(insideFridge);
    if (newTemperature == DEVICE_DISCONNECTED_C)
    {
      Serial.println("Error: Could not read temperature data");
      return -1;
    }

    if (temperatureVar == -999.99) {
          temperatureVar = newTemperature;
        }
        else {
          temperatureVar = (((1.0 - FILTER_COMBINE_READINGS) * temperatureVar) + ((FILTER_COMBINE_READINGS) * newTemperature));
        }
Serial.print("Temp C: ");
    Serial.println(temperatureVar);
    return 0;
    
}
void loop() {

  /* Check if it is time to read DHT and DS18B20 sensors */
  if ((sensReadTimer * MAIN_CYCLE_MS) >= UPDATESENSORS_INTERV) {
    Serial.print("reading dht");
    if (readDht(tempSensor_0.state, humiSensor_0.state) == 0 && readDS18B20(tempSensor_1.state) == 0) {
      sensReadTimer = 0;
    }
  }
  else {
    sensReadTimer++;
  }

  /* Try to send each entity to it's corresponding endpoint via POST*/
  if (sensorUpdatesSent != 0) {
    if ((sensorUpdatesSent & 0x01) && (tempSensor_0.postToHA(&Client, &http) == 0)) {
      sensorUpdatesSent &= ~0x01;
      delay(2000);
    }
    if ((sensorUpdatesSent & 0x02) && (humiSensor_0.postToHA(&Client, &http) == 0)) {
      sensorUpdatesSent &= ~0x02;
      delay(2000);
    }
    if ((sensorUpdatesSent & 0x04) && (tempSensor_1.postToHA(&Client, &http) == 0)) {
      sensorUpdatesSent &= ~0x04;
      delay(2000);
    }
    delay(2000);
    if (retryDelay >= 10) {
      sensorUpdatesSent = 0;
    }
    else {
      retryDelay++;
    }
    Serial.println("llop");
  }

  /* If all entities were send, reset READY TO SEND timer */
  if ((postTimer * MAIN_CYCLE_MS) >= POST_TO_SERVER_INTERV) {
    postTimer = 0;
    sensorUpdatesSent = 0;
    retryDelay = 0;
    sensorUpdatesSent |= ( 0x01 | 0x02 | 0x04);
  }
  else {
    postTimer++;
  }

  delay(MAIN_CYCLE_MS);
}
