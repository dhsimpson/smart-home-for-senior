//EEPROM 헤더, 도어락 비밀번호가 갱신될 때마다 EEPROM에 저장해 도어락이 꺼져도 저장된 비밀번호를 사용할 수 있다.
#include <EEPROM.h>
#define EEPROM_SIZE 5

// AWS 헤더
#include <AWS_IOT.h>
#include <WiFi.h>
#include <ArduinoJson.h>

// Time 헤더, NTP 서버로 부터 현재 시간을 얻어온다.
#include <NTPClient.h>
#include <WiFiUdp.h>

// AWS 전역 설정
AWS_IOT team5_Door; // AWS_IOT instance
char WIFI_SSID[]="dhsimpson";
char WIFI_PASSWORD[]= "핫스팟 비밀번호";
char HOST_ADDRESS[]="afkhvrjlvs6fo-ats.iot.ap-northeast-2.amazonaws.com"; // at Thing's Interact HTTPS
char CLIENT_ID[]= "Door";
char TOPIC_NAME_update[]= "$aws/things/Door/shadow/update";
char TOPIC_NAME_DoorPW[]= "team5/Door";

int status = WL_IDLE_STATUS;
int tick = 0, msgCount = 0, msgReceived = 0;
char payload[512];
char rcvdPayload[512];


// 도어락 회로도 전역 설정

const int btn = 16;
const int openLed = 21;
const int closeLed = 23;

const int freq = 5000;
const int ledChannel1 = 0;
const int resolution = 8;

bool btnFlag = false; // false is stop led
bool doorFlag = false; // false is stop led

unsigned long startTime = 0;
unsigned long pressedTime = 0;
unsigned long lastPressedTime = 0;
unsigned long debounceDelay = 500;

unsigned long touchTime = 0;
unsigned long lastTouchedTime = 0;

int passWord[4] = {1,2,3,4}; //터치패드 초기 비밀번호, 비밀번호를 바꾸면 이 배열과 EEPROM에 비밀번호가 저장된다.
int touchValues[4] = {0, 0, 0, 0}; // 터치패드 입력 배열
int touchIdx = 0;

// 도어락 터치 센서 핀 번호
const int touchPin1 = 32;
const int touchPin2 = 33;
const int touchPin3 = 27;
const int touchPin4 = 14;
const int touchPin5 = 12;
const int touchPin6 = 4;
const int touchPin7 = 13;
//const int touchPin8 = 26;
const int touchPin9 = 15;

// 도어락 터치 센서 입력 값
const int touchValue1 = 1;
const int touchValue2 = 2;
const int touchValue3 = 3;
const int touchValue4 = 4;
const int touchValue5 = 5;
const int touchValue6 = 6;
const int touchValue7 = 7;
//const int touchValue8 = 8;
const int touchValue9 = 9;

// 도어락 터치 센서 플레그
uint16_t val1 = 0;
uint16_t val2 = 0;
uint16_t val3 = 0;
uint16_t val4 = 0;
uint16_t val5 = 0;
uint16_t val6 = 0;
uint16_t val7 = 0;
uint16_t val9 = 0;

// time 전역 설정
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

//비밀번호 변화 콜백 함수
void callBackDoorPW(char *topicName, int payloadLen, char *payLoad)
{
  strncpy(rcvdPayload,payLoad,payloadLen);
  rcvdPayload[payloadLen] = 0;
  msgReceived = 1;
}



//버튼 인터럽트 핸들러
void IRAM_ATTR detectBtn(){
  pressedTime = millis();
  if(pressedTime - lastPressedTime >= debounceDelay){
    if(btnFlag){
      if( passWord[0] == touchValues[0] && passWord[1] == touchValues[1] && passWord[2] == touchValues[2] && passWord[3] == touchValues[3] ){
        doorFlag = true;
        Serial.print(touchValues[0]);
        Serial.print(" ");
        Serial.print(touchValues[1]);
        Serial.print(" ");
        Serial.print(touchValues[2]);
        Serial.print(" ");
        Serial.println(touchValues[3]);
      }else{
        doorFlag = false;
        Serial.print(touchValues[0]);
        Serial.print(" ");
        Serial.print(touchValues[1]);
        Serial.print(" ");
        Serial.print(touchValues[2]);
        Serial.print(" ");
        Serial.println(touchValues[3]);
      }
    }
    btnFlag = !btnFlag;
    lastPressedTime = pressedTime;
  }//추가로, 버튼이 FALSE 에서 TRUE 로 TOGGLE 됐을 때를 비밀번호 입력 완료 시점으로 본다.
}

