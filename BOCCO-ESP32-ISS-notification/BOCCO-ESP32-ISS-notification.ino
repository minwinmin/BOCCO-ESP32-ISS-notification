#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

///////////////////////////////////////////////
//BOCCO API ライブラリ
#include "config.h"
#include "bocco_api.h"
//BOCCO アカウント情報
#define BOCCO_EMAIL     "xxxxxx"
#define BOCCO_PASSWORD  "xxxxxx"
#define BOCCO_API_KEY   "xxxxxx"
#define ACCESS_TOKEN    "xxxxxx"
BoccoAPI boccoAPI(BOCCO_EMAIL, BOCCO_PASSWORD, BOCCO_API_KEY);
///////////////////////////////////////////////


/////////////////////////////////////////////////
////BOCCO API ライブラリ
//#include "config.h"
//#include "bocco_api.h"
////BOCCO アカウント情報
//#define BOCCO_EMAIL     "takuro.mikami@gmail.com"
//#define BOCCO_PASSWORD  "24tnfowe34"
//#define BOCCO_API_KEY   "9ebc34c1b90e437b7e4d641b17f400ce00d7eab553b38f3625a39efd4650276a"
//#define ACCESS_TOKEN    "904306f1d55d76ff00bccf2a7c4a5da5de9bb29194d59b8f9d0c86f013fead3b"
//BoccoAPI boccoAPI(BOCCO_EMAIL, BOCCO_PASSWORD, BOCCO_API_KEY);
/////////////////////////////////////////////////

//初期設定の為のアクセスポイント名
#define AP_NAME "test"

//任意場所の緯度と経度
//35.691610, 139.712713
String LAT = "35.6918";
String LON = "139.71";
String API_URL = "http://api.open-notify.org/iss-pass.json?lat=" + LAT + "&lon=" + LON + "&alt=10&n=10";
//APIでかえってきたJSONを格納する
//容量は大きめに取る
StaticJsonDocument<600> doc;

//プッシュスイッチ
const int SW = 0;
int valueSW = HIGH;

//unix timeを日付に変換したものを格納する配列
int date[5];

//unix timeとは、UTC時刻における1970年1月1日午前0時0分0秒（UNIXエポック）からの経過秒数を表す
//JST(日本時間の場合)、これに9時間足したものになる。

//参考
//https://hackaday.io/page/4786-epoch-to-time-date-converter 参考
void unixTimeToDaytime(int unixtime, int timezone_h, int timezone_m){
  int year_month[12]={31,28,31,30,31,30,31,31,30,31,30,31};
  int leapyear_month[12]={31,29,31,30,31,30,31,31,30,31,30,31};
  
  int oneYear = 31536000;
  int oneDay = 86400;
  int oneHour = 3600;
  int oneMinute = 60;
  
  int fixedUnixTime = unixtime + timezone_h * 60 * 60 + timezone_m * 60;
  int ElapsedSecond = fixedUnixTime % 60;
  fixedUnixTime /= 60;
  int ElapsedMinute = fixedUnixTime % 60;
  fixedUnixTime /= 60;
  int ElapsedHours = fixedUnixTime % 24;
  fixedUnixTime /= 24;

  int ElapsedDays = fixedUnixTime;

  int ElapsedYears = 1970+(ElapsedDays/365);

  //1970年からの閏年を計算
  int i;
  int leap_days = 0;
  for(i=1972; i<ElapsedYears; i+=4){
    if(((i%4==0)&&(i%100!=0)) || (i%400==0)){
      leap_days++;
    }
  }

  int fixedElapsedYears = 1970+((ElapsedDays-leap_days)/365);
  //0日はないから1を足す
  int day_of_year = ((ElapsedDays-leap_days)%365)+1;
  int temp_days;
  if(((fixedElapsedYears%4==0)&&(fixedElapsedYears%100!=0)) || (fixedElapsedYears%400==0)){
      year_month[1]=29;
      int leap_year_ind = 1;
  }else{
    year_month[1]=28;
    temp_days=0;
  }
  
  int ElapsedMonths;
  int month_days;
  for(ElapsedMonths=0; ElapsedMonths<=11; ElapsedMonths++){
    if(day_of_year<=temp_days){
      break;
    }
    temp_days = temp_days + year_month[ElapsedMonths];
  }
  temp_days = temp_days - year_month[ElapsedMonths-1]; 
  int ElapsedDate = day_of_year - temp_days;

  Serial.println("ElapsedYears : ");
  Serial.println(ElapsedYears);
  date[0] = ElapsedYears;
  Serial.println("ElapsedMonths : ");
  Serial.println(ElapsedMonths);
  date[1] = ElapsedMonths;
  Serial.println("ElapsedDate : ");
  Serial.println(ElapsedDate);
  date[2] = ElapsedDate;
  Serial.println("ElapsedHours : ");
  Serial.println(ElapsedHours);
  date[3] = ElapsedHours;
  Serial.println("ElapsedMinute : ");
  Serial.println(ElapsedMinute);
  date[4] = ElapsedMinute;

}

