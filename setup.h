/*
#define DUSTSENS_LED 32
#define DUSTSENS_VOUT_PIN 35
#define DUSTSENS_SAMPL_DELAY 28 28ms from datasheet of sensor
#define LIGHT_SENS_PIN 34*/

/* One wire pin of DHT22 */
#define DHT_SENS_PIN 13
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321

/* One wire pin of DS18B20 */
#define ONE_WIRE_BUS 26
#define TEMPERATURE_PRECISION 12

#define MAIN_CYCLE_MS 50
#define UPDATESENSORS_INTERV (10*1000)
#define POST_TO_SERVER_INTERV (4*60*1000)
/*#define UPDATELIGHTSENS_INTERV (100)
*/
#define DBG_SER Serial