void setup() {
  Serial.begin(115200);
  //EEPROM 셋팅
  EEPROM.begin(EEPROM_SIZE);
  Serial.print("EEPROM(0): ");
  Serial.println(EEPROM.read(0));
  if(EEPROM.read(0) == int('#')){
    passWord[0] = EEPROM.read(1);
    passWord[1] = EEPROM.read(2);
    passWord[2] = EEPROM.read(3);
    passWord[3] = EEPROM.read(4);
  }
  //AWS 셋팅
   while (status != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(WIFI_SSID);
  // Connect to WPA/WPA2 network. Change this line if using open or WEP network:
    status = WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  // wait 5 seconds for connection:
    delay(5000);
  }
  Serial.println("Connected to wifi");
  
  if(team5_Door.connect(HOST_ADDRESS,CLIENT_ID)== 0) { // Connect to AWS
    Serial.println("Connected to AWS");
    delay(1000);
    
    //안드로이드로부터의 Door newPW message Subscribe
    if(0==team5_Door.subscribe(TOPIC_NAME_DoorPW,callBackDoorPW))
      Serial.println("Subscribe( Door PW ) Successfull");
    else {
      Serial.println("Subscribe( Door PW ) Failed, Check the Thing Name, Certificates");
      while(1);
    }
  }
  else {
    Serial.println("AWS connection failed, Check the HOST Address");
    while(1);
  }
  delay(2000);

  // 도어락 시간 셋팅
  btnFlag = false; // false is stop led
  startTime = millis();
  pressedTime = startTime;
  lastPressedTime = startTime;
  
  touchTime = millis();
  lastTouchedTime = millis();

  ledcSetup(ledChannel1, freq, resolution);
  ledcAttachPin(openLed, ledChannel1);
  pinMode(btn, INPUT);
  pinMode(closeLed, OUTPUT);
  attachInterrupt(digitalPinToInterrupt(btn), detectBtn, RISING);

  // time 셋팅
  timeClient.begin();
  timeClient.setTimeOffset(3600);
//  RTC.begin();
//  RTC.adjust(DateTime(2019, 12, 10, 11, 52, 0));
}
void loop() {
  //안드로이드 APP에서 새로운 비밀번호 설정을 하면 Lambda를 통해 MQTT 메시지가 들어 온다. 
  //MQTT메시지로 부터 받은 비밀번호를 EEPROM에 저장한다.
  StaticJsonDocument<200> msg; // reserve stack mem for handling json msg
  if(msgReceived == 1) {
    msgReceived = 0;
    Serial.print("Received Message(Update):");
    Serial.println(rcvdPayload);
    // Deserialize the JSON document
    if (deserializeJson(msg, rcvdPayload)) { // if error
      Serial.print(F("deserializeJson() failed.. \n"));
      while(1);
    }

    String Data = msg["PW"];
    Serial.print("flag : " );
    Serial.println(Data);
    if(Data[4]=='C'){
      String newPw = Data;
      Serial.print("Change PW : ");
      Serial.println(newPw);
      //새로운 비밀번호를 EEPROM에 저장 하는 기능 추가해야 함
      EEPROM.write(0, int('#'));
      EEPROM.write(1, newPw[0]-48);
      EEPROM.write(2, newPw[1]-48);
      EEPROM.write(3, newPw[2]-48);
      EEPROM.write(4, newPw[3]-48);
      EEPROM.commit();
      passWord[0] = EEPROM.read(1);
      passWord[1] = EEPROM.read(2);
      passWord[2] = EEPROM.read(3);
      passWord[3] = EEPROM.read(4);
      Serial.print(passWord[0]);
      Serial.print(passWord[1]);
      Serial.print(passWord[2]);
      Serial.print(passWord[3]);
      Serial.println("");
    }else if (Data[4]=='O') {
      //안드로이드 APP에서 문열기 버튼을 클릭했을 때, Lambda 함수를 거쳐 MQTT 메시지가 전송된다. 
      //MQTT 메시지에 담긴 비밀번호와 EEPROM에 담긴 비밀번호를 비교해 도어락 여닫기를 결정한다.
      String openPw = Data;
      Serial.print("Open Door PW : ");
      Serial.println(openPw);
      if(passWord[0] == openPw[0]-48 && passWord[1] == openPw[1]-48 && passWord[2] == openPw[2]-48 && passWord[3] == openPw[3]-48){
      digitalWrite(closeLed, LOW);
      ledcWrite(ledChannel1, 0);
      delay(500);
      ledcWrite(ledChannel1, 64);
      delay(500);
      ledcWrite(ledChannel1, 128);
      delay(500);
      ledcWrite(ledChannel1, 256);
      delay(500);
      doorFlag = false;
      //문 열었을 때, Door 사물 섀도우에 문 열린 상태 등록
      timeClient.update();
      unsigned long tStamp = timeClient.getEpochTime();
      sprintf(payload,"{\"state\":{\"reported\":{\"door\":\"open\",\"openTime\":%lu}}}",tStamp);
      if(team5_Door.publish(TOPIC_NAME_update,payload)==0){
        Serial.print("Publish Message:");
        Serial.println(payload);
      }else{
        Serial.print("Publish failed:");
        Serial.println(payload);
      }
      vTaskDelay(5000 / portTICK_RATE_MS);
      }else{
        ledcWrite(ledChannel1, 0);
        digitalWrite(closeLed, HIGH);
      }
    }
  }
    
  
  if(btnFlag){
    // 인터럽트를 적용한 버튼을 클릭해 버튼 플래그가 1이 되면 터치패드이용할 수 있다. 
    //터치패드 마다 터치 시간 플레그 적용
    val1= touchRead(touchPin1);
    val2= touchRead(touchPin2);
    val3= touchRead(touchPin3);
    val4= touchRead(touchPin4);
    val5= touchRead(touchPin5);
    val6= touchRead(touchPin6);
    val7= touchRead(touchPin7);
    val9= touchRead(touchPin9);
    touchTime = millis();
    
    if( touchIdx<=3 && touchTime - lastTouchedTime >= debounceDelay ){
      if ( 10 <= val1 &&val1 <= 20 ){
        touchValues[touchIdx++] = touchValue1;
        Serial.print(val1);
        Serial.print(" : ");
        Serial.println(touchValue1);
        lastTouchedTime = touchTime;
        return;
      }else if ( 10 <= val2 &&val2 <= 20 ){
        touchValues[touchIdx++] = touchValue2;
        Serial.print(val2);
        Serial.print(" : ");
        Serial.println(touchValue2);
        lastTouchedTime = touchTime;
        return;
      }else if ( 10 <= val3 &&val3 <= 20 ){
        touchValues[touchIdx++] = touchValue3;
        Serial.print(val3);
        Serial.print(" : ");
        Serial.println(touchValue3);
        lastTouchedTime = touchTime;
        return;
      }else if ( 10 <= val4 &&val4 <= 20 ){
        touchValues[touchIdx++] = touchValue4;
        Serial.print(val4);
        Serial.print(" : ");
        Serial.println(touchValue4);
        lastTouchedTime = touchTime;
        return;
      }else if ( 10 <= val5 &&val5 <= 20 ){
        touchValues[touchIdx++] = touchValue5;
        Serial.print(val5);
        Serial.print(" : ");
        Serial.println(touchValue5);
        lastTouchedTime = touchTime;
        return;
      }else if ( 10 <= val6 &&val6 <= 20 ){
        touchValues[touchIdx++] = touchValue6;
        Serial.print(val6);
        Serial.print(" : ");
        Serial.println(touchValue6);
        lastTouchedTime = touchTime;
        return;
      }else if ( 10 <= val7 &&val7 <= 20 ){
        touchValues[touchIdx++] = touchValue7;
        Serial.print(val7);
        Serial.print(" : ");
        Serial.println(touchValue7);
        lastTouchedTime = touchTime;
        return;
      }else if ( 10 <= val9 &&val9 <= 20 ){
        touchValues[touchIdx++] = touchValue9;
        Serial.print(val9);
        Serial.print(" : ");
        Serial.println(touchValue9);
        lastTouchedTime = touchTime;
        return;
      }
    }
  }else{
    // 터치센서 값 저장하는 배열을 초기화
    touchValues[0] = touchValues[1] = touchValues[2] = touchValues[3] = 0;
    touchIdx = 0; 
  }
  if(doorFlag){ //doorFlag 는 터치 센서 콜백함수에서
    digitalWrite(closeLed, LOW);
    ledcWrite(ledChannel1, 0);
    delay(500);
    ledcWrite(ledChannel1, 64);
    delay(500);
    ledcWrite(ledChannel1, 128);
    delay(500);
    ledcWrite(ledChannel1, 256);
    delay(500);
    doorFlag = false;
    //문 열었을 때, Door 사물 섀도우에 등록
    timeClient.update();
    unsigned long tStamp = timeClient.getEpochTime();
    
    sprintf(payload,"{\"state\":{\"reported\":{\"door\":\"open\",\"openTime\":%lu}}}",tStamp);
    if(team5_Door.publish(TOPIC_NAME_update,payload)==0){
      Serial.print("Publish Message:");
      Serial.println(payload);
    }else{
      Serial.print("Publish failed:");
      Serial.println(payload);
    }
    vTaskDelay(5000 / portTICK_RATE_MS);
  }else{
    ledcWrite(ledChannel1, 0);
    digitalWrite(closeLed, HIGH);
  }

}
