class HAentity {
#define POST_REQ_PAYLOAD_MAXLEN 200

  private:
    char restRequestData[POST_REQ_PAYLOAD_MAXLEN];
    uint8_t respCode = 0;
    
  public:
    char* APIendpointUrl;

    double state = -999.99;
    boolean isMeasurement = true;
    char friendlyName[30] = "";
    char unitOfMeasurement[5] = "";
    char icon[20] = "";
    char devClass[30] = "";

    HAentity(char* friendNm, char* unitOfMeas, char* haIcon, char* devClss, boolean measurement) {
      snprintf(friendlyName, 30, friendNm);
      snprintf(unitOfMeasurement, 5, unitOfMeas);
      snprintf(icon, 20, haIcon);
      snprintf(devClass, 30, devClss);
      isMeasurement = measurement;
    }

    void setVal(double newState) {
      state = newState;
    }
    char* getRestPayload() {
      /* update restRequestData string with new sensor data, before returning string */
      snprintf(restRequestData, (POST_REQ_PAYLOAD_MAXLEN), "{\"state\":%d.%d, \"attributes\": {\"friendly_name\":\"%s\",\"unit_of_measurement\":\"%s\",\"icon\":\"%s\",\"device_class\":\"%s\",\"is_measurement\":\"%s\", \"state_class\":\"measurement\"}}", (int16_t)state, (((int16_t)(abs(state) * 100)) % 100), friendlyName, unitOfMeasurement, icon, devClass, (isMeasurement ? "True" : "False"));
      return restRequestData;
    }

    int8_t postToHA(WiFiClient* wifiCli, HTTPClient* http) {

      if(state == -999.99){
        return -1;
      }
      http->begin(*wifiCli, this->APIendpointUrl);
      http->addHeader("Content-Type", "application/json");
      http->addHeader("Authorization", HOMEASSISTANT_TOKEN);

      respCode = http->POST(this->getRestPayload());
      http->end();

      if(respCode != 200){
         DBG_SER.println("ResponseCode: " + String(respCode));
         /*DBG_SER.println("ResponseStr: " + http->getString());*/
         return -1;
      }
      else{
        DBG_SER.print("POST | ");
        DBG_SER.print(friendlyName);
        DBG_SER.println(" \t\t[ OK ]");
      }
      return 0;
    }
};
