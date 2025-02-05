#ifndef Network_H_
#define Network_H_
#include <Arduino.h>
#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"

#define WIFI_SSID "<YOUR WIFI SSID>"
#define WIFI_PASSWORD "<YOUR WIFI PASSWORD>"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCRUe_9DE9PucND3zL32u9oCIVdUg_hp3o"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://smartlock-a7ffe-default-rtdb.firebaseio.com/" 

#define FIREBASE_PROJECT_ID "smartlock-a7ffe"


class Network{
private:
  FirebaseData fbdo;
  FirebaseAuth auth;
  FirebaseConfig config;

  void firebaseInit();
  friend void WiFiEventConnected(WiFiEvent_t event, WiFiEventInfo_t info);
  friend void WiFiEventGotIP(WiFiEvent_t event, WiFiEventInfo_t info);
  friend void WiFiEventDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);
  friend void FirestoreTokenStatusCallback(TokenInfo info);

public:
  Network();
  void initWiFi();
  void firestoreDataUpdate(String lock, String rfid, String mode, String datetime);
  bool firestoreSearchRFID(String rfid);
};

static Network *instance = NULL;

Network::Network(){
  instance = this;
}

void WiFiEventConnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WIFI CONNECTED! BUT WAIT FOR THE LOCAL IP ADDR");
}

void WiFiEventGotIP(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.print("LOCAL IP ADDRESS: ");
  Serial.println(WiFi.localIP());
  instance->firebaseInit();
}

void WiFiEventDisconnected(WiFiEvent_t event, WiFiEventInfo_t info){
  Serial.println("WIFI DISCONNECTED!");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

void FirestoreTokenStatusCallback(TokenInfo info){
  Serial.printf("Token Info: type = %s, status = %s\n", getTokenType(info), getTokenStatus(info));
}

void Network::initWiFi(){
  WiFi.disconnect();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  instance->firebaseInit();
}

void Network::firebaseInit(){
  config.api_key = API_KEY;

//   auth.user.email = USER_EMAIL;
//   auth.user.password = USER_PASSWORD;

  config.token_status_callback = FirestoreTokenStatusCallback;
  if (Firebase.signUp(&config, &auth, "", "")){
    Serial.println("ok");

  }
  else{
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }
  Firebase.begin(&config, &auth);
  Serial.println("Firebase Connected");
}

bool Network::firestoreSearchRFID(String rfid){
    if(WiFi.status() == WL_CONNECTED && Firebase.ready()){
        String documentPath = "rfids";
        
        if(Firebase.Firestore.getDocument(&fbdo,FIREBASE_PROJECT_ID,"",documentPath.c_str(),"")){
            Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
            return fbdo.payload().indexOf(rfid) != -1;
        }else{
            Serial.println(fbdo.errorReason());
            return false;
        }
    }
}

void Network::firestoreDataUpdate(String lock, String rfid, String mode, String datetime){
  if(WiFi.status() == WL_CONNECTED && Firebase.ready()){
    char buffer[1024];
    sprintf(buffer,"Logs/%s_%s",lock.c_str(),datetime.c_str());
    String documentPath = buffer;

    FirebaseJson content;

    content.set("fields/RFID/stringValue", rfid.c_str());
    content.set("fields/AccessMode/stringValue", mode.c_str());
    // content.set("field/AccessDateTime/stringValue", datetime.c_str());

    if(Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "RFID,AccessMode")){
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      return;
    }else{
      Serial.println(fbdo.errorReason());
    }

    if(Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())){
      Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
      return;
    }else{
      Serial.println(fbdo.errorReason());
    }
  }
}

#endif