//ボタン割り込み
void clickSw(){
  Serial.println("clickSw");
  valueSW = digitalRead(SW);
  Serial.println(valueSW);
}

void setup() {
  Serial.begin(115200);

  WiFiManager wifiManager;
  wifiManager.setTimeout(180);
  if(!wifiManager.autoConnect(AP_NAME)) {
    Serial.println("timeout");
    delay(3000);
    //初期設定が成功しない場合 reset して初期設定をリトライする
    ESP.restart();
    delay(5000);
    return;
  }
  //ルーターとの接続に成功
  Serial.println("connected...");
  delay(1000);

  //現在時刻を取得するための設定
  //NTPの設定
  configTime(9*3600L, 0, "ntp.nict.jp", "time.google.com", "ntp.jst.mfeed.ad.jp");

  //////////////////////////////////////////
  //アクセストークンを設定
  boccoAPI.setAccessToken(ACCESS_TOKEN);

  //BOCCO 1番目のルームを取得
  if(!boccoAPI.getFirstRoom()){
      Serial.println("BOCCO ルーム取得に失敗");
      return;
  }
  //////////////////////////////////////////

  pinMode(SW, INPUT_PULLUP);
  attachInterrupt(SW, clickSw, CHANGE );

}

//時刻を書くのするオブジェクト
struct tm timeInfo;

void loop() {
  //正確な現在時刻を取得
  //ある時刻にBOCCOを喋らせたい場合は、時刻でif文を書くと良い
  //  getLocalTime(&timeInfo);
  //  sprintf(s, " %04d/%02d/%02d %02d:%02d:%02d",
  //          timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
  //          timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);//人間が読める形式に変換
  //  delay(1000);
  //  Serial.println(s);//時間をシリアルモニタへ出力
  //  delay(1000);

  //スイッチが押された時にAPIから情報を取得
  if(valueSW == LOW){
    HTTPClient http;
    http.begin(API_URL);
    int httpCode = http.GET();

    //返答がある場合
    if(httpCode > 0){
      String payload = http.getString();
      deserializeJson(doc, payload);
      const char* message = doc["message"]; 
      JsonObject request = doc["request"];
      int request_altitude = request["altitude"]; 
      
      //JSONがさらに入れ子上になっているので、まずJSON Arrayに格納する
      //そこから２次元配列の要領で引き出す
      JsonArray response = doc["response"];
      int response_0_duration = response[0]["duration"]; // 439
      long response_0_risetime = response[0]["risetime"]; // 1596505697 
      Serial.print("response_0_duration : ");
      Serial.println(response_0_duration);
      Serial.print("response_0_risetime : ");
      Serial.println(response_0_risetime);

      //unixtimeを日付に変換
      //unixTimeToDaytime(unixtime, 時差(時間) , 時差(分));
      //JST（日本標準時）に変換する場合は下記の通り
      unixTimeToDaytime(response_0_risetime, 9, 0);

      String text = "ISSが" + String(date[1]) + "月" + String(date[2]) + "日" + String(date[3]) + "時"+ String(date[4]) + "分に通過するよ";
      char c_text[50];
      text.toCharArray(c_text, 50);
      /////////////////////////////////////
      boccoAPI.postMessageText(c_text);
      Serial.println(text);      
      /////////////////////////////////////
      
    }else{
      Serial.println("Error");
    }

  }

}
